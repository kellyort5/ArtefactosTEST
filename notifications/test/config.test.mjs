import assert from 'node:assert/strict';
import {test} from 'node:test';
import {ConfigError, parseConfig} from '../src/config.js';
import {NODE_A, NODE_B, TOKEN_A, TOKEN_B, WEBHOOK_SECRET, sha256hex} from './helpers.mjs';

const valid = () => ({
  mqtt: {url: 'mqtt://192.0.2.10:1883'},
  nodes: [{device_id: NODE_A, token_sha256: sha256hex(TOKEN_A)}],
  webhook: {url: 'https://hooks.example.invalid/sentinel', auth_header: {name: 'X-Api-Key', value: WEBHOOK_SECRET}},
  status_api: {tls: {cert_file: './tls/cert.pem', key_file: './tls/key.pem'}, cors_origins: ['https://localhost', 'capacitor://localhost']},
});
const reject = (mutate, expected) => {
  const raw = valid();
  mutate(raw);
  assert.throws(() => parseConfig(raw, {base: '/srv'}), error => {
    assert.ok(error instanceof ConfigError);
    if (expected) assert.match(error.message, expected);
    // Ningún mensaje repite un secreto ni un valor configurado.
    for (const secret of [WEBHOOK_SECRET, TOKEN_A, sha256hex(TOKEN_A), 'hooks.example.invalid'])
      assert.ok(!error.message.includes(secret), 'el mensaje de error filtra un valor');
    return true;
  });
};

test('una configuración válida se normaliza con valores por defecto acotados', () => {
  const config = parseConfig(valid(), {base: '/srv/notify'});
  assert.equal(config.mqtt.clientId, 'sentinel-notifications');
  assert.deepEqual(config.alerts, {minSeverity: 'warning', includeSimulated: false, repeatWindowMs: 30000});
  assert.equal(config.webhook.timeoutMs, 5000);
  assert.equal(config.statusApi.port, 8443);
  assert.deepEqual([...config.statusApi.corsOrigins], ['https://localhost', 'capacitor://localhost']);
  assert.match(config.statusApi.tls.certFile.replace(/\\/g, '/'), /\/srv\/notify\/tls\/cert\.pem$/);
  assert.ok(config.stateDir.replace(/\\/g, '/').endsWith('/srv/notify/state'));
  assert.throws(() => { config.alerts.minSeverity = 'info'; }, TypeError); // inmutable
});

test('el webhook es opcional y su ausencia se representa como canal sin configurar', () => {
  const raw = valid();
  delete raw.webhook;
  assert.equal(parseConfig(raw).webhook, null);
});

test('el secreto del webhook puede venir del entorno y prevalece sobre el archivo', () => {
  const raw = valid();
  delete raw.webhook.auth_header.value;
  assert.equal(parseConfig(raw, {env: {NOTIFY_WEBHOOK_AUTH_VALUE: 'FROM-ENV'}}).webhook.authHeader.value, 'FROM-ENV');
  assert.throws(() => parseConfig(raw), /webhook\.auth_header\.value/);
});

test('rechaza campos desconocidos en cualquier nivel', () => {
  reject(raw => { raw.extra = 1; }, /raíz\.extra/);
  reject(raw => { raw.mqtt.username = 'x'; }, /mqtt\.username/);
  reject(raw => { raw.webhook.method = 'PUT'; }, /webhook\.method/);
  reject(raw => { raw.status_api.cors = '*'; }, /status_api\.cors/);
});

test('el webhook exige HTTPS salvo bucle local con la opción explícita', () => {
  reject(raw => { raw.webhook.url = 'http://hooks.example.invalid/x'; }, /webhook\.url/);
  reject(raw => { raw.webhook.url = 'http://127.0.0.1:9/x'; }, /webhook\.url/); // sin allow_insecure_loopback
  reject(raw => { raw.webhook.url = 'ftp://hooks.example.invalid/x'; }, /webhook\.url/);
  reject(raw => { raw.webhook.allow_insecure_loopback = true; raw.webhook.url = 'http://hooks.example.invalid/x'; }, /bucle local/);
  const ok = valid();
  ok.webhook.allow_insecure_loopback = true;
  ok.webhook.url = 'http://127.0.0.1:9/x';
  assert.equal(parseConfig(ok).webhook.url, 'http://127.0.0.1:9/x');
});

test('rechaza credenciales o fragmentos en la URL del webhook', () => {
  reject(raw => { raw.webhook.url = 'https://user:pass@hooks.example.invalid/x'; }, /sin credenciales/);
  reject(raw => { raw.webhook.url = 'https://hooks.example.invalid/x#frag'; }, /fragmento/);
  reject(raw => { raw.webhook.url = 'no es una url'; }, /URL inválida/);
});

test('valida la cabecera de autenticación del webhook', () => {
  reject(raw => { raw.webhook.auth_header.name = 'Content-Length'; }, /cabecera no permitido/);
  reject(raw => { raw.webhook.auth_header.name = 'Bad Name'; }, /cabecera no permitido/);
  reject(raw => { raw.webhook.auth_header.value = 'a\r\nX-Injected: 1'; }, /sin controles/);
});

