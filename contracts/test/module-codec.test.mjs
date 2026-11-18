import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFile, readdir } from 'node:fs/promises';
import {
  ContractError, decodeMessage, encodeMessage, decodeMetadata, makeCommand,
  decodeModuleMessage, encodeModuleMessage, decodeModuleMetadata, makeModuleCommand,
  decodeAnyMessage, encodeAnyMessage, decodeAnyMetadata,
  checkModuleCommandAdmission, checkNewModuleCommandContext,
  checkModuleAckForCommand, MODULE_COMMAND_RESULTS,
  mqttTopic, MQTT_POLICY, eventIdentity, fragmentMessage, BleAssembler, BLE_UUIDS,
} from '../dist/src/index.js';

const base = new URL('../fixtures/', import.meta.url);
const commands = new URL('v2/commands/', base);
const fixture = async name => JSON.parse(await readFile(new URL(name, commands), 'utf8'));
const metadata = { version: 2, device_id: 'node-example', boot_id: 'boot-example', revision: 3, uptime_ms: 1000, max_message_bytes: 2048 };
const context = {
  channel: 'ble', authenticated: true, authorized: true, encrypted: true, authorizedDeviceId: 'node-example',
  bootId: 'boot-example', revision: 3, uptimeMs: 1000, profile: 'cebo', targetSelected: true,
};
const invalid = (fn, secret = 'NEVER-ECHO-THIS') => assert.throws(fn, error =>
  error instanceof ContractError && !error.message.includes(secret));

for (const name of (await readdir(commands)).filter(name => name.endsWith('.valid.json'))) {
  test(`command v2: ${name}`, async () => {
    const message = await fixture(name);
    assert.deepEqual(decodeModuleMessage(encodeModuleMessage(message)), message);
    assert.deepEqual(decodeAnyMessage(encodeAnyMessage(message)), message);
  });
}

for (const folder of ['states', 'metrics', 'events', 'acks', 'invalid']) {
  const location = new URL(`v2/${folder}/`, base);
  for (const name of (await readdir(location)).filter(name => /\.(valid|invalid)\.json$/.test(name))) {
    test(`shared ${folder}: ${name}`, async () => {
      const bytes = await readFile(new URL(name, location));
      if (name.endsWith('.invalid.json')) {
        invalid(() => decodeModuleMessage(bytes), 'fiction-only-never-echo');
      } else {
        const message = decodeModuleMessage(bytes);
        assert.deepEqual(decodeModuleMessage(encodeModuleMessage(message)), message);
        assert.ok(encodeModuleMessage(message).byteLength <= 2048);
        if (message.type === 'event') {
          assert.equal(eventIdentity(message), `${message.device_id}/${message.message_id}`);
        }
      }
    });
  }
}

test('v1 entry points and historical event identities retain their original semantics', async () => {
  const historical = JSON.parse(await readFile(new URL('event.valid.json', base), 'utf8'));
  assert.deepEqual(decodeAnyMessage(encodeMessage(historical)), historical);
  assert.equal(eventIdentity(historical), `${historical.device_id}/${historical.message_id}`);
  const metaV1 = { ...metadata, version: 1 };
  assert.equal(decodeMetadata(JSON.stringify(metaV1)).version, 1);
  assert.equal(decodeAnyMetadata(JSON.stringify(metaV1)).version, 1);
  assert.equal(makeCommand(metaV1, { operation: 'get_state', args: {} }, 'legacy-command').version, 1);
  const functional = await fixture('get_state.valid.json');
  assert.throws(() => decodeMessage(JSON.stringify(functional)), { code: 'unsupported_version' });
  assert.throws(() => decodeModuleMessage(JSON.stringify(historical)), { code: 'unsupported_version' });
});

