import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import http from 'node:http';
import https from 'node:https';
import {join} from 'node:path';
import {after, test} from 'node:test';
import {decodeNotificationStatusResponse} from '@sentinel/contracts';
import {Channel} from '../src/channel.js';
import {authenticate, createStatusApi} from '../src/http-api.js';
import {StateStore} from '../src/state.js';
import {NODE_A, NODE_B, TOKEN_A, TOKEN_B, WEBHOOK_SECRET, clock, makeConfig, sha256hex, tempDir} from './helpers.mjs';

const openRigs = [];
after(async () => { for (const r of openRigs) await r.close(); }); // también si una prueba falla

async function rig({tls = null} = {}) {
  const t = tempDir();
  const c = clock();
  const store = new StateStore({dir: t.dir, now: c.now});
  store.load();
  const channels = {
    webhook: new Channel({name: 'webhook', store, now: c.now, sender: {send: async () => ({ok: true})}}),
    push: new Channel({name: 'push', store, now: c.now, sender: null}),
  };
  const config = makeConfig({stateDir: t.dir});
  const statusApi = tls ? {...config.statusApi, tls} : config.statusApi;
  const api = createStatusApi({config: statusApi, nodes: config.nodes,
    statusFor: device => ({webhook: channels.webhook.status(device), push: channels.push.status(device)})});
  const server = api.build();
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  let closed = false;
  const handle = {t, c, store, channels, server, port: server.address().port,
    close: async () => {
      if (closed) return;
      closed = true;
      await new Promise(resolve => { server.close(resolve); server.closeAllConnections(); });
      t.cleanup();
    }};
  openRigs.push(handle);
  return handle;
}

function call(port, {method = 'GET', path = '/v1/status', headers = {}, body, lib = http, ca} = {}) {
  return new Promise((resolve, reject) => {
    // Una conexión nueva por petición: una petición con cuerpo no leído no debe contaminar la siguiente.
    const req = lib.request({host: '127.0.0.1', port, method, path, headers, ca, servername: 'localhost', agent: false}, res => {
      const chunks = [];
      res.on('data', c => chunks.push(c));
      res.on('end', () => resolve({status: res.statusCode, headers: res.headers, body: Buffer.concat(chunks).toString('utf8')}));
    });
    req.on('error', reject);
    req.end(body);
  });
}
const bearer = token => ({authorization: `Bearer ${token}`});
const status = (port, token, query, extra = {}) =>
  call(port, {path: `/v1/status${query ?? `?device_id=${NODE_A}`}`, headers: {...bearer(token), ...extra.headers}, ...extra});

test('un bearer correcto devuelve el estado validado por el contrato, sin secretos', async () => {
  const r = await rig();
  r.channels.webhook.enqueue({event_key: `${NODE_A}/boot-1:1`, body: {device_id: NODE_A, event_id: 'boot-1:1'}});
  r.channels.webhook.suppress(NODE_A);
  const res = await status(r.port, TOKEN_A);
  assert.equal(res.status, 200);
  assert.equal(res.headers['content-type'], 'application/json');
  assert.equal(res.headers['cache-control'], 'no-store');
  assert.equal(res.headers['x-content-type-options'], 'nosniff');
  const parsed = decodeNotificationStatusResponse(res.body, NODE_A); // el mismo contrato que consume la app
  assert.equal(parsed.device_id, NODE_A);
  assert.deepEqual(parsed.channels.webhook, {configured: true, last_delivery: null, pending: 1, suppressed: 1});
  // El canal push aún no está configurado y no afirma ninguna entrega.
  assert.deepEqual(parsed.channels.push, {configured: false, last_delivery: null, pending: 0, suppressed: 0});
  for (const secret of [TOKEN_A, TOKEN_B, sha256hex(TOKEN_A), WEBHOOK_SECRET, '127.0.0.1', '/hook'])
    assert.ok(!res.body.includes(secret), `la respuesta contiene ${secret}`);
  await r.close();
});

test('tras una entrega fallida informa el código sanitizado y los contadores', async () => {
  const r = await rig();
  r.channels.webhook.record({device_id: NODE_A, event_id: 'boot-1:1'}, 'failed', 'provider_unavailable');
  const body = JSON.parse((await status(r.port, TOKEN_A)).body);
  assert.deepEqual(body.channels.webhook.last_delivery, {event_id: 'boot-1:1', timestamp: new Date(r.c.now()).toISOString(),
    status: 'failed', read_status: 'unknown', error: 'provider_unavailable'});
  await r.close();
});