test('los nodos exigen ID válido, hash SHA-256 y credenciales distintas', () => {
  reject(raw => { raw.nodes = []; }, /nodes/);
  reject(raw => { raw.nodes[0].device_id = 'bad id'; }, /device_id/);
  reject(raw => { raw.nodes[0].token_sha256 = TOKEN_A; }, /token_sha256/); // no es un hash
  reject(raw => { raw.nodes[0].token_sha256 = sha256hex(TOKEN_A).toUpperCase(); }, /token_sha256/);
  reject(raw => { raw.nodes.push({device_id: NODE_A, token_sha256: sha256hex(TOKEN_B)}); }, /repetido/);
  reject(raw => { raw.nodes.push({device_id: NODE_B, token_sha256: sha256hex(TOKEN_A)}); }, /comparten credencial/);
});

test('la API de estado exige TLS salvo bucle local con la opción explícita', () => {
  reject(raw => { delete raw.status_api.tls; }, /HTTPS obligatorio/);
  reject(raw => { delete raw.status_api.tls; raw.status_api.allow_insecure_loopback = true; raw.status_api.host = '0.0.0.0'; }, /HTTPS obligatorio/);
  reject(raw => { delete raw.status_api.tls.key_file; }, /obligatorios|key_file/);
  const local = valid();
  delete local.status_api.tls;
  local.status_api.allow_insecure_loopback = true;
  local.status_api.host = '127.0.0.1';
  assert.equal(parseConfig(local).statusApi.tls, null);
});

test('CORS solo admite orígenes exactos, nunca comodines', () => {
  reject(raw => { raw.status_api.cors_origins = ['*']; }, /cors_origins/);
  reject(raw => { raw.status_api.cors_origins = ['https://localhost/']; }, /origen exacto/);
  reject(raw => { raw.status_api.cors_origins = ['https://localhost/path']; }, /origen exacto/);
  reject(raw => { raw.status_api.cors_origins = ['null']; }, /cors_origins/);
});

test('los límites numéricos se comprueban', () => {
  reject(raw => { raw.alerts = {min_severity: 'fatal'}; }, /min_severity/);
  reject(raw => { raw.alerts = {repeat_window_ms: -1}; }, /repeat_window_ms/);
  reject(raw => { raw.webhook.timeout_ms = 10; }, /timeout_ms/);
  reject(raw => { raw.status_api.port = 70000; }, /port/);
  reject(raw => { raw.mqtt.url = 'http://192.0.2.10:1883'; }, /mqtt\.url/);
  reject(raw => { raw.mqtt.client_id = 'bad id!'; }, /client_id/);
});

test('el archivo de ejemplo documentado es una configuración válida y solo contiene valores ficticios', async () => {
  const {readFileSync} = await import('node:fs');
  const text = readFileSync(new URL('../config/notifications.example.json', import.meta.url), 'utf8');
  const config = parseConfig(JSON.parse(text), {base: '/srv/notify'});
  assert.equal(config.webhook.url, 'https://hooks.example.invalid/sentinel');
  assert.match(text, /0{64}/);
  assert.equal(config.statusApi.corsOrigins.length, 2);
  assert.ok(!/password|secret|key[_-]?id|BEGIN/i.test(text.replace('NOTIFY_WEBHOOK_AUTH_VALUE', '').replace('key_file', '')));
});

test('la sección push valida la credencial de FCM y la vida del mensaje', () => {
  const raw = valid();
  assert.equal(parseConfig(raw).push, null); // sin credencial el canal queda sin configurar
  raw.push = {credential_file: 'fcm/service-account.json'};
  const config = parseConfig(raw, {base: '/srv/notify'});
  assert.ok(config.push.credentialFile.split('\\').join('/').endsWith('/srv/notify/fcm/service-account.json'));
  assert.equal(config.push.ttlMs, 3600000);
  raw.push.ttl_seconds = 120;
  assert.equal(parseConfig(raw).push.ttlMs, 120000);
  reject(r => { r.push = {}; }, /push\.credential_file/);
  reject(r => { r.push = {credential_file: 'x.json', ttl_seconds: 10}; }, /ttl_seconds/);
  reject(r => { r.push = {credential_file: 'x.json', ttl_seconds: 90000}; }, /ttl_seconds/);
  reject(r => { r.push = {credential_file: 'x.json', project_id: 'p'}; }, /push\.project_id/);
  reject(r => { r.push = {credential_file: 42}; }, /credential_file/);
});

test('el ejemplo documentado incluye push con una ruta ficticia fuera del repositorio', async () => {
  const {readFileSync} = await import('node:fs');
  const config = parseConfig(JSON.parse(readFileSync(new URL('../config/notifications.example.json', import.meta.url), 'utf8')));
  assert.ok(config.push.credentialFile.split('\\').join('/').endsWith('/run/notifications/fcm-service-account.json'));
});