test('v2 event origin is explicit, survives replay and never changes legacy v1 events', async () => {
  const path = new URL('v2/events/cebo.http-benign.valid.json', base);
  const sample = JSON.parse(await readFile(path, 'utf8'));
  for (const origin of ['real', 'simulated']) {
    sample.payload.origin = origin;
    const decoded = decodeAnyMessage(encodeModuleMessage(sample));
    assert.equal(decoded.payload.origin, origin);
    assert.equal(eventIdentity(decoded), `${sample.device_id}/${sample.message_id}`);
  }
  delete sample.payload.origin;
  invalid(() => encodeModuleMessage(sample));
  sample.payload.origin = 'unknown';
  invalid(() => encodeModuleMessage(sample));
  const historical = JSON.parse(await readFile(new URL('event.valid.json', base), 'utf8'));
  assert.equal(Object.hasOwn(decodeAnyMessage(encodeMessage(historical)).payload, 'origin'), false);
});

test('metadata negotiation and command construction preserve version, boot and monotonic TTL', () => {
  assert.equal(decodeModuleMetadata(JSON.stringify(metadata)).version, 2);
  assert.equal(decodeAnyMetadata(JSON.stringify(metadata)).version, 2);
  const message = makeModuleCommand(metadata, { operation: 'escudo.scan', args: {} }, 'scan-job', 100);
  assert.equal(message.payload.command_id, 'scan-job');
  assert.equal(message.payload.expected_revision, 3);
  assert.equal(message.payload.target_boot_id, metadata.boot_id);
  assert.equal(message.payload.expires_at_uptime_ms, 31100);
  for (const elapsed of [-1, 15001, NaN, 0.5]) invalid(() => makeModuleCommand(metadata, { operation: 'get_state', args: {} }, 'id', elapsed));
  invalid(() => decodeModuleMetadata(' '.repeat(257)));
  invalid(() => decodeModuleMetadata(JSON.stringify({ ...metadata, password: 'NEVER-ECHO-THIS' })));
  assert.throws(() => decodeAnyMetadata(JSON.stringify({ ...metadata, version: 999 })), { code: 'unsupported_version' });
});

test('UTF-8 byte limits reject oversized SSIDs and keys independently of schema characters', async () => {
  const message = await fixture('sanitizador.configure_upstream.valid.json');
  message.payload.args.ssid = 'é'.repeat(16);
  assert.equal(decodeModuleMessage(JSON.stringify(message)).payload.args.ssid, message.payload.args.ssid);
  message.payload.args.ssid = 'é'.repeat(17);
  invalid(() => encodeModuleMessage(message));
  message.payload.args.ssid = 'LAB';
  message.payload.args.wifi_password = 'é'.repeat(32);
  invalid(() => encodeModuleMessage(message));
  message.payload.args.wifi_password = 'example-only';
  message.payload.args.ssid = '\ud800';
  invalid(() => encodeModuleMessage(message));
  const literal = JSON.stringify(message).replace('\\ud800', '\ud800');
  invalid(() => decodeModuleMessage(literal));
  invalid(() => decodeAnyMessage(literal));
  message.payload.args.ssid = '😀';
  assert.equal(decodeModuleMessage(JSON.stringify(message)).payload.args.ssid, '😀');
});

test('invalid UTF-8, versions, inherited types and extra secret fields fail with sanitised errors', async () => {
  invalid(() => decodeModuleMessage(new Uint8Array([0xff])));
  invalid(() => decodeModuleMessage('{'));
  for (const type of ['constructor', '__proto__', 'toString', null, 1, {}]) {
    const message = await fixture('get_state.valid.json');
    message.type = type;
    invalid(() => decodeModuleMessage(JSON.stringify(message)));
  }
  const message = await fixture('get_state.valid.json');
  message.payload.args.password = 'NEVER-ECHO-THIS';
  invalid(() => encodeModuleMessage(message));
  delete message.payload.args.password;
  message.version = 999;
  assert.throws(() => decodeAnyMessage(JSON.stringify(message)), { code: 'unsupported_version' });
  const cyclic = {}; cyclic.self = cyclic;
  invalid(() => encodeAnyMessage(cyclic));
  invalid(() => encodeAnyMessage({ toJSON() { throw new Error('NEVER-ECHO-THIS'); } }));
});