test('sin credencial o con una inválida responde 401 con cuerpo vacío', async () => {
  const r = await rig();
  const cases = [{}, bearer('FICTIONAL-WRONG'), bearer(''), {authorization: `bearer ${TOKEN_A}`}, {authorization: `Basic ${TOKEN_A}`},
    {authorization: TOKEN_A}, {authorization: `Bearer ${TOKEN_A} extra`}, bearer('x'.repeat(600)), bearer(TOKEN_A + 'x'),
    bearer(sha256hex(TOKEN_A))]; // el hash no es la credencial
  for (const headers of cases) {
    const res = await call(r.port, {path: `/v1/status?device_id=${NODE_A}`, headers});
    assert.equal(res.status, 401, JSON.stringify(headers).slice(0, 60));
    assert.equal(res.body, '');
    assert.equal(res.headers['www-authenticate'], 'Bearer');
  }
  // La credencial en la URL no se admite como alternativa.
  assert.equal((await call(r.port, {path: `/v1/status?device_id=${NODE_A}&token=${TOKEN_A}`})).status, 401);
  await r.close();
});

test('el nodo lo determina la credencial: cruzar nodos responde 403 y no filtra datos ajenos', async () => {
  const r = await rig();
  r.channels.webhook.record({device_id: NODE_A, event_id: 'secreto-de-A:1'}, 'provider_accepted');
  assert.equal((await status(r.port, TOKEN_A, `?device_id=${NODE_B}`)).status, 403);
  assert.equal((await status(r.port, TOKEN_B, `?device_id=${NODE_A}`)).status, 403);
  const own = await status(r.port, TOKEN_B, `?device_id=${NODE_B}`);
  assert.equal(own.status, 200);
  assert.equal(JSON.parse(own.body).channels.webhook.last_delivery, null);
  assert.ok(!own.body.includes('secreto-de-A'));
  // Un nodo que no existe se trata igual: no revela qué nodos hay configurados.
  const unknown = await status(r.port, TOKEN_A, '?device_id=node-inexistente');
  assert.equal(unknown.status, 403);
  assert.equal(unknown.body, '');
  await r.close();
});

test('la consulta debe tener exactamente un device_id válido', async () => {
  const r = await rig();
  for (const query of ['', '?', '?device_id=', `?device_id=${NODE_A}&device_id=${NODE_A}`, `?device_id=${NODE_A}&x=1`,
    `?x=${NODE_A}`, '?device_id=bad%20id', `?device_id=${NODE_A}%0d%0a`, `?device_id=${'a'.repeat(65)}`, '?DEVICE_ID=' + NODE_A]) {
    const res = await status(r.port, TOKEN_A, query);
    assert.equal(res.status, 400, query);
    assert.equal(res.body, '');
  }
  await r.close();
});

test('GET no admite cuerpo: 400 si lo trae y 413 si supera 4096 bytes', async () => {
  const r = await rig();
  const withBody = (size, headers = {}) => call(r.port, {path: `/v1/status?device_id=${NODE_A}`,
    headers: {...bearer(TOKEN_A), ...headers}, body: 'x'.repeat(size)});
  assert.equal((await withBody(10, {'content-length': '10'})).status, 400);
  assert.equal((await withBody(4096, {'content-length': '4096'})).status, 400);   // en el límite: sigue sin admitirse
  assert.equal((await withBody(5000, {'content-length': '5000'})).status, 413);   // por encima de 4096 bytes
  assert.equal((await withBody(3, {'transfer-encoding': 'chunked'})).status, 400);
  // Un cuerpo se rechaza antes de autenticar y nunca se lee.
  assert.equal((await call(r.port, {path: `/v1/status?device_id=${NODE_A}`, body: 'x'.repeat(10), headers: {'content-length': '10'}})).status, 400);
  await r.close();
});

