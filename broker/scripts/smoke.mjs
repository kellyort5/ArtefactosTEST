// Real MQTT 5 clients against the Docker broker; only disposable local lab data.
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import mqtt from 'mqtt';

const broker = new URL('../', import.meta.url);
const lab = JSON.parse(await readFile(new URL('.secrets/clients.json', broker), 'utf8'));
const host = process.env.SENTINEL_TEST_HOST || lab.host;
const clients = new Set();
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const root = fileURLToPath(new URL('../', broker));
let version = 1;
const topic = suffix => `sentinel/v${version}/nodes/${lab.node_id}/${suffix}`;
const peerTopic = suffix => `sentinel/v${version}/nodes/${lab.peer_id}/${suffix}`;

async function connect(transport, account, overrides = {}) {
  const port = transport === 'mqtt' ? 1883 : 9001;
  const options = {
    ...account, ...overrides, protocolVersion: 5, clean: true, reconnectPeriod: 0,
    connectTimeout: 4000, properties: { sessionExpiryInterval: 0 },
  };
  const client = mqtt.connect(`${transport}://${host}:${port}${transport === 'ws' ? '/mqtt' : ''}`, options);
  clients.add(client);
  client.on('error', () => {}); // Individual operations assert errors; never log packets/credentials.
  try {
    await new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(Object.assign(new Error('connection_timeout'), { code: 'TEST_TIMEOUT' })), 5000);
      client.once('connect', () => { clearTimeout(timer); resolve(); });
      client.once('error', error => { clearTimeout(timer); reject(error); });
    });
    return client;
  } catch (error) {
    await close(client);
    throw error;
  }
}

async function close(client) {
  clients.delete(client);
  await new Promise(resolve => client.end(true, {}, resolve));
}

async function subscribe(client, value) {
  const granted = await client.subscribeAsync(value, { qos: 1 });
  assert(granted.every(item => item.qos < 128), 'subscription was denied');
}

function receive(client, value, expected, retained = false) {
  return new Promise((resolve, reject) => {
    const handler = (name, payload, packet) => {
      if (name !== value || payload.toString() !== expected) return;
      clearTimeout(timer); client.off('message', handler);
      if (retained && !packet.retain) reject(new Error('retained flag missing'));
      else resolve();
    };
    const timer = setTimeout(() => { client.off('message', handler); reject(new Error('message not received')); }, 4000);
    client.on('message', handler);
  });
}

async function rejection(transport, account, override, allowedCodes) {
  let connected;
  try {
    connected = await connect(transport, account, override);
  } catch (error) {
    assert(allowedCodes.includes(error.code), `unexpected rejection code ${error.code}`);
    return;
  }
  await close(connected);
  assert.fail('connection should have been rejected');
}

async function deniedPublish(sender, observer, value) {
  await subscribe(observer, value);
  const marker = `denied-${crypto.randomUUID()}`;
  let delivered = false;
  const handler = (name, payload) => { if (name === value && payload.toString() === marker) delivered = true; };
  observer.on('message', handler);
  let denied = false;
  try {
    await sender.publishAsync(value, marker, { qos: 1, retain: false });
  } catch (error) {
    assert.equal(error.code, 135, 'expected MQTT 5 Not authorized PUBACK');
    denied = true;
  }
  await pause(400);
  observer.off('message', handler);
  assert(denied, 'broker must explicitly deny publication');
  assert.equal(delivered, false, 'denied message was delivered');
}

async function deniedRead(reader, writer, authorized, value) {
  // Mosquitto may accept SUBSCRIBE then filter unauthorized deliveries.
  try { await reader.subscribeAsync(value, { qos: 1 }); }
  catch (error) { assert([128, 135].includes(error.code)); }
  await subscribe(authorized, value);
  const marker = `read-isolation-${crypto.randomUUID()}`;
  let delivered = false;
  const handler = (name, payload) => { if (name === value && payload.toString() === marker) delivered = true; };
  reader.on('message', handler);
  const received = receive(authorized, value, marker);
  await writer.publishAsync(value, marker, { qos: 1 });
  await received;
  await pause(400);
  reader.off('message', handler);
  assert.equal(delivered, false, 'unauthorized subscriber received a message');
}

