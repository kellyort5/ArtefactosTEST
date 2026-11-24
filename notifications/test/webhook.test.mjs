import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync, writeFileSync} from 'node:fs';
import https from 'node:https';
import net from 'node:net';
import {join} from 'node:path';
import {test} from 'node:test';
import {classify, createWebhook} from '../src/webhook.js';
import {NODE_A, WEBHOOK_SECRET, tempDir, webhookServer} from './helpers.mjs';

const alert = {schema: 'sentinel.alert.v1', device_id: NODE_A, event_id: 'boot-1:5', severity: 'warning',
  name: 'cebo.interaction', origin: 'real', details: {source_ipv4: '192.0.2.2'}};
const config = (port, extra = {}) => ({url: `http://127.0.0.1:${port}/hook?x=1`, timeoutMs: 1000,
  authHeader: {name: 'X-Api-Key', value: WEBHOOK_SECRET}, caFile: null, ...extra});

test('clasifica los estados HTTP en los códigos del contrato', () => {
  assert.deepEqual(classify(200), {ok: true});
  assert.deepEqual(classify(204), {ok: true});
  assert.deepEqual(classify(429, '7'), {ok: false, error: 'rate_limited', retryAfterMs: 7000});
  assert.deepEqual(classify(429, 'mañana'), {ok: false, error: 'rate_limited', retryAfterMs: 0});
  for (const status of [408, 500, 502, 503, 504]) assert.equal(classify(status).error, 'provider_unavailable');
  for (const status of [301, 302, 400, 401, 403, 404, 410]) assert.equal(classify(status).error, 'provider_rejected');
});

