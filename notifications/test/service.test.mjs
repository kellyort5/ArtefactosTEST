// Integración del componente completo con un broker MQTT real en proceso (aedes,
// TCP anónimo como el Mosquitto del MVP), un receptor de webhook local y la API
// de estado. Los eventos son los fixtures del contrato: son ficticios y no
// acreditan un incidente real del ESP32 (eso es la comprobación física 2.1).
import assert from 'node:assert/strict';
import {existsSync, readFileSync} from 'node:fs';
import http from 'node:http';
import net from 'node:net';
import {join} from 'node:path';
import {after, test} from 'node:test';
import {Aedes} from 'aedes';
import mqtt from 'mqtt';
import {createService} from '../src/service.js';
import {NODE_A, NODE_B, TOKEN_A, TOKEN_B, WEBHOOK_SECRET, bytes, ceboEvent, clock, fixture, makeConfig, noLog,
  recordingLog, tempDir, topicFor, until, webhookServer} from './helpers.mjs';

const cleanups = [];
after(async () => { for (const fn of cleanups.reverse()) await fn().catch(() => {}); });

async function broker() {
  const aedes = await Aedes.createBroker();
  const server = net.createServer(aedes.handle);
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const subscribed = [];
  aedes.on('subscribe', (subscriptions, client) => subscribed.push({client: client?.id, topics: subscriptions.map(s => s.topic)}));
  const handle = {aedes, url: `mqtt://127.0.0.1:${server.address().port}`, subscribed,
    close: async () => { server.close(); server.closeAllConnections?.(); await new Promise(resolve => aedes.close(resolve)); }};
  cleanups.push(handle.close);
  return handle;
}

async function publisher(url) {
  const client = await mqtt.connectAsync(url, {protocolVersion: 4, clientId: `pub-${Math.random().toString(16).slice(2)}`});
  cleanups.push(() => client.endAsync(true));
  return {publish: (topic, payload) => client.publishAsync(topic, payload, {qos: 1})};
}

async function setup({respond, c = clock(), stateDir, log = noLog, clientId = 'notify-test', brokerHandle} = {}) {
  const t = stateDir ? null : tempDir();
  const dir = stateDir ?? t.dir;
  const b = brokerHandle ?? await broker();
  const hook = await webhookServer(respond);
  const base = makeConfig({webhookPort: hook.port, stateDir: dir});
  const config = {...base, mqtt: {url: b.url, clientId}, statusApi: {...base.statusApi, port: 0}};
  const service = createService(config, {now: c.now, log});
  const {port} = await service.start();
  const handle = {t, dir, c, b, hook, config, service, port, log,
    stop: () => service.stop(),
    async ready(topicCount = 4) { await until(() => b.subscribed.filter(s => s.client === clientId).flatMap(s => s.topics).length >= topicCount); }};
  cleanups.push(async () => { await service.stop(); await hook.close(); t?.cleanup(); });
  return handle;
}

const status = (rig, token = TOKEN_A, device = NODE_A) => new Promise((resolve, reject) => {
  http.get({host: '127.0.0.1', port: rig.port, path: `/v1/status?device_id=${device}`, headers: {authorization: `Bearer ${token}`}, agent: false},
    res => { const chunks = []; res.on('data', d => chunks.push(d)); res.on('end', () => resolve({status: res.statusCode,
      json: res.statusCode === 200 ? JSON.parse(Buffer.concat(chunks).toString()) : null})); }).on('error', reject);
});

test('un incidente Cebo por MQTT llega al webhook, correlacionado con el evento y sin duplicados', async () => {
  const rig = await setup();
  await rig.ready();
  const pub = await publisher(rig.b.url);
  const event = ceboEvent({id: 'boot-1:42', severity: 'critical', ip: '192.0.2.77', protocol: 'telnet'});
  await pub.publish(topicFor(), bytes(event));
  await until(() => rig.hook.requests.length === 1);
  const [request] = rig.hook.requests;
  assert.equal(request.json.event_id, 'boot-1:42');
  assert.equal(request.json.device_id, NODE_A);
  assert.equal(request.json.severity, 'critical');
  assert.deepEqual(request.json.details, event.payload.details);
  assert.equal(request.headers['x-api-key'] ?? request.headers.authorization, `Bearer ${WEBHOOK_SECRET}`);

  // MQTT reentrega el mismo ID (QoS 1 puede duplicar): no hay segunda alerta.
  await pub.publish(topicFor(), bytes(event));
  await pub.publish(topicFor(), bytes(event));
  await until(() => rig.service.router.counters.duplicates === 2);
  await rig.service.deliver();
  assert.equal(rig.hook.requests.length, 1);
  await until(async () => (await status(rig)).json.channels.webhook.last_delivery?.status === 'provider_accepted');
  const {json} = await status(rig);
  assert.deepEqual(json.channels.webhook.last_delivery.event_id, 'boot-1:42');
  assert.equal(json.channels.webhook.last_delivery.read_status, 'unknown');
  assert.equal(json.channels.push.configured, false);
});

