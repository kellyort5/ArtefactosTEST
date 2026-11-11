import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile, readdir} from 'node:fs/promises';
import {Ajv2020} from 'ajv/dist/2020.js';
import addFormats from 'ajv-formats';
import * as notifications from '../dist/src/notifications.js';

const fixtures = new URL('../fixtures/v2/notifications/', import.meta.url);
const load = async name => JSON.parse(await readFile(new URL(name, fixtures), 'utf8'));
const node = 'node-example';
const origin = await load('origin-reference.json');
const registration = await load('registration-request.valid.json');
const status = await load('status-response.valid.json');
const push = await load('push.valid.json');
const decoders = {
  'registration-request': notifications.decodeNotificationRegistrationRequest,
  'registration-response': notifications.decodeNotificationRegistrationResponse,
  'unregistration-request': notifications.decodeNotificationUnregistrationRequest,
  'unregistration-response': notifications.decodeNotificationUnregistrationResponse,
  'status-request': notifications.decodeNotificationStatusRequest,
  'status-response': notifications.decodeNotificationStatusResponse,
  push: (input, device) => notifications.decodePushNotification(input, device, origin),
};
const sanitized = error => error instanceof notifications.NotificationContractError
  && Object.keys(error).sort().join(',') === 'code,name'
  && error.message === error.code && !JSON.stringify(error).includes('FICTIONAL');

for (const file of await readdir(fixtures)) {
  if (!file.endsWith('.json') || file.startsWith('origin-')) continue;
  const kind = Object.keys(decoders).find(key => file.startsWith(key + '.'));
  test(`notification fixture: ${file}`, async () => {
    assert.ok(kind, `No decoder for ${file}`);
    const input = await readFile(new URL(file, fixtures));
    if (file.endsWith('.valid.json')) assert.deepEqual(decoders[kind](input, node), JSON.parse(input));
    else assert.throws(() => decoders[kind](input, node), sanitized);
  });
}

test('standalone closed schemas accept valid body fixtures and reject secret fields at every depth', async () => {
  const ajv = new Ajv2020({strict: true});
  addFormats(ajv, {mode: 'full'});
  for (const kind of Object.keys(decoders)) {
    const schema = JSON.parse(await readFile(new URL(`../schemas/v2/notification-${kind}.schema.json`, import.meta.url)));
    const validate = ajv.compile(schema);
    const body = await load(`${kind}.valid.json`);
    assert.equal(validate(body), true, kind);
    function check(object) {
      for (const key of ['token', 'bearer', 'password', 'chat_id', 'url']) {
        if (Object.hasOwn(object, key)) continue;
        object[key] = 'FICTIONAL-SECRET';
        assert.equal(validate(body), false, `${kind}: ${key}`);
        assert.throws(() => decoders[kind](JSON.stringify(body), node), sanitized);
        delete object[key];
      }
      for (const child of Object.values(object)) if (child && typeof child === 'object') check(child);
    }
    check(body);
  }
});

test('registration uses its own 4096-byte body and 2048-byte token limits', () => {
  const decode = text => notifications.decodeNotificationRegistrationRequest(text, node);
  const maximumToken = {...registration, token: 'F'.repeat(2048)};
  const text = JSON.stringify(maximumToken);
  assert.ok(Buffer.byteLength(text) > 2048);
  assert.equal(decode(text).token.length, 2048);
  assert.throws(() => decode(JSON.stringify({...registration, token: 'F'.repeat(2049)})), sanitized);
  assert.equal(decode(text + ' '.repeat(4096 - Buffer.byteLength(text))).device_id, node);
  assert.throws(() => decode(text + ' '.repeat(4097 - Buffer.byteLength(text))), error => sanitized(error) && error.code === 'notification_too_large');
  const unicodeToken = {...registration, token: 'é'.repeat(1024)};
  assert.equal(decode(JSON.stringify(unicodeToken)).token, unicodeToken.token);
  assert.throws(() => decode(JSON.stringify({...registration, token: 'é'.repeat(1025)})), error => sanitized(error) && error.code === 'token_too_large');
});

test('strict UTF-8, malformed JSON, BOM, surrogate escapes and invalid root types are sanitized', () => {
  for (const decode of Object.values(decoders)) {
    for (const input of [new Uint8Array([0xff]), new Uint8Array([0xc0, 0xaf]), '{', '\ufeff{}', '{}\ud800', '{"token":"\\ud800"}', 'null', '[]', 'true', '"FICTIONAL"', undefined]) {
      assert.throws(() => decode(input, node), sanitized);
    }
  }
  assert.equal(notifications.decodeNotificationRegistrationRequest(JSON.stringify({...registration, token: '😀'}), node).token, '😀');
});

test('identifiers are 1..64 allowed ASCII characters, including no trailing newline', () => {
  for (const key of ['device_id', 'installation_id']) {
    for (const id of ['', 'a'.repeat(65), 'é', 'node/example', 'node example', 'node\n', 'node\0']) {
      assert.throws(() => notifications.decodeNotificationRegistrationRequest(JSON.stringify({...registration, [key]: id}), key === 'device_id' ? id : node), sanitized);
    }
    const id = 'A._:-09'.padEnd(64, 'x');
    assert.equal(notifications.decodeNotificationRegistrationRequest(JSON.stringify({...registration, [key]: id}), key === 'device_id' ? id : node)[key], id);
  }
  for (const authorized of ['', 'node\n', undefined, null, 7]) {
    assert.throws(() => notifications.decodeNotificationRegistrationRequest(JSON.stringify(registration), authorized), sanitized);
  }
});