test('UTC calendar and time quality remain canonical in v2', async () => {
  const message = await fixture('set_time.valid.json');
  for (const date of ['2026-02-29T00:00:00Z', '2026-01-01T00:00:60Z', '0000-01-01T00:00:00Z', '2026-01-01T00:00:00+00:00']) {
    message.payload.args.timestamp = date;
    invalid(() => encodeModuleMessage(message));
  }
  message.payload.args.timestamp = '2024-02-29T00:00:00Z';
  assert.equal(decodeModuleMessage(encodeModuleMessage(message)).payload.args.timestamp, message.payload.args.timestamp);
  message.timestamp = '2026-01-01T00:00:00Z';
  invalid(() => encodeModuleMessage(message));
});

test('HTTPS destinations reject plain text, credentials, invalid ports and secret query endpoints', async () => {
  const doh = await fixture('sanitizador.configure_doh.valid.json');
  for (const endpoint of ['http://resolver.example/dns-query', 'https://user:NEVER-ECHO-THIS@resolver.example/', 'https://resolver.example:0/', 'https://resolver.example/dns-query?token=NEVER-ECHO-THIS']) {
    doh.payload.args.endpoint = endpoint;
    invalid(() => encodeModuleMessage(doh));
  }
  const mqtt = await fixture('configure_connectivity.valid.json');
  mqtt.payload.args.mqtt_uri = 'mqtt://broker.example:0';
  invalid(() => encodeModuleMessage(mqtt));
});

test('the 2048-byte logical message boundary is independent of BLE fragmentation', async () => {
  const message = await fixture('sanitizador.configure_doh.valid.json');
  message.message_id = 'a'.repeat(64);
  message.payload.command_id = message.message_id;
  message.payload.args.ca_certificate = '';
  const padding = 2048 - new TextEncoder().encode(JSON.stringify(message)).length;
  assert.ok(padding >= 1 && padding <= 1536);
  message.payload.args.ca_certificate = 'A'.repeat(padding);
  const encoded = encodeModuleMessage(message);
  assert.equal(encoded.byteLength, 2048);
  const assembler = new BleAssembler();
  let result;
  for (const packet of fragmentMessage(encoded, 42)) result = assembler.push(packet, 1000);
  assert.deepEqual(decodeModuleMessage(result), message);
  assert.equal(BLE_UUIDS.service, 'a90d0001-6d64-4f43-9e6d-9db51f1f0000');
  message.payload.args.ca_certificate += 'A';
  assert.throws(() => encodeModuleMessage(message), { code: 'message_too_large' });
});

test('channel and identity admission are explicit and never execute a command', async () => {
  const secret = await fixture('configure_connectivity.valid.json');
  assert.equal(checkModuleCommandAdmission(secret, context), null);
  assert.equal(checkModuleCommandAdmission(secret, { ...context, channel: 'mqtt' }), 'unauthorized');
  assert.equal(checkModuleCommandAdmission(secret, { ...context, authenticated: false }), 'unauthorized');
  assert.equal(checkModuleCommandAdmission(secret, { ...context, authorized: false }), 'unauthorized');
  assert.equal(checkModuleCommandAdmission(secret, { ...context, encrypted: false }), 'unauthorized');
  assert.equal(checkModuleCommandAdmission(secret, { ...context, authorizedDeviceId: 'other-node' }), 'unauthorized');
  assert.equal(checkModuleCommandAdmission(secret, { ...context, retained: true }), 'invalid_message');
  const capture = await fixture('escudo.capture_start.valid.json');
  assert.equal(checkNewModuleCommandContext(capture, { ...context, profile: 'escudo', targetSelected: true }), null);
  assert.equal(checkNewModuleCommandContext(capture, { ...context, profile: 'escudo', targetSelected: false }), 'invalid_state');
  assert.equal(checkModuleCommandAdmission(capture, context), null);
  assert.equal(checkNewModuleCommandContext(capture, context), 'invalid_state');
});

