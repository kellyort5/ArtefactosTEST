import assert from 'node:assert/strict';
import {test} from 'node:test';
import {Channel} from '../src/channel.js';
import {ingest} from '../src/ingest.js';
import {Router} from '../src/router.js';
import {StateStore} from '../src/state.js';
import {NODE_A, NODE_B, bytes, ceboEvent, clock, recordingLog, tempDir, topicFor} from './helpers.mjs';

const allowed = new Set([NODE_A, NODE_B]);
const event = options => {
  const result = ingest(topicFor(options?.device ?? NODE_A), bytes(ceboEvent(options)), allowed);
  assert.equal(result.kind, 'event');
  return result.event;
};

function rig({minSeverity = 'warning', includeSimulated = true, repeatWindowMs = 30000, webhook = true} = {}) {
  const t = tempDir();
  const c = clock();
  const log = recordingLog();
  const store = new StateStore({dir: t.dir, now: c.now, log});
  store.load();
  const sent = [];
  const sender = webhook ? {send: async alert => { sent.push(alert); return {ok: true}; }} : null;
  const channels = {webhook: new Channel({name: 'webhook', store, sender, now: c.now, log}),
    push: new Channel({name: 'push', store, sender: null, now: c.now, log})};
  const router = new Router({store, channels, config: {minSeverity, includeSimulated, repeatWindowMs}, now: c.now, log});
  return {t, c, store, channels, router, sent, log};
}

test('una alerta se correlaciona con el evento y solo lleva los metadatos permitidos', () => {
  const r = rig();
  const out = r.router.handle(event({id: 'boot-9:3', severity: 'critical', ip: '192.0.2.9', protocol: 'telnet'}));
  assert.deepEqual(out, {outcome: 'routed', channels: ['webhook']});
  const [{alert}] = r.channels.webhook.queue;
  assert.equal(alert.schema, 'sentinel.alert.v1');
  assert.equal(alert.device_id, NODE_A);
  assert.equal(alert.event_id, 'boot-9:3'); // el ID original del evento, nunca uno nuevo
  assert.equal(alert.severity, 'critical');
  assert.equal(alert.details.source_ipv4, '192.0.2.9');
  assert.deepEqual(Object.keys(alert).sort(), ['boot_id', 'details', 'device_id', 'event_id', 'name', 'origin',
    'schema', 'severity', 'time_quality', 'timestamp', 'uptime_ms']);
  r.t.cleanup();
});

test('la misma identidad nunca produce dos alertas, aunque MQTT la retransmita', () => {
  const r = rig();
  assert.equal(r.router.handle(event({id: 'boot-1:1'})).outcome, 'routed');
  for (let i = 0; i < 3; ++i) assert.equal(r.router.handle(event({id: 'boot-1:1'})).outcome, 'duplicate');
  assert.equal(r.channels.webhook.queue.length, 1);
  assert.equal(r.router.counters.duplicates, 3);
  // La identidad es (nodo, ID): el mismo ID en otro nodo es otro evento.
  assert.equal(r.router.handle(event({id: 'boot-1:1', device: NODE_B})).outcome, 'routed');
  r.t.cleanup();
});

test('la severidad mínima y el origen simulado se aplican según la configuración', () => {
  const r = rig({minSeverity: 'warning', includeSimulated: false});
  assert.equal(r.router.handle(event({id: 'a:1', severity: 'info'})).outcome, 'below_threshold');
  assert.equal(r.router.handle(event({id: 'a:2', severity: 'warning', origin: 'simulated'})).outcome, 'simulated');
  assert.equal(r.router.handle(event({id: 'a:3', severity: 'warning', origin: 'real'})).outcome, 'routed');
  assert.equal(r.router.handle(event({id: 'a:4', severity: 'critical', ip: '192.0.2.4'})).outcome, 'routed');
  const info = rig({minSeverity: 'info'});
  assert.equal(info.router.handle(event({id: 'a:1', severity: 'info'})).outcome, 'routed');
  [r, info].forEach(x => x.t.cleanup());
});

test('un origen repetido se suprime dentro de la ventana y se cuenta', () => {
  const r = rig();
  assert.equal(r.router.handle(event({id: 'a:1'})).outcome, 'routed');
  r.c.advance(10000);
  assert.equal(r.router.handle(event({id: 'a:2'})).outcome, 'suppressed');
  r.c.advance(10000);
  assert.equal(r.router.handle(event({id: 'a:3'})).outcome, 'suppressed');
  assert.equal(r.channels.webhook.queue.length, 1);
  assert.equal(r.channels.webhook.status(NODE_A).suppressed, 2);
  // La ventana se cuenta desde la última alerta enviada (30 s desde la primera).
  r.c.advance(10000);
  assert.equal(r.router.handle(event({id: 'a:4'})).outcome, 'routed');
  assert.equal(r.channels.webhook.queue.length, 2);
  // Otro origen, otro protocolo u otro nodo tienen su propia ventana.
  assert.equal(r.router.handle(event({id: 'b:1', ip: '192.0.2.50'})).outcome, 'routed');
  assert.equal(r.router.handle(event({id: 'b:2', protocol: 'telnet'})).outcome, 'routed');
  assert.equal(r.router.handle(event({id: 'b:3', device: NODE_B})).outcome, 'routed');
  // El contador es por nodo: el otro nodo no ve la supresión.
  assert.equal(r.channels.webhook.status(NODE_B).suppressed, 0);
  r.t.cleanup();
});

test('una escalada de severidad no se suprime aunque el origen esté en ventana', () => {
  const r = rig();
  assert.equal(r.router.handle(event({id: 'a:1', severity: 'warning'})).outcome, 'routed');
  r.c.advance(1000);
  assert.equal(r.router.handle(event({id: 'a:2', severity: 'warning'})).outcome, 'suppressed');
  r.c.advance(1000);
  assert.equal(r.router.handle(event({id: 'a:3', severity: 'critical'})).outcome, 'routed'); // escalada
  r.c.advance(1000);
  assert.equal(r.router.handle(event({id: 'a:4', severity: 'critical'})).outcome, 'suppressed');
  // Una severidad menor tras la escalada sigue dentro de la ventana de la alerta crítica.
  assert.equal(r.router.handle(event({id: 'a:5', severity: 'warning'})).outcome, 'suppressed');
  assert.equal(r.channels.webhook.queue.length, 2);
  r.t.cleanup();
});

test('con ventana 0 no hay control de repeticiones y sin webhook no se encola nada', () => {
  const r = rig({repeatWindowMs: 0});
  assert.equal(r.router.handle(event({id: 'a:1'})).outcome, 'routed');
  assert.equal(r.router.handle(event({id: 'a:2'})).outcome, 'routed');
  const none = rig({webhook: false});
  assert.deepEqual(none.router.handle(event({id: 'a:1'})), {outcome: 'suppressed', channels: []});
  assert.equal(none.channels.webhook.status(NODE_A).configured, false);
  assert.equal(none.channels.webhook.status(NODE_A).pending, 0);
  assert.equal(none.channels.webhook.status(NODE_A).last_delivery, null);
  [r, none].forEach(x => x.t.cleanup());
});

test('el control de repeticiones recuerda un número acotado de orígenes', () => {
  const r = rig();
  for (let i = 0; i < 400; ++i)
    r.router.handle(event({id: `a:${i}`, ip: `198.51.${Math.floor(i / 250)}.${i % 250}`}));
  assert.ok(r.router.windows.size <= 256);
  r.t.cleanup();
});
