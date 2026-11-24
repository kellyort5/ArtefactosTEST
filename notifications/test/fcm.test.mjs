import assert from 'node:assert/strict';
import {generateKeyPairSync} from 'node:crypto';
import {chmodSync, writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {test} from 'node:test';
import {CredentialError, createFcmProvider, credentialPermissionWarning, mapFcmError, parseCredential} from '../src/fcm.js';
import {TOKEN_ANDROID_1, tempDir} from './helpers.mjs';

const push = {notification: {title: 'Sentinel Node', body: 'Actividad detectada en Cebo. Abre la app para consultar el incidente.'},
  data: {device_id: 'node-example', event_id: 'boot-1:5', severity: 'warning', route: 'cebo'}};

// Cuenta de servicio ficticia con una clave RSA efímera generada para la prueba: no es utilizable
// contra ningún proyecto y no se escribe en el repositorio.
function fictionalServiceAccount() {
  const {privateKey} = generateKeyPairSync('rsa', {modulusLength: 2048, privateKeyEncoding: {type: 'pkcs8', format: 'pem'}, publicKeyEncoding: {type: 'spki', format: 'pem'}});
  return {type: 'service_account', project_id: 'fictional-project', private_key_id: 'FICTIONAL',
    private_key: privateKey, client_email: 'fictional@fictional-project.iam.gserviceaccount.com', client_id: '0'};
}

test('los errores del SDK se reducen a los códigos del contrato', () => {
  const cases = {
    'messaging/registration-token-not-registered': {error: 'invalid_registration', drop: true, remove: true},
    'messaging/invalid-registration-token': {error: 'invalid_registration', drop: true, remove: true},
    'messaging/invalid-argument': {error: 'provider_rejected', drop: true},
    'messaging/payload-size-limit-exceeded': {error: 'provider_rejected', drop: true},
    'messaging/message-rate-exceeded': {error: 'rate_limited'},
    'messaging/device-message-rate-exceeded': {error: 'rate_limited'},
    'messaging/authentication-error': {error: 'provider_rejected'},
    'messaging/mismatched-credential': {error: 'provider_rejected'},
    'messaging/third-party-auth-error': {error: 'provider_rejected'},
    'messaging/server-unavailable': {error: 'provider_unavailable'},
    'messaging/internal-error': {error: 'provider_unavailable'},
    'messaging/unknown-error': {error: 'provider_unavailable'},
    'app/network-error': {error: 'provider_unavailable'},
    'algo/desconocido': {error: 'provider_unavailable'},
  };
  for (const [code, expected] of Object.entries(cases)) {
    const result = mapFcmError({code, message: `FICTIONAL-SECRET ${TOKEN_ANDROID_1}`});
    assert.equal(result.ok, false, code);
    assert.equal(result.error, expected.error, code);
    assert.equal(Boolean(result.drop), Boolean(expected.drop), code);
    assert.equal(Boolean(result.remove), Boolean(expected.remove), code);
    assert.ok(!JSON.stringify(result).includes('FICTIONAL'), 'el mensaje del proveedor no se conserva');
  }
  // La forma `errorInfo.code` del SDK también se reconoce; un error sin código es «no disponible».
  assert.equal(mapFcmError({errorInfo: {code: 'messaging/registration-token-not-registered'}}).remove, true);
  assert.equal(mapFcmError(new Error('boom')).error, 'provider_unavailable');
  assert.equal(mapFcmError(undefined).error, 'provider_unavailable');
});

test('el mensaje a FCM es notification+data del contrato, con prioridad alta y vida limitada', async () => {
  const calls = [];
  const provider = createFcmProvider({messaging: {send: async message => { calls.push(message); return 'projects/p/messages/FICTIONAL'; }}, ttlMs: 120000});
  assert.equal(provider.configured, true);
  assert.deepEqual(await provider.send({token: TOKEN_ANDROID_1, push}), {ok: true}); // aceptado, nunca «leído»
  assert.deepEqual(calls, [{token: TOKEN_ANDROID_1, notification: push.notification, data: push.data,
    android: {priority: 'high', ttl: 120000}}]);
});

test('un error del proveedor nunca devuelve su mensaje ni el token', async () => {
  const provider = createFcmProvider({messaging: {send: async () => {
    throw Object.assign(new Error(`Token ${TOKEN_ANDROID_1} FICTIONAL-SECRET`), {code: 'messaging/internal-error'}); }}});
  const result = await provider.send({token: TOKEN_ANDROID_1, push});
  assert.deepEqual(result, {ok: false, error: 'provider_unavailable'});
});

test('una credencial válida se carga con el SDK oficial sin conectar a la red', () => {
  const t = tempDir();
  try {
    const file = join(t.dir, 'sa.json');
    writeFileSync(file, JSON.stringify(fictionalServiceAccount()));
    const provider = createFcmProvider({credentialFile: file, appName: 'test-valid'});
    assert.equal(provider.configured, true);
    assert.equal(provider.problem, null);
  } finally { t.cleanup(); }
});

test('una credencial inválida deja el canal sin configurar y su mensaje nombra el campo, no el valor', () => {
  const t = tempDir();
  try {
    const good = fictionalServiceAccount();
    const secret = good.private_key;
    const variants = {
      'no es JSON': 'no json {{{',
      'type': JSON.stringify({...good, type: 'authorized_user'}),
      'project_id': JSON.stringify({...good, project_id: ''}),
      'client_email': JSON.stringify({...good, client_email: undefined}),
      'private_key': JSON.stringify({...good, private_key: 'FICTIONAL-NOT-A-KEY'}),
    };
    let n = 0;
    for (const [problem, text] of Object.entries(variants)) {
      const file = join(t.dir, `sa-${n}.json`);
      writeFileSync(file, text);
      const provider = createFcmProvider({credentialFile: file, appName: `test-invalid-${n++}`});
      assert.equal(provider.configured, false, problem);
      assert.ok(provider.problem.includes(problem), `${problem}: ${provider.problem}`);
      assert.ok(!provider.problem.includes(secret.slice(40, 80)) && !provider.problem.includes('FICTIONAL-NOT'));
    }
    // Un archivo inexistente tampoco filtra la ruta ni lanza.
    const missing = createFcmProvider({credentialFile: join(t.dir, 'no-existe.json'), appName: 'test-missing'});
    assert.equal(missing.configured, false);
    assert.ok(!missing.problem.includes(t.dir));
    assert.throws(() => parseCredential('x'), CredentialError);
  } finally { t.cleanup(); }
});

test('sin proveedor utilizable el envío falla como error interno sin lanzar', async () => {
  const provider = createFcmProvider({credentialFile: '/no/existe.json', appName: 'test-none'});
  assert.deepEqual(await provider.send({token: TOKEN_ANDROID_1, push}), {ok: false, error: 'internal_error'});
});

test('avisa si el archivo de credencial es legible por otros usuarios (POSIX)', {skip: process.platform === 'win32' ? 'permisos POSIX' : false}, () => {
  const t = tempDir();
  try {
    const file = join(t.dir, 'sa.json');
    writeFileSync(file, '{}');
    chmodSync(file, 0o644);
    assert.match(credentialPermissionWarning(file), /0600/);
    chmodSync(file, 0o600);
    assert.equal(credentialPermissionWarning(file), null);
    assert.equal(credentialPermissionWarning(join(t.dir, 'no-existe')), null);
  } finally { t.cleanup(); }
});