test('freshness precedes duplicate lookup and revision only applies to new commands', async () => {
  const message = await fixture('get_state.valid.json');
  assert.equal(checkModuleCommandAdmission(message, { ...context, revision: 4 }), null);
  assert.equal(checkNewModuleCommandContext(message, { ...context, revision: 4 }), 'revision_conflict');
  assert.equal(checkModuleCommandAdmission(message, { ...context, bootId: 'new-boot' }), 'wrong_boot');
  assert.equal(checkModuleCommandAdmission(message, { ...context, uptimeMs: 31000 }), 'expired');
  assert.equal(checkModuleCommandAdmission(message, { ...context, uptimeMs: 0 }), 'expired');
  // Rechecking an identical request is pure: the consumer owns its bounded cache and execution.
  const before = structuredClone(message);
  assert.equal(checkModuleCommandAdmission(message, context), null);
  assert.equal(checkModuleCommandAdmission(message, context), null);
  assert.deepEqual(message, before);
  const oldCapture = await fixture('escudo.capture_start.valid.json');
  assert.equal(checkModuleCommandAdmission(oldCapture, { ...context, revision: 4, profile: 'cebo' }), null);
  // A cached response may be replayed; only a new operation checks current mode/revision.
  assert.equal(checkNewModuleCommandContext(oldCapture, context), 'invalid_state');
});

test('MQTT version and node namespaces preserve historical event routes and QoS', () => {
  assert.equal(mqttTopic(1, 'node-example', 'events'), 'sentinel/v1/nodes/node-example/events');
  assert.equal(mqttTopic(2, 'node-example', 'events'), 'sentinel/v2/nodes/node-example/events');
  assert.deepEqual(MQTT_POLICY.commands, { qos: 1, retain: false });
  assert.deepEqual(MQTT_POLICY.availability, { qos: 1, retain: true });
  for (const id of ['other/node', '+', '#', '', 'node-example\n', 123, null]) invalid(() => mqttTopic(2, id, 'commands'));
  invalid(() => mqttTopic(2, 'node-example', 'constructor'));
});

test('each operation correlates to its documented applied result without confusing catalogs and history', async () => {
  const names = (await readdir(commands)).filter(name => name.endsWith('.valid.json'));
  assert.equal(names.length, Object.keys(MODULE_COMMAND_RESULTS).length);
  for (const name of names) {
    const command = await fixture(name);
    const kind = MODULE_COMMAND_RESULTS[command.payload.operation][0];
    const ack = JSON.parse(await readFile(new URL(`v2/acks/${kind ?? 'accepted'}.valid.json`, base), 'utf8'));
    ack.payload.status = 'applied';
    ack.payload.command_id = command.payload.command_id;
    if (kind === 'job') ack.payload.result.job_id = command.payload.command_id;
    if (kind === 'sanitizador.audit') ack.payload.result.audit.job_id = command.payload.args.job_id;
    assert.equal(checkModuleAckForCommand(command, ack), null, command.payload.operation);
    ack.payload.command_id = 'other-command';
    if (kind === 'job') ack.payload.result.job_id = 'other-command';
    assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
  }
  const command = await fixture('read_events.valid.json');
  const ack = JSON.parse(await readFile(new URL('v2/acks/cebo.stats.valid.json', base), 'utf8'));
  ack.payload.command_id = command.payload.command_id;
  assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
});

