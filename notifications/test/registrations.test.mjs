// API de registro, renovación y baja de instalaciones push (contrato de notificaciones).
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import http from 'node:http';
import {join} from 'node:path';
import {after, test} from 'node:test';
import {decodeNotificationRegistrationResponse, decodeNotificationUnregistrationResponse} from '@sentinel/contracts';
import {createStatusApi} from '../src/http-api.js';
import {MAX_REGISTRATIONS, REGISTRATION_TTL_MS, RegistrationStore} from '../src/registry.js';
import {NODE_A, NODE_B, TOKEN_A, TOKEN_B, TOKEN_ANDROID_1, TOKEN_ANDROID_2, clock, makeConfig, recordingLog, tempDir}
  from './helpers.mjs';

const openRigs = [];
after(async () => { for (const r of openRigs) await r.close(); });

async function rig({configured = true} = {}) {
  const t = tempDir();
  const c = clock();
  const log = recordingLog();
  const registry = new RegistrationStore({dir: t.dir, now: c.now, log});
  registry.load();
  const config = makeConfig({stateDir: t.dir});
  const api = createStatusApi({config: config.statusApi, nodes: config.nodes, statusFor: () => ({}),
    registrations: {configured, register: (d, i, tk) => registry.register(d, i, tk), remove: (d, i) => registry.remove(d, i), save: () => registry.save()}});
  const server = api.build();
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  let closed = false;
  const handle = {t, c, log, registry, port: server.address().port,
    close: async () => {
      if (closed) return;
      closed = true;
      await new Promise(resolve => { server.close(resolve); server.closeAllConnections(); });
      t.cleanup();
    }};
  openRigs.push(handle);
  return handle;
}

function call(port, {method = 'GET', path, headers = {}, body}) {
  return new Promise((resolve, reject) => {
    const req = http.request({host: '127.0.0.1', port, method, path, headers, agent: false}, res => {
      const chunks = [];
      res.on('data', c => chunks.push(c));
      res.on('end', () => resolve({status: res.statusCode, headers: res.headers, body: Buffer.concat(chunks).toString('utf8')}));
    });
    req.on('error', reject);
    req.end(body);
  });
}
const auth = token => ({authorization: `Bearer ${token}`});
const json = {'content-type': 'application/json'};
const register = (r, body, {token = TOKEN_A, headers = {}} = {}) => call(r.port, {method: 'POST', path: '/v1/registrations',
  headers: {...auth(token), ...json, ...headers}, body: typeof body === 'string' ? body : JSON.stringify(body)});
const del = (r, installation, token = TOKEN_A, query = '') => call(r.port, {method: 'DELETE', path: `/v1/registrations/${installation}${query}`, headers: auth(token)});
const request = (over = {}) => ({device_id: NODE_A, installation_id: 'inst-1', platform: 'android', token: TOKEN_ANDROID_1, ...over});

test('el alta registra la instalación y responde lo que valida el contrato, sin el token', async () => {
  const r = await rig();
  const res = await register(r, request());
  assert.equal(res.status, 200);
  assert.equal(res.headers['content-type'], 'application/json');
  assert.deepEqual(decodeNotificationRegistrationResponse(res.body, NODE_A), {device_id: NODE_A, installation_id: 'inst-1', status: 'registered'});
  assert.ok(!res.body.includes(TOKEN_ANDROID_1));
  assert.equal(r.registry.get(NODE_A, 'inst-1').token, TOKEN_ANDROID_1);
  // Ya está en disco al responder.
  const file = JSON.parse(readFileSync(join(r.t.dir, 'registrations.json'), 'utf8'));
  assert.equal(file.registrations.length, 1);
});

test('renovar con otro token sustituye al propio y no consume capacidad', async () => {
  const r = await rig();
  await register(r, request());
  const renewed = await register(r, request({token: TOKEN_ANDROID_2}));
  assert.equal(renewed.status, 200);
  assert.deepEqual(r.registry.forDevice(NODE_A).map(x => x.token), [TOKEN_ANDROID_2]);
  assert.equal(r.registry.size, 1);
  // Se renueva el plazo de 30 días.
  r.c.advance(REGISTRATION_TTL_MS - 1000);
  assert.equal((await register(r, request({token: TOKEN_ANDROID_2}))).status, 200);
  r.c.advance(REGISTRATION_TTL_MS - 1);
  assert.ok(r.registry.get(NODE_A, 'inst-1'));
});

