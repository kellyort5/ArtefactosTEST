// El proceso real (`node src/index.js`) arrancado desde un archivo de configuración
// privado, con un broker MQTT en proceso y un receptor de webhook local.
import assert from 'node:assert/strict';
import {spawn, spawnSync} from 'node:child_process';
import {writeFileSync} from 'node:fs';
import net from 'node:net';
import {join} from 'node:path';
import {after, test} from 'node:test';
import {Aedes} from 'aedes';
import mqtt from 'mqtt';
import {NODE_A, TOKEN_A, WEBHOOK_SECRET, bytes, ceboEvent, sha256hex, tempDir, topicFor, until, webhookServer}
  from './helpers.mjs';

const entry = new URL('../src/index.js', import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1');
const cleanups = [];
after(async () => { for (const fn of cleanups.reverse()) await fn().catch(() => {}); });

function writeConfig(dir, overrides) {
  const file = join(dir, 'notifications.json');
  writeFileSync(file, JSON.stringify({
    mqtt: {url: 'mqtt://127.0.0.1:1', client_id: 'notify-entry'},
    nodes: [{device_id: NODE_A, token_sha256: sha256hex(TOKEN_A)}],
    alerts: {include_simulated: true},
    status_api: {host: '127.0.0.1', port: 18443, allow_insecure_loopback: true},
    state_dir: join(dir, 'state'),
    ...overrides,
  }));
  return file;
}

test('el proceso arranca, recibe un evento por MQTT y lo entrega al webhook sin registrar secretos', async () => {
  const t = tempDir();
  cleanups.push(async () => t.cleanup());
  const aedes = await Aedes.createBroker();
  const mqttServer = net.createServer(aedes.handle);
  await new Promise(resolve => mqttServer.listen(0, '127.0.0.1', resolve));
  cleanups.push(async () => { mqttServer.close(); mqttServer.closeAllConnections?.(); await new Promise(r => aedes.close(r)); });
  const subscriptions = [];
  aedes.on('subscribe', (subs, client) => subscriptions.push(...subs.map(s => `${client?.id}:${s.topic}`)));
  const hook = await webhookServer();
  cleanups.push(() => hook.close());

  const file = writeConfig(t.dir, {
    mqtt: {url: `mqtt://127.0.0.1:${mqttServer.address().port}`, client_id: 'notify-entry'},
    webhook: {url: `http://127.0.0.1:${hook.port}/hook`, allow_insecure_loopback: true,
      auth_header: {name: 'X-Api-Key', value: WEBHOOK_SECRET}},
  });
  const child = spawn(process.execPath, [entry], {env: {...process.env, NOTIFY_CONFIG: file}, stdio: ['ignore', 'pipe', 'pipe']});
  cleanups.push(async () => { child.kill(); });
  let output = '';
  child.stdout.on('data', d => { output += d; });
  child.stderr.on('data', d => { output += d; });

  // Si el proceso falla, la salida del hijo explica por qué.
  const diagnose = error => { error.message += `
--- salida del proceso ---
${output}`; throw error; };
  await until(() => subscriptions.length >= 2, 10000).catch(diagnose); // v1 y v2 del único nodo
  assert.ok(subscriptions.includes(`notify-entry:sentinel/v2/nodes/${NODE_A}/events`));
  assert.ok(subscriptions.includes(`notify-entry:sentinel/v1/nodes/${NODE_A}/events`));
  const client = await mqtt.connectAsync(`mqtt://127.0.0.1:${mqttServer.address().port}`, {protocolVersion: 4});
  cleanups.push(() => client.endAsync(true));
  await client.publishAsync(topicFor(), bytes(ceboEvent({id: 'boot-1:99', severity: 'critical'})), {qos: 1});
  await until(() => hook.requests.length === 1, 10000).catch(diagnose);
  assert.equal(hook.requests[0].json.event_id, 'boot-1:99');
  assert.equal(hook.requests[0].headers['x-api-key'], WEBHOOK_SECRET);

  const lines = output.trim().split('\n').map(line => JSON.parse(line)); // todo el registro es JSON por línea
  assert.ok(lines.some(l => l.msg === 'started' && l.status_api_port === 18443 && l.nodes === 1 && l.webhook === true));
  assert.ok(lines.some(l => l.msg === 'event' && l.event_id === 'boot-1:99' && l.outcome === 'routed'));
  for (const secret of [WEBHOOK_SECRET, TOKEN_A, sha256hex(TOKEN_A), `127.0.0.1:${hook.port}`, '/hook'])
    assert.ok(!output.includes(secret), `el registro contiene ${secret}`);
});

test('una configuración inválida termina con código 1 y un mensaje que no repite el valor', () => {
  const t = tempDir();
  try {
    const file = writeConfig(t.dir, {webhook: {url: 'http://hooks.example.invalid/FICTIONAL-PATH-SECRET', auth_header: {name: 'X-Key', value: WEBHOOK_SECRET}}});
    const run = spawnSync(process.execPath, [entry], {env: {...process.env, NOTIFY_CONFIG: file}, encoding: 'utf8', timeout: 20000});
    assert.equal(run.status, 1);
    const text = run.stdout + run.stderr;
    assert.match(text, /config_invalid/);
    assert.match(text, /webhook\.url/);
    for (const secret of [WEBHOOK_SECRET, 'FICTIONAL-PATH-SECRET', 'hooks.example.invalid']) assert.ok(!text.includes(secret));
    // Sin archivo tampoco arranca.
    const none = spawnSync(process.execPath, [entry], {env: {...process.env, NOTIFY_CONFIG: ''}, encoding: 'utf8', timeout: 20000});
    assert.equal(none.status, 1);
  } finally { t.cleanup(); }
});