test('ack correlation handles a rejected old boot and rejects mismatched data and applied boot', async () => {
  const command = await fixture('get_state.valid.json');
  const ack = JSON.parse(await readFile(new URL('v2/acks/accepted.valid.json', base), 'utf8'));
  ack.payload.command_id = command.payload.command_id;
  assert.equal(checkModuleAckForCommand(command, ack), null);
  ack.boot_id = 'new-boot';
  assert.equal(checkModuleAckForCommand(command, ack), 'wrong_boot');
  ack.payload.status = 'rejected'; ack.payload.error = 'wrong_boot';
  assert.equal(checkModuleAckForCommand(command, ack), null);
  ack.device_id = 'other-node';
  assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
  for (const [operation, file, mutate] of [
    ['sanitizador.audit_result', 'sanitizador.audit', a => { a.payload.result.audit.job_id = 'other-job'; }],
    ['sanitizador.rules.put', 'sanitizador.rule', a => { a.payload.result.rule.action = 'allow'; }],
    ['sanitizador.rules.remove', 'sanitizador.rule_removed', a => { a.payload.result.rule_id = 'other-rule'; }],
    ['escudo.select_target', 'escudo.target', a => { a.payload.result.target.channel = 11; }],
  ]) {
    const pending = await fixture(operation + '.valid.json');
    const reply = JSON.parse(await readFile(new URL(`v2/acks/${file}.valid.json`, base), 'utf8'));
    reply.payload.command_id = pending.payload.command_id;
    mutate(reply);
    assert.equal(checkModuleAckForCommand(pending, reply), 'invalid_message', operation);
  }
});

test('catalog ACK respects requested size and advances its snapshot cursor', async () => {
  for (const [operation, file, field] of [
    ['escudo.networks', 'escudo.networks', 'networks'],
    ['sanitizador.clients', 'sanitizador.clients', 'clients'],
    ['sanitizador.rules.list', 'sanitizador.rules', 'rules'],
  ]) {
    const command = await fixture(operation + '.valid.json');
    command.payload.args.limit = 1;
    const ack = JSON.parse(await readFile(new URL(`v2/acks/${file}.valid.json`, base), 'utf8'));
    ack.payload.command_id = command.payload.command_id;
    assert.equal(checkModuleAckForCommand(command, ack), null);
    ack.payload.result[field].push(structuredClone(ack.payload.result[field][0]));
    assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
    ack.payload.result[field].pop();
    command.payload.args.after = 'snapshot:1';
    ack.payload.result.more = true; ack.payload.result.next = 'snapshot:1';
    assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
  }
});

test('history ACK cannot rewind or advance across missing events without a declared gap', async () => {
  const command = await fixture('read_events.valid.json');
  const ack = JSON.parse(await readFile(new URL('v2/acks/events.valid.json', base), 'utf8'));
  ack.payload.command_id = command.payload.command_id;
  command.payload.args.after_cursor = 100;
  assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
  command.payload.args.after_cursor = 0; command.payload.args.limit = 1;
  assert.equal(checkModuleAckForCommand(command, ack), 'invalid_message');
  ack.payload.result.gap = true;
  assert.equal(checkModuleAckForCommand(command, ack), null);
  ack.payload.result = { kind: 'events', next_cursor: 0, retained_from: 0, retained_to: 0, more: false, gap: false };
  assert.equal(checkModuleAckForCommand(command, ack), null);
  command.payload.args.after_cursor = 100; ack.payload.result.next_cursor = 100;
  assert.equal(checkModuleAckForCommand(command, ack), null);
});


test('trusted LAN MQTT admits authenticated commands without encryption, BLE still requires encryption', async () => {
  const command = await fixture('get_state.valid.json');
  const mqttContext = {...context, channel:'mqtt', profile:'cebo', encrypted:false};
  assert.equal(checkModuleCommandAdmission(command,mqttContext),null);
  assert.equal(checkModuleCommandAdmission(command,{...mqttContext,authenticated:false}),'unauthorized');
  assert.equal(checkModuleCommandAdmission(command,{...mqttContext,authorized:false}),'unauthorized');
  assert.equal(checkModuleCommandAdmission(command,{...mqttContext,channel:'ble'}),'unauthorized');
  const config=await fixture('configure_connectivity.valid.json');
  assert.equal(checkModuleCommandAdmission(config,mqttContext),'unauthorized');
  config.payload.args.ca_certificate='obsolete-ca';
  invalid(()=>encodeModuleMessage(config));
});