test('envía la alerta correlacionada con el cuerpo, la identidad y la autenticación propia', async () => {
  const server = await webhookServer();
  const hook = createWebhook(config(server.port));
  assert.deepEqual(await hook.send(alert, 3), {ok: true});
  hook.close();
  const [request] = server.requests;
  assert.equal(request.method, 'POST');
  assert.equal(request.url, '/hook?x=1');
  assert.deepEqual(request.json, alert);
  assert.equal(request.headers['content-type'], 'application/json');
  assert.equal(request.headers['idempotency-key'], `${NODE_A}/boot-1:5`);
  assert.equal(request.headers['x-sentinel-attempt'], '3');
  assert.equal(request.headers['x-api-key'], WEBHOOK_SECRET);
  assert.match(request.headers['user-agent'], /^sentinel-notifications\//);
  await server.close();
});

test('los fallos del destino se reducen a códigos sin mensajes, URL ni secretos', async () => {
  for (const [status, error] of [[500, 'provider_unavailable'], [503, 'provider_unavailable'], [401, 'provider_rejected'],
    [404, 'provider_rejected'], [429, 'rate_limited']]) {
    const server = await webhookServer(() => ({status, body: `FICTIONAL-RESPONSE-BODY ${WEBHOOK_SECRET}`}));
    const hook = createWebhook(config(server.port));
    const result = await hook.send(alert, 1);
    assert.equal(result.ok, false);
    assert.equal(result.error, error);
    assert.ok(!JSON.stringify(result).includes('FICTIONAL'), 'no se devuelve nada de la respuesta');
    assert.ok(!JSON.stringify(result).includes(WEBHOOK_SECRET));
    hook.close();
    await server.close();
  }
});

test('no sigue redirecciones: no reenvía la autenticación a otro destino', async () => {
  const other = await webhookServer();
  const server = await webhookServer(() => ({status: 302, headers: {location: `http://127.0.0.1:${other.port}/robado`}}));
  const hook = createWebhook(config(server.port));
  assert.deepEqual(await hook.send(alert, 1), {ok: false, error: 'provider_rejected'});
  assert.equal(other.requests.length, 0);
  hook.close();
  await Promise.all([server.close(), other.close()]);
});

test('un destino que no responde agota el tiempo y se informa como no disponible', async () => {
  const server = await webhookServer(() => ({hang: true}));
  const hook = createWebhook(config(server.port, {timeoutMs: 150}));
  const started = Date.now();
  assert.deepEqual(await hook.send(alert, 1), {ok: false, error: 'provider_unavailable'});
  assert.ok(Date.now() - started < 1500);
  hook.close();
  await server.close();
});

test('un destino inalcanzable se informa como no disponible sin detalles de red', async () => {
  const probe = net.createServer();
  await new Promise(resolve => probe.listen(0, '127.0.0.1', resolve));
  const {port} = probe.address();
  await new Promise(resolve => probe.close(resolve)); // puerto cerrado: conexión rechazada
  const hook = createWebhook(config(port));
  const result = await hook.send(alert, 1);
  assert.deepEqual(result, {ok: false, error: 'provider_unavailable'});
  hook.close();
});

test('una respuesta enorme se descarta sin acumularse', async () => {
  const server = await webhookServer(() => ({status: 200, body: 'x'.repeat(2 * 1024 * 1024)}));
  const hook = createWebhook(config(server.port));
  assert.deepEqual(await hook.send(alert, 1), {ok: true});
  hook.close();
  await server.close();
});

test('rechaza enviar una alerta que supere el tamaño acotado', async () => {
  const server = await webhookServer();
  const hook = createWebhook(config(server.port));
  assert.deepEqual(await hook.send({...alert, details: {pad: 'x'.repeat(20000)}}, 1), {ok: false, error: 'internal_error'});
  assert.equal(server.requests.length, 0);
  hook.close();
  await server.close();
});

// --- TLS real: requiere el binario `openssl`; se omite si no está -----------------
const openssl = spawnSync('openssl', ['version'], {encoding: 'utf8'});
const tls = {skip: openssl.status === 0 ? false : 'openssl no disponible'};

function selfSigned(dir) {
  const key = join(dir, 'key.pem'), cert = join(dir, 'cert.pem');
  const run = spawnSync('openssl', ['req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', key, '-out', cert,
    '-days', '1', '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost,IP:127.0.0.1'], {encoding: 'utf8'});
  assert.equal(run.status, 0, 'no se pudo generar el certificado de prueba');
  return {key, cert};
}
async function tlsReceiver(files, onRequest) {
  const server = https.createServer({key: readFileSync(files.key), cert: readFileSync(files.cert)}, (req, res) => {
    onRequest(req); res.writeHead(200); res.end('ok');
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  return {port: server.address().port, close: () => new Promise(resolve => { server.close(resolve); server.closeAllConnections(); })};
}

test('HTTPS verifica el certificado: uno autofirmado se rechaza salvo que su CA esté configurada', tls, async () => {
  const t = tempDir();
  try {
    const files = selfSigned(t.dir);
    let seen = 0;
    const receiver = await tlsReceiver(files, () => { seen++; });
    const url = `https://localhost:${receiver.port}/hook`;
    // Sin la CA: el certificado no se confía y la alerta no llega.
    const untrusted = createWebhook({url, timeoutMs: 2000, authHeader: null, caFile: null});
    assert.deepEqual(await untrusted.send(alert, 1), {ok: false, error: 'provider_unavailable'});
    assert.equal(seen, 0, 'nunca se envía el cuerpo a un destino no verificado');
    untrusted.close();
    // Con la CA propia configurada (destino en red privada) sí se entrega.
    const trusted = createWebhook({url, timeoutMs: 2000, authHeader: {name: 'X-Api-Key', value: WEBHOOK_SECRET}, caFile: files.cert});
    assert.deepEqual(await trusted.send(alert, 1), {ok: true});
    assert.equal(seen, 1);
    trusted.close();
    // Nombre distinto del certificado: también se rechaza.
    const wrongName = createWebhook({url: `https://127.0.0.2:${receiver.port}/hook`, timeoutMs: 2000, authHeader: null, caFile: files.cert});
    const result = await wrongName.send(alert, 1);
    assert.equal(result.ok, false);
    wrongName.close();
    await receiver.close();
  } finally { t.cleanup(); }
});
