import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {join} from 'node:path';
import {test} from 'node:test';
import {decodePushNotification} from '@sentinel/contracts';
import {backoff} from '../src/channel.js';
import {ingest} from '../src/ingest.js';
import {PushChannel, createPushSender} from '../src/push.js';
import {RegistrationStore} from '../src/registry.js';
import {Router} from '../src/router.js';
import {StateStore} from '../src/state.js';
import {NODE_A, NODE_B, TOKEN_ANDROID_1, TOKEN_ANDROID_2, bytes, ceboEvent, clock, fakeProvider, recordingLog,
  tempDir, topicFor} from './helpers.mjs';

const allowed = new Set([NODE_A, NODE_B]);
const eventOf = options => ingest(topicFor(options?.device ?? NODE_A), bytes(ceboEvent(options)), allowed).event;

function rig({results = []} = {}) {
  const t = tempDir();
  const c = clock();
  const log = recordingLog();
  const store = new StateStore({dir: t.dir, now: c.now, log});
  store.load();
  const registry = new RegistrationStore({dir: t.dir, now: c.now, log});
  registry.load();
  const provider = fakeProvider({results});
  const channel = new PushChannel({registry, store, sender: createPushSender({registry, provider, log}), now: c.now, log});
  const router = new Router({store, channels: {push: channel}, config: {minSeverity: 'warning', includeSimulated: true, repeatWindowMs: 30000}, now: c.now, log});
  return {t, c, log, store, registry, provider, channel, router};
}

test('cada instalación del nodo recibe un push; las de otros nodos no', async () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.registry.register(NODE_A, 'inst-2', TOKEN_ANDROID_2);
  r.registry.register(NODE_B, 'inst-3', 'FICTIONAL-OTHER-NODE-TOKEN');
  assert.deepEqual(r.router.handle(eventOf({id: 'boot-1:10', severity: 'critical'})), {outcome: 'routed', channels: ['push']});
  assert.equal(r.channel.queue.length, 2);
  assert.equal(await r.channel.process(), 2);
  assert.deepEqual(r.provider.sent.map(s => s.token).sort(), [TOKEN_ANDROID_1, TOKEN_ANDROID_2]);
  assert.equal(r.channel.status(NODE_A).pending, 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.status, 'provider_accepted');
  assert.equal(r.channel.status(NODE_B).last_delivery, null);
  r.t.cleanup();
});

test('el push lleva exactamente el contenido del contrato con el ID original y nada más', async () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-7:3', severity: 'warning', ip: '192.0.2.99', protocol: 'telnet'}));
  await r.channel.process();
  const [{push}] = r.provider.sent;
  assert.deepEqual(push, {
    notification: {title: 'Sentinel Node', body: 'Actividad detectada en Cebo. Abre la app para consultar el incidente.'},
    data: {device_id: NODE_A, event_id: 'boot-7:3', severity: 'warning', route: 'cebo'}});
  // Es la proyección que valida el contrato frente al evento original.
  decodePushNotification(JSON.stringify(push), NODE_A, {device_id: NODE_A, message_id: 'boot-7:3',
    payload: {name: 'cebo.interaction', profile: 'cebo', severity: 'warning'}});
  const sent = JSON.stringify(r.provider.sent);
  for (const private_ of ['192.0.2.99', 'telnet', 'source_ipv4', 'details', 'auth_attempt', 'password']) assert.ok(!sent.includes(private_), private_);
  r.t.cleanup();
});