test('la identidad cruzada se rechaza con 403 y no registra nada', async () => {
  const r = await rig();
  const cross = await register(r, request({device_id: NODE_B}));          // cuerpo de otro nodo
  assert.equal(cross.status, 403);
  assert.equal(cross.body, '');
  assert.equal(r.registry.size, 0);
  assert.equal((await register(r, request({device_id: NODE_A}), {token: TOKEN_B})).status, 403); // credencial de B, cuerpo de A
  assert.equal(r.registry.size, 0);
  assert.equal((await register(r, request({device_id: NODE_B}), {token: TOKEN_B})).status, 200); // B registra lo suyo
  assert.equal(r.registry.forDevice(NODE_A).length, 0);
});

test('sin credencial válida responde 401', async () => {
  const r = await rig();
  for (const headers of [{}, auth('FICTIONAL-WRONG'), {authorization: TOKEN_A}]) {
    const res = await call(r.port, {method: 'POST', path: '/v1/registrations', headers: {...json, ...headers}, body: JSON.stringify(request())});
    assert.equal(res.status, 401);
  }
  assert.equal(r.registry.size, 0);
});

test('la capacidad de 32 instalaciones se agota con 409 y una baja libera plaza', async () => {
  const r = await rig();
  for (let i = 0; i < MAX_REGISTRATIONS; ++i)
    assert.equal((await register(r, request({installation_id: `inst-${i}`, token: `FICTIONAL-TOKEN-${i}`}))).status, 200);
  const full = await register(r, request({installation_id: 'inst-extra'}));
  assert.equal(full.status, 409);
  assert.equal(full.body, '');
  assert.equal((await register(r, request({installation_id: 'inst-3', token: 'FICTIONAL-RENEWED'}))).status, 200); // renovar sí
  assert.equal((await del(r, 'inst-4')).status, 200);
  assert.equal((await register(r, request({installation_id: 'inst-extra'}))).status, 200);
});

test('la entrada inválida responde 400 sin guardar nada ni repetir el contenido', async () => {
  const r = await rig();
  const bad = [
    '{no es json', '', '[]', 'null',
    request({platform: 'ios'}), request({installation_id: 'con espacios'}), request({installation_id: ''}),
    request({token: ''}), request({token: 'con espacios'}), request({token: 'a'.repeat(2049)}),
    {...request(), extra: 'FICTIONAL-EXTRA'}, {device_id: NODE_A, installation_id: 'inst-1', platform: 'android'},
    request({token: 'tab\there'}),
  ];
  for (const body of bad) {
    const res = await register(r, body);
    assert.equal(res.status, 400, typeof body === 'string' ? body : JSON.stringify(body).slice(0, 80));
    assert.equal(res.body, '');
  }
  // El tipo de contenido debe ser JSON.
  assert.equal((await register(r, request(), {headers: {'content-type': 'text/plain'}})).status, 400);
  assert.equal((await call(r.port, {method: 'POST', path: '/v1/registrations', headers: auth(TOKEN_A), body: JSON.stringify(request())})).status, 400);
  assert.equal(r.registry.size, 0);
  assert.ok(!JSON.stringify(r.log.entries).includes('FICTIONAL'));
});

test('un cuerpo de más de 4096 bytes responde 413, declarado o transmitido', async () => {
  const r = await rig();
  const big = JSON.stringify(request({token: 'a'.repeat(5000)}));
  assert.equal((await register(r, big)).status, 413);
  // Sin Content-Length (transmisión por trozos) el límite también se aplica.
  const chunked = await call(r.port, {method: 'POST', path: '/v1/registrations',
    headers: {...auth(TOKEN_A), ...json, 'transfer-encoding': 'chunked'}, body: big});
  assert.equal(chunked.status, 413);
  // 4096 exactos caben; un token de 2048 bytes es el máximo admitido.
  assert.equal((await register(r, request({token: 'a'.repeat(2048)}))).status, 200);
  assert.equal((await register(r, request({installation_id: 'inst-2', token: 'a'.repeat(2049)}))).status, 400);
  assert.equal(r.registry.size, 1);
});

test('sin proveedor push configurado el registro responde 503 y no guarda tokens', async () => {
  const r = await rig({configured: false});
  const res = await register(r, request());
  assert.equal(res.status, 503);
  assert.equal(res.body, '');
  assert.equal(r.registry.size, 0);
  // La baja sigue siendo posible e idempotente.
  assert.equal((await del(r, 'inst-1')).status, 200);
});