test('all operations require explicit node context, including removal and status', async () => {
  for (const [kind, decode] of Object.entries(decoders)) {
    const raw = JSON.stringify(await load(`${kind}.valid.json`));
    assert.throws(() => decode(raw, 'node-other'), error => sanitized(error) && error.code === 'unauthorized_device');
    assert.throws(() => decode(raw), sanitized);
  }
});

test('push preserves original node/message identity and severity without leaking source details', () => {
  const source = {...origin, payload: {...origin.payload, source_ip: '192.0.2.1', token: 'FICTIONAL-SOURCE-SECRET'}};
  const built = notifications.makePushNotification(source, node);
  assert.deepEqual(built, push);
  assert.equal(built.data.event_id, source.message_id);
  assert.equal(JSON.stringify(built).includes('FICTIONAL-SOURCE'), false);
  assert.deepEqual(notifications.decodePushNotification(JSON.stringify(built), node, source), built);
  for (const altered of [{...origin, message_id: 'other-event'}, {...origin, payload: {...origin.payload, severity: 'critical'}}]) {
    assert.throws(() => notifications.decodePushNotification(JSON.stringify(built), node, altered), error => sanitized(error) && error.code === 'event_identity_mismatch');
  }
  for (const altered of [{...origin, device_id: 'other-node'}, {...origin, message_id: 'x\n'}, {...origin, payload: {...origin.payload, name: 'system.status'}}, null]) {
    assert.throws(() => notifications.makePushNotification(altered, node), sanitized);
    assert.throws(() => notifications.decodePushNotification(JSON.stringify(built), node, altered), sanitized);
  }
});

test('push fixture correlates to a complete original event accepted by the v2 schema', async () => {
  const ajv = new Ajv2020({strict: true});
  addFormats(ajv, {mode: 'full'});
  const common = JSON.parse(await readFile(new URL('../schemas/v2/common.schema.json', import.meta.url)));
  const eventSchema = JSON.parse(await readFile(new URL('../schemas/v2/event.schema.json', import.meta.url)));
  ajv.addSchema(common);
  const validate = ajv.compile(eventSchema);
  const source = await load('origin-event.json');
  assert.equal(validate(source), true, JSON.stringify(validate.errors));
  assert.ok(Buffer.byteLength(JSON.stringify(source)) <= 2048);
  assert.equal(source.message_id, origin.message_id);
  assert.equal(source.device_id, origin.device_id);
  assert.deepEqual(notifications.makePushNotification(source, node), push);
  assert.deepEqual(notifications.decodePushNotification(JSON.stringify(push), node, source), push);
});

test('status distinguishes provider acceptance and unknown reading; errors and counters stay bounded', () => {
  const decode = body => notifications.decodeNotificationStatusResponse(JSON.stringify(body), node);
  assert.equal(decode(status).channels.webhook.last_delivery.read_status, 'unknown');
  for (const change of [
    channel => {channel.last_delivery.read_status = 'read';},
    channel => {channel.last_delivery.status = 'delivered';},
    channel => {channel.last_delivery.error = 'FICTIONAL-provider-secret';},
    channel => {channel.last_delivery.timestamp = '2026-02-30T12:00:00Z';},
    channel => {channel.last_delivery.timestamp += '\n';},
    channel => {channel.pending = -1;},
    channel => {channel.pending = 0.1;},
    channel => {channel.suppressed = Number.MAX_SAFE_INTEGER + 1;},
  ]) {
    const body = structuredClone(status); change(body.channels.webhook);
    assert.throws(() => decode(body), sanitized);
  }
  const failed = structuredClone(status); failed.channels.telegram.last_delivery.error = null;
  assert.throws(() => decode(failed), sanitized);
});

test('HTTP fixtures specify create/renew/delete/status and cross-node rejection without operating a server', async () => {
  for (const file of (await readdir(fixtures)).filter(name => name.endsWith('.http'))) {
    const transcript = await readFile(new URL(file, fixtures), 'utf8');
    const [request, response] = transcript.split('\nHTTP/1.1 ');
    const [requestHeaders, requestBody] = request.split('\n\n');
    const [method, path] = requestHeaders.split('\n')[0].split(' ');
    assert.match(requestHeaders, /Authorization: Bearer FICTIONAL-/);
    let validateRequest;
    if (method === 'POST') validateRequest = () => notifications.decodeNotificationRegistrationRequest(requestBody, node);
    if (method === 'DELETE') validateRequest = () => notifications.decodeNotificationUnregistrationRequest(JSON.stringify({device_id: node, installation_id: path.split('/').at(-1)}), node);
    if (method === 'GET') validateRequest = () => notifications.decodeNotificationStatusRequest(JSON.stringify({device_id: new URL(path, 'https://example.invalid').searchParams.get('device_id')}), node);
    assert.ok(validateRequest);
    if (file.endsWith('.invalid.http')) {assert.throws(validateRequest, sanitized); continue;}
    validateRequest();
    assert.match(response, /^200\n/);
    const body = response.split('\n\n')[1];
    const decodeResponse = method === 'POST' ? notifications.decodeNotificationRegistrationResponse : method === 'DELETE' ? notifications.decodeNotificationUnregistrationResponse : notifications.decodeNotificationStatusResponse;
    decodeResponse(body, node);
    assert.equal(body.includes('TOKEN'), false);
  }
});