test('la cola y el estado solo guardan identificadores: nunca el token ni la IP de origen', async () => {
  const r = rig({results: [{ok: false, error: 'provider_unavailable'}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-8:1', ip: '192.0.2.88'}));
  await r.channel.process(); // falla: el elemento sigue pendiente en el estado
  r.store.save();
  const saved = readFileSync(join(r.t.dir, 'state.json'), 'utf8');
  assert.ok(saved.includes('boot-8:1') && saved.includes('inst-1'));
  for (const secret of [TOKEN_ANDROID_1, '192.0.2.88', 'source_ipv4']) assert.ok(!saved.includes(secret), secret);
  r.t.cleanup();
});

test('sin instalaciones registradas no se encola nada', () => {
  const r = rig();
  assert.deepEqual(r.router.handle(eventOf({id: 'boot-1:1'})), {outcome: 'suppressed', channels: []});
  assert.equal(r.channel.queue.length, 0);
  r.registry.register(NODE_B, 'inst-1', TOKEN_ANDROID_1); // solo hay una instalación de otro nodo
  assert.equal(r.router.handle(eventOf({id: 'boot-1:2', ip: '192.0.2.5'})).channels.length, 0);
  r.t.cleanup();
});

test('un token inválido se retira, no se reintenta y no afecta a las demás instalaciones', async () => {
  const r = rig({results: [{ok: false, error: 'invalid_registration', drop: true, remove: true}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.registry.register(NODE_A, 'inst-2', TOKEN_ANDROID_2);
  r.router.handle(eventOf({id: 'boot-1:1'}));
  assert.equal(await r.channel.process(), 1); // la primera se descarta y la segunda se envía
  assert.equal(r.provider.sent.length, 2);
  assert.equal(r.registry.get(NODE_A, 'inst-1'), null);
  assert.ok(r.registry.get(NODE_A, 'inst-2'));
  assert.equal(r.channel.queue.length, 0);
  assert.equal(r.channel.failures, 0, 'un fallo definitivo no frena el canal');
  // La baja quedó guardada en disco, sin el token.
  const reloaded = new RegistrationStore({dir: r.t.dir, now: r.c.now});
  reloaded.load();
  assert.equal(reloaded.get(NODE_A, 'inst-1'), null);
  assert.ok(r.log.entries.some(e => e.msg === 'registration_removed' && e.installation_id === 'inst-1'));
  // Nada de lo registrado contiene un token.
  assert.ok(!JSON.stringify(r.log.entries).includes('FICTIONAL'));
  // La última entrega informada es la aceptada, y el fallo no se pierde en el contador.
  assert.equal(r.channel.status(NODE_A).last_delivery.status, 'provider_accepted');
  assert.equal(r.channel.stats.dropped, 1);
  r.t.cleanup();
});

test('un único token inválido deja el fallo visible en el estado', async () => {
  const r = rig({results: [{ok: false, error: 'invalid_registration', drop: true, remove: true}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-1:1'}));
  await r.channel.process();
  const d = r.channel.status(NODE_A).last_delivery;
  assert.deepEqual([d.event_id, d.status, d.error, d.read_status], ['boot-1:1', 'failed', 'invalid_registration', 'unknown']);
  assert.equal(r.channel.status(NODE_A).pending, 0);
  assert.equal(r.registry.size, 0);
  r.t.cleanup();
});

test('un fallo temporal del proveedor se reintenta con espera creciente y se recupera', async () => {
  const r = rig({results: [{ok: false, error: 'provider_unavailable'}, {ok: false, error: 'rate_limited'}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-1:1'}));
  assert.equal(await r.channel.process(), 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'provider_unavailable');
  assert.equal(r.channel.status(NODE_A).pending, 1);
  assert.ok(r.registry.get(NODE_A, 'inst-1'), 'un fallo temporal no borra la instalación');
  r.c.advance(backoff(1));
  assert.equal(await r.channel.process(), 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'rate_limited');
  r.c.advance(backoff(2));
  assert.equal(await r.channel.process(), 1);
  assert.equal(r.provider.sent.length, 3);
  assert.equal(r.channel.status(NODE_A).last_delivery.status, 'provider_accepted');
  r.t.cleanup();
});

test('si la instalación se da de baja o vence mientras espera, el envío se descarta sin llamar al proveedor', async () => {
  const r = rig({results: [{ok: false, error: 'provider_unavailable'}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-1:1'}));
  await r.channel.process();                       // falla y queda pendiente
  r.registry.remove(NODE_A, 'inst-1');             // baja del usuario
  r.c.advance(backoff(1));
  assert.equal(await r.channel.process(), 0);
  assert.equal(r.provider.sent.length, 1, 'no se vuelve a llamar al proveedor');
  assert.equal(r.channel.queue.length, 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'invalid_registration');
  r.t.cleanup();
});

test('la renovación con otro token envía con el nuevo y no con el anterior', async () => {
  const r = rig({results: [{ok: false, error: 'provider_unavailable'}]});
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.router.handle(eventOf({id: 'boot-1:1'}));
  await r.channel.process();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_2); // renovación con token nuevo
  r.c.advance(backoff(1));
  await r.channel.process();
  assert.deepEqual(r.provider.sent.map(s => s.token), [TOKEN_ANDROID_1, TOKEN_ANDROID_2]);
  r.t.cleanup();
});

test('la deduplicación por evento evita pushes repetidos y la ventana de 30 s los controla por canal', async () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  const first = eventOf({id: 'boot-1:1'});
  assert.equal(r.router.handle(first).outcome, 'routed');
  assert.equal(r.router.handle(first).outcome, 'duplicate'); // reentrega de MQTT
  r.c.advance(5000);
  assert.equal(r.router.handle(eventOf({id: 'boot-1:2'})).outcome, 'suppressed'); // mismo origen en ventana
  assert.equal(r.channel.status(NODE_A).suppressed, 1);
  await r.channel.process();
  assert.equal(r.provider.sent.length, 1);
  r.t.cleanup();
});

test('un canal sin proveedor configurado lo indica y no encola', () => {
  const t = tempDir();
  const store = new StateStore({dir: t.dir});
  store.load();
  const registry = new RegistrationStore({dir: t.dir});
  registry.load();
  registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  const channel = new PushChannel({registry, store, sender: null});
  assert.equal(channel.enqueue({event_key: 'a/b', body: {device_id: NODE_A, event_id: 'b', severity: 'warning'}}), false);
  assert.deepEqual(channel.status(NODE_A), {configured: false, last_delivery: null, pending: 0, suppressed: 0});
  t.cleanup();
});