test('la baja es idempotente y solo actúa dentro del nodo autenticado', async () => {
  const r = await rig();
  await register(r, request({installation_id: 'compartida', token: TOKEN_ANDROID_1}));
  await register(r, request({device_id: NODE_B, installation_id: 'compartida', token: TOKEN_ANDROID_2}), {token: TOKEN_B});
  const removed = await del(r, 'compartida', TOKEN_A);
  assert.equal(removed.status, 200);
  assert.deepEqual(decodeNotificationUnregistrationResponse(removed.body, NODE_A), {device_id: NODE_A, installation_id: 'compartida', status: 'removed'});
  assert.equal(r.registry.get(NODE_A, 'compartida'), null);
  assert.equal(r.registry.get(NODE_B, 'compartida').token, TOKEN_ANDROID_2, 'la instalación del otro nodo no se toca');
  // Repetida o inexistente: misma respuesta, sin revelar nada de otros nodos.
  for (const id of ['compartida', 'no-existe']) {
    const again = await del(r, id, TOKEN_A);
    assert.equal(again.status, 200);
    assert.deepEqual(JSON.parse(again.body), {device_id: NODE_A, installation_id: id, status: 'removed'});
  }
  assert.ok(r.registry.get(NODE_B, 'compartida'));
  assert.equal((await del(r, 'compartida', 'FICTIONAL-WRONG')).status, 401);
  // Persistido.
  const file = JSON.parse(readFileSync(join(r.t.dir, 'registrations.json'), 'utf8'));
  assert.deepEqual(file.registrations.map(x => x.device_id), [NODE_B]);
});

test('la baja rechaza consulta, cuerpo e identificadores inválidos', async () => {
  const r = await rig();
  await register(r, request());
  assert.equal((await del(r, 'inst-1', TOKEN_A, '?device_id=node-other')).status, 400);
  const withBody = await call(r.port, {method: 'DELETE', path: '/v1/registrations/inst-1', headers: {...auth(TOKEN_A), 'content-length': '2'}, body: '{}'});
  assert.equal(withBody.status, 400);
  assert.ok(r.registry.get(NODE_A, 'inst-1'), 'ninguna baja rechazada borra el registro');
  for (const id of ['a%20b', 'a/b', '', 'x'.repeat(65)])
    assert.equal((await del(r, id)).status, 404, id);
});

test('ninguna respuesta ni registro contiene tokens', async () => {
  const r = await rig();
  const responses = [await register(r, request()), await register(r, request({token: TOKEN_ANDROID_2})), await del(r, 'inst-1'),
    await register(r, request({device_id: NODE_B})), await register(r, '{roto')];
  for (const res of responses)
    for (const secret of [TOKEN_ANDROID_1, TOKEN_ANDROID_2, TOKEN_A]) assert.ok(!res.body.includes(secret) && !JSON.stringify(res.headers).includes(secret));
  assert.ok(!JSON.stringify(r.log.entries).includes(TOKEN_ANDROID_1));
});

test('si el registro no se pudo guardar en disco no se confirma al cliente', async () => {
  const r = await rig();
  r.registry.file = join(r.t.dir, 'no-existe', 'registrations.json'); // la escritura fallará
  const res = await register(r, request());
  assert.equal(res.status, 500);
  assert.equal(res.body, '');
  assert.ok(r.log.entries.some(e => e.msg === 'registrations_save_failed'));
  assert.ok(!JSON.stringify(r.log.entries).includes(TOKEN_ANDROID_1));
});

test('un cuerpo que supera 4096 bytes se rechaza en cuanto se excede, sin esperar a que termine la transmisión', async () => {
  const r = await rig();
  const status = await new Promise((resolve, reject) => {
    const req = http.request({host: '127.0.0.1', port: r.port, method: 'POST', path: '/v1/registrations', agent: false,
      headers: {...auth(TOKEN_A), ...json, 'transfer-encoding': 'chunked'}}, res => { res.resume(); resolve(res.statusCode); });
    req.on('error', reject);
    req.write('x'.repeat(5000)); // no se llama a end(): el cliente sigue «transmitiendo»
    setTimeout(() => { req.destroy(); reject(new Error('el servidor siguió esperando el cuerpo')); }, 2000);
  });
  assert.equal(status, 413);
});