async function run() {
  // Test payloads are transport probes; no fake product state or incidents.
  for (version of [1, 2]) {
  for (const transport of ['mqtt', 'ws']) {
    const node = await connect(transport, lab.accounts.node);
    const operator = await connect(transport, lab.accounts.operator);
    const viewer = await connect(transport, lab.accounts.viewer);
    const peerViewer = await connect(transport, lab.peer_accounts.viewer);
    const peerNode = await connect(transport, lab.peer_accounts.node);
    await subscribe(node, topic('commands'));
    await subscribe(viewer, topic('events'));
    await subscribe(operator, topic('acks'));
    let received = receive(node, topic('commands'), 'transport-command-probe');
    await operator.publishAsync(topic('commands'), 'transport-command-probe', { qos: 1 });
    await received;
    received = receive(viewer, topic('events'), 'transport-event-probe');
    await node.publishAsync(topic('events'), 'transport-event-probe', { qos: 1 });
    await received;
    received = receive(operator, topic('acks'), 'transport-ack-probe');
    await node.publishAsync(topic('acks'), 'transport-ack-probe', { qos: 1 });
    await received;
    console.log(`PASS v${version} ${transport}: node/operator/viewer authorized round trips`);

    await rejection(transport, {}, {}, [134, 135]);
    await rejection(transport, { ...lab.accounts.node, password: 'invalid-disposable-password' }, {}, [134, 135]);
    await deniedPublish(node, peerViewer, peerTopic('state'));
    await deniedPublish(viewer, node, topic('commands'));
    await deniedPublish(operator, peerViewer, peerTopic('state'));
    await deniedPublish(operator, peerNode, peerTopic('commands'));
    await deniedPublish(operator, viewer, topic('events'));
    await deniedRead(node, peerNode, peerViewer, peerTopic('events'));
    await deniedRead(viewer, operator, node, topic('commands'));
    await deniedRead(operator, peerNode, peerViewer, peerTopic('state'));
    console.log(`PASS v${version} ${transport}: anonymous/password/ACL denied and unauthorized deliveries absent`);
    await Promise.all([node, operator, viewer, peerViewer, peerNode].map(close));
  }

  }
  version = 1;
  const node = await connect('mqtt', lab.accounts.node);
  const marker = `persistence-${crypto.randomUUID()}`;
  await node.publishAsync(topic('state'), marker, { qos: 1, retain: true });
  await close(node);
  execFileSync('python3', ['broker/scripts/manage.py', 'restart'], { cwd: root, stdio: ['ignore', 'ignore', 'pipe'] });
  let viewer;
  for (let attempt = 0; attempt < 20; attempt++) {
    try { viewer = await connect('ws', lab.accounts.viewer); break; }
    catch { await pause(300); }
  }
  assert(viewer, 'broker did not reconnect after restart');
  const persisted = receive(viewer, topic('state'), marker, true);
  await subscribe(viewer, topic('state'));
  await persisted;
  await close(viewer);
  // Clear only the disposable test retained probe; do not leave fake product state.
  const cleanup = await connect('mqtt', lab.accounts.node);
  await cleanup.publishAsync(topic('state'), '', { qos: 1, retain: true });
  await close(cleanup);
  console.log('PASS restart: retained state survives in named volume and is received with retain=true');
}

try { await run(); }
catch (error) {
  // Assertion messages contain only our probes, never account values or network packets.
  console.error(`FAIL broker smoke: ${error.code || error.name}: ${error.message}`);
  process.exitCode = 1;
} finally {
  await Promise.all([...clients].map(close));
}
