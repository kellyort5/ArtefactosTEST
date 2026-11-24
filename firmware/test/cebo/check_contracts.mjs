#!/usr/bin/env node
// Pasa los eventos, métricas, estados y ACK que produce el código C++ de Cebo
// por el decodificador v2 compartido (AJV). Requiere el paquete de contratos
// construido (`npm run build -w contracts`) o SENTINEL_CONTRACTS_MODULE con la
// ruta de su dist/src/index.js.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {join} from 'node:path';
import {pathToFileURL} from 'node:url';
import {buildCebo, runCebo, repo} from './build.mjs';

const target = process.env.SENTINEL_CONTRACTS_MODULE
  ? pathToFileURL(process.env.SENTINEL_CONTRACTS_MODULE).href : '@sentinel/contracts';
const {decodeModuleMessage, checkModuleAckForCommand} = await import(target);

const lines = runCebo(buildCebo(), ['dump']).trim().split('\n');
const fixture = path => JSON.parse(readFileSync(join(repo, 'contracts/fixtures/v2', path), 'utf8'));
const envelope = (type, n, payload) => ({
  version: 2, type, device_id: 'node-example', boot_id: 'boot-example',
  message_id: `cebo.check-${n}`, seq: n, uptime_ms: 1000, timestamp: null,
  time_quality: 'unknown', payload,
});
const eventFor = (details, n, severity = 'info', name = 'cebo.interaction') =>
  envelope('event', n, {journal_seq: n, name, profile: 'cebo', code: null,
    severity, details, origin: 'real'});
const stateTemplate = fixture('states/cebo.valid.json');
const stateFor = module => {
  const state = structuredClone(stateTemplate);
  state.payload.module = module;
  // Las capacidades que anuncia el módulo con los dos servicios.
  state.payload.capabilities.honeypot_http = true;
  state.payload.capabilities.honeypot_telnet = true;
  return state;
};
const accepts = message => decodeModuleMessage(JSON.stringify(message));
const rejects = (message, why) =>
  assert.throws(() => decodeModuleMessage(JSON.stringify(message)), undefined, why);

const counts = {events: 0, states: 0, metrics: 0, stats: 0, status: 0};
const suspicions = new Set();
let n = 0;
for (const line of lines) {
  const [label, ...rest] = line.split(' ');
  // `event <severity> <json>`, el resto `<label> <json>`.
  const severity = label === 'event' ? rest.shift() : 'info';
  const json = JSON.parse(rest.join(' '));
  if (label === 'details' || label === 'event') {
    accepts(eventFor(json, ++n, severity));
    counts.events++;
    suspicions.add(json.suspicion);
    // El contrato es cerrado: un campo extra con un secreto se rechaza.
    rejects(eventFor({...json, password: 'x'}, 99, severity), 'extra field');
    rejects(eventFor({...json, path: '/login?x=1'}, 99, severity), 'query in path');
    // La sospecha va ligada a la severidad: cualquier otra combinación falla.
    const wrong = {informational: 'warning', medium: 'critical', high: 'info'}[json.suspicion];
    rejects(eventFor(json, 99, wrong), `suspicion ${json.suspicion} with severity ${wrong}`);
  } else if (label === 'module') {
    accepts(stateFor(json));
    counts.states++;
  } else if (label === 'metrics') {
    const message = envelope('metrics', ++n, {scope: 'cebo', interval_ms: 5000, values: json});
    accepts(message);
    counts.metrics++;
    const extra = structuredClone(message);
    extra.payload.values.http.body_bytes = 1;
    rejects(extra, 'extra metric');
    const over = structuredClone(message);
    over.payload.values.http.active_connections = 7;
    rejects(over, 'active_connections above 6');
  } else if (label === 'stats') {
    // El ACK de cebo.stats debe corresponder a su orden y llevar estos valores.
    const command = fixture('commands/cebo.stats.valid.json');
    const ack = fixture('acks/cebo.stats.valid.json');
    ack.payload.command_id = command.payload.command_id;
    ack.payload.result = json;
    accepts(ack);
    assert.equal(checkModuleAckForCommand(command, ack), null);
    assert.equal(json.kind, 'cebo.stats');
    counts.stats++;
    const bad = structuredClone(ack);
    bad.payload.result.values.telnet.credentials = 'x';
    rejects(bad, 'extra field in stats');
  } else if (label === 'status') {
    // Diagnóstico por registro de configuración dañado.
    const event = eventFor(json, ++n, 'warning', 'system.status');
    accepts(event);
    counts.status++;
  } else {
    assert.fail('unexpected dump line: ' + label);
  }
}
assert(counts.events >= 12 && counts.states >= 5 && counts.metrics === 1 &&
  counts.stats === 1 && counts.status === 1, `expected samples, got ${JSON.stringify(counts)}`);
// Las tres sospechas del contrato aparecen en eventos reales del módulo.
assert.deepEqual([...suspicions].sort(), ['high', 'informational', 'medium']);
// cebo.configure del contrato se acepta tal cual (el módulo lo valida aparte).
accepts(fixture('commands/cebo.configure.valid.json'));
console.log(`Cebo C++ events, metrics, cebo.stats and state -> shared v2 codec: PASS (${counts.events} events, ` +
  `${counts.metrics} metrics, ${counts.stats} stats, ${counts.status} status, ${counts.states} states; host)`);