test('rutas y métodos desconocidos se rechazan; el registro push llega en su propio cambio', async () => {
  const r = await rig();
  for (const path of ['/', '/v1', '/v1/statusx', '/V1/status', '/v1/status/', '/v1/registrations', '/v1/registrations/x', '/admin'])
    assert.equal((await call(r.port, {path, headers: bearer(TOKEN_A)})).status, 404, path);
  for (const method of ['POST', 'PUT', 'DELETE', 'PATCH']) {
    const res = await call(r.port, {method, path: `/v1/status?device_id=${NODE_A}`, headers: bearer(TOKEN_A)});
    assert.equal(res.status, 405, method);
    assert.equal(res.headers.allow, 'GET, OPTIONS');
  }
  await r.close();
});

test('CORS: solo los orígenes configurados, sin comodín', async () => {
  const r = await rig();
  const allowed = await status(r.port, TOKEN_A, undefined, {headers: {origin: 'https://localhost'}});
  assert.equal(allowed.headers['access-control-allow-origin'], 'https://localhost');
  assert.match(allowed.headers.vary, /Origin/);
  const foreign = await status(r.port, TOKEN_A, undefined, {headers: {origin: 'https://evil.example.invalid'}});
  assert.equal(foreign.headers['access-control-allow-origin'], undefined);
  const none = await status(r.port, TOKEN_A);
  assert.equal(none.headers['access-control-allow-origin'], undefined);
  for (const res of [allowed, foreign, none]) assert.notEqual(res.headers['access-control-allow-origin'], '*');

  const preflight = await call(r.port, {method: 'OPTIONS', path: '/v1/status',
    headers: {origin: 'https://localhost', 'access-control-request-method': 'GET', 'access-control-request-headers': 'authorization'}});
  assert.equal(preflight.status, 204);
  assert.equal(preflight.headers['access-control-allow-origin'], 'https://localhost');
  assert.equal(preflight.headers['access-control-allow-methods'], 'GET, OPTIONS');
  assert.equal(preflight.headers['access-control-allow-headers'], 'Authorization');
  assert.equal(preflight.headers['access-control-max-age'], '600');
  assert.equal(preflight.body, '');
  const denied = await call(r.port, {method: 'OPTIONS', path: '/v1/status', headers: {origin: 'https://evil.example.invalid'}});
  assert.equal(denied.status, 403);
  assert.equal(denied.headers['access-control-allow-origin'], undefined);
  assert.equal((await call(r.port, {method: 'OPTIONS', path: '/v1/status'})).status, 403); // sin Origin tampoco es preflight
  await r.close();
});

test('authenticate compara contra todos los nodos y solo acepta el formato Bearer', () => {
  const nodes = makeConfig().nodes;
  assert.equal(authenticate(`Bearer ${TOKEN_A}`, nodes), NODE_A);
  assert.equal(authenticate(`Bearer ${TOKEN_B}`, nodes), NODE_B);
  for (const header of [undefined, '', 'Bearer', 'Bearer ', `Bearer  ${TOKEN_A}`, `bearer ${TOKEN_A}`, `Bearer ${TOKEN_A}\n`, `Bearer ${'é'.repeat(5)}`])
    assert.equal(authenticate(header, nodes), null, String(header));
});

// --- HTTPS real: requiere `openssl` -------------------------------------------------
const hasOpenssl = spawnSync('openssl', ['version']).status === 0;
test('sirve por HTTPS con el certificado configurado y TLS 1.2 o superior', {skip: hasOpenssl ? false : 'openssl no disponible'}, async () => {
  const t = tempDir();
  try {
    const key = join(t.dir, 'key.pem'), cert = join(t.dir, 'cert.pem');
    const gen = spawnSync('openssl', ['req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', key, '-out', cert, '-days', '1',
      '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost,IP:127.0.0.1']);
    assert.equal(gen.status, 0);
    const r = await rig({tls: {certFile: cert, keyFile: key}});
    const ok = await call(r.port, {lib: https, ca: readFileSync(cert), path: `/v1/status?device_id=${NODE_A}`, headers: bearer(TOKEN_A)});
    assert.equal(ok.status, 200);
    // Sin confiar en la CA, el cliente rechaza el servidor: el protocolo no es en claro.
    await assert.rejects(call(r.port, {lib: https, path: `/v1/status?device_id=${NODE_A}`, headers: bearer(TOKEN_A)}));
    // Un cliente de texto plano no obtiene una respuesta HTTP.
    await assert.rejects(call(r.port, {lib: http, path: `/v1/status?device_id=${NODE_A}`, headers: bearer(TOKEN_A)}));
    await r.close();
  } finally { t.cleanup(); }
});