test('v1, eventos que no son de Cebo, inválidos y de otros nodos no producen alertas', async () => {
  const log = recordingLog();
  const rig = await setup({log});
  await rig.ready();
  const pub = await publisher(rig.b.url);
  await pub.publish(topicFor(), bytes(fixture('events/system.booted.valid.json')));          // v2, no es Cebo
  const v1 = fixture('../event.valid.json');
  await pub.publish(topicFor(v1.device_id, 1), bytes(v1));                                     // v1 de un nodo no configurado
  await pub.publish(topicFor(), bytes('{no es json FICTIONAL-SECRET'));                        // inválido
  await pub.publish(topicFor(), bytes({...ceboEvent({id: 'bad:1'}), extra: 'campo'}));         // viola el contrato
  await pub.publish(topicFor(NODE_B), bytes(ceboEvent({device: NODE_A, id: 'spoof:1'})));      // identidad cruzada
  await pub.publish(topicFor(), bytes(ceboEvent({id: 'info:1', severity: 'info'})));           // por debajo del umbral
  await until(() => rig.service.counters.ignored >= 1 && rig.service.counters.invalid >= 3 && rig.service.router.counters.below_threshold === 1);
  await rig.service.deliver();
  assert.equal(rig.hook.requests.length, 0);
  assert.ok(!JSON.stringify(log.entries).includes('FICTIONAL-SECRET'), 'lo recibido nunca se registra');
  assert.ok(!JSON.stringify(log.entries).includes(WEBHOOK_SECRET));
});

test('el estado se persiste antes de devolver el control al cliente MQTT (antes del PUBACK)', async () => {
  const rig = await setup({respond: () => ({status: 503})});
  const file = join(rig.dir, 'state.json');
  rig.service.onMessage(topicFor(), bytes(ceboEvent({id: 'boot-3:1'})));
  // Síncrono: al volver ya están en disco el ID visto y la alerta pendiente.
  assert.ok(existsSync(file));
  const saved = JSON.parse(readFileSync(file, 'utf8'));
  assert.ok(saved.seen[`${NODE_A}/boot-3:1`]);
  assert.equal(saved.outbox.webhook[0].alert.event_id, 'boot-3:1');
});

test('si el webhook falla se reintenta con límites visibles y se recupera sin duplicar ni perder', async () => {
  let up = false;
  const accepted = []; // alertas que el destino aceptó (2xx)
  const c = clock();
  const rig = await setup({c, respond: req => {
    if (!up) return {status: 503};
    accepted.push(req.json.event_id);
    return {status: 200};
  }});
  await rig.ready();
  const pub = await publisher(rig.b.url);
  await pub.publish(topicFor(), bytes(ceboEvent({id: 'boot-1:1', ip: '192.0.2.1'})));
  await pub.publish(topicFor(), bytes(ceboEvent({id: 'boot-1:2', ip: '192.0.2.2'})));
  await until(() => rig.service.router.counters.routed === 2);
  await until(() => rig.hook.requests.length >= 1);
  let state = (await status(rig)).json.channels.webhook;
  assert.equal(state.pending, 2);
  assert.deepEqual([state.last_delivery.status, state.last_delivery.error], ['failed', 'provider_unavailable']);
  const attempts = rig.hook.requests.length;
  await rig.service.deliver();                       // todavía en espera de 1 s: no hay intento nuevo
  assert.equal(rig.hook.requests.length, attempts);
  up = true;
  c.advance(1000);
  await rig.service.deliver();
  assert.deepEqual(rig.hook.requests.slice(attempts).map(r => r.json.event_id), ['boot-1:1', 'boot-1:2']);
  state = (await status(rig)).json.channels.webhook;
  assert.deepEqual([state.pending, state.last_delivery.status, state.last_delivery.error], [0, 'provider_accepted', null]);
  // Cada alerta se aceptó exactamente una vez, aunque hubo varios intentos fallidos.
  assert.deepEqual(accepted, ['boot-1:1', 'boot-1:2']);
  c.advance(60000);
  await rig.service.deliver();
  assert.deepEqual(accepted, ['boot-1:1', 'boot-1:2']);
});

test('un reinicio del componente conserva lo pendiente y la deduplicación', async () => {
  let up = false;
  const c = clock();
  const first = await setup({c, respond: () => (up ? {status: 200} : {status: 500})});
  await first.ready();
  first.service.onMessage(topicFor(), bytes(ceboEvent({id: 'boot-5:1', ip: '192.0.2.5'})));
  await first.service.deliver();
  assert.equal((await status(first)).json.channels.webhook.pending, 1);
  await first.stop();

  up = true;
  // Mismo directorio de estado, mismo destino: otro proceso.
  const base = makeConfig({webhookPort: first.hook.port, stateDir: first.dir});
  const config = {...base, mqtt: {url: first.b.url, clientId: 'notify-test-2'}, statusApi: {...base.statusApi, port: 0}};
  const second = createService(config, {now: c.now, log: noLog});
  cleanups.push(() => second.stop());
  await second.start();
  c.advance(60000);
  await until(async () => { await second.deliver(); return second.channels.webhook.status(NODE_A).pending === 0; });
  assert.equal(first.hook.requests.at(-1).json.event_id, 'boot-5:1');
  // La retransmisión del mismo evento tras el reinicio no genera otra alerta.
  const sent = first.hook.requests.length;
  second.onMessage(topicFor(), bytes(ceboEvent({id: 'boot-5:1', ip: '192.0.2.5'})));
  await second.deliver();
  assert.equal(first.hook.requests.length, sent);
  assert.equal(second.router.counters.duplicates, 1);
});

test('con el componente caído, el broker conserva los eventos QoS 1 (sesión persistente) y se procesan al volver', async () => {
  const b = await broker();
  const rig = await setup({brokerHandle: b, clientId: 'notify-persistent'});
  await rig.ready();
  await rig.stop();
  const pub = await publisher(b.url);
  await pub.publish(topicFor(), bytes(ceboEvent({id: 'boot-7:1', severity: 'critical', ip: '192.0.2.7'})));
  const base = makeConfig({webhookPort: rig.hook.port, stateDir: rig.dir});
  const again = createService({...base, mqtt: {url: b.url, clientId: 'notify-persistent'}, statusApi: {...base.statusApi, port: 0}},
    {now: rig.c.now, log: noLog});
  cleanups.push(() => again.stop());
  await again.start();
  await until(() => rig.hook.requests.some(r => r.json.event_id === 'boot-7:1'));
});

test('el estado de un nodo no se ve con la credencial de otro y no expone secretos', async () => {
  const rig = await setup();
  rig.service.onMessage(topicFor(NODE_A), bytes(ceboEvent({id: 'boot-8:1', device: NODE_A})));
  await rig.service.deliver();
  const own = await status(rig, TOKEN_B, NODE_B);
  assert.equal(own.status, 200);
  assert.equal(own.json.channels.webhook.last_delivery, null);
  assert.equal((await status(rig, TOKEN_B, NODE_A)).status, 403);
  assert.equal((await status(rig, 'FICTIONAL-WRONG', NODE_A)).status, 401);
  const text = JSON.stringify((await status(rig)).json);
  for (const secret of [WEBHOOK_SECRET, TOKEN_A, 'x-api-key', '127.0.0.1']) assert.ok(!text.includes(secret));
});

test('sin webhook configurado el estado lo indica sin afirmar entrega', async () => {
  const t = tempDir();
  const b = await broker();
  const base = makeConfig({stateDir: t.dir});
  const config = {...base, webhook: null, mqtt: {url: b.url, clientId: 'notify-none'}, statusApi: {...base.statusApi, port: 0}};
  const service = createService(config, {now: clock().now, log: noLog});
  cleanups.push(async () => { await service.stop(); t.cleanup(); });
  const {port} = await service.start();
  service.onMessage(topicFor(), bytes(ceboEvent({id: 'boot-9:1'})));
  const rig = {port};
  const {json} = await status(rig);
  assert.deepEqual(json.channels.webhook, {configured: false, last_delivery: null, pending: 0, suppressed: 0});
  assert.deepEqual(json.channels.push, {configured: false, last_delivery: null, pending: 0, suppressed: 0});
});
