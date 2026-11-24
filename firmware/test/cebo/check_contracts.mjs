#!/usr/bin/env node
// Pasa los eventos y estados que produce el código C++ de Cebo por el
// decodificador v2 compartido (AJV). Requiere el paquete de contratos
// construido (`npm run build -w contracts`) o SENTINEL_CONTRACTS_MODULE con la
// ruta de su dist/src/index.js.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {join} from 'node:path';
import {pathToFileURL} from 'node:url';
import {buildCebo, runCebo, repo} from './build.mjs';

const target = process.env.SENTINEL_CONTRACTS_MODULE
  ? pathToFileURL(process.env.SENTINEL_CONTRACTS_MODULE).href : '@sentinel/contracts';
const {decodeModuleMessage} = await import(target);

const lines = runCebo(buildCebo(), ['dump']).trim().split('\n');
const eventFor = (details, n) => ({
  version: 2, type: 'event', device_id: 'node-example', boot_id: 'boot-example',
  message_id: `cebo.http-check-${n}`, seq: n, uptime_ms: 1000, timestamp: null,
  time_quality: 'unknown',
  payload: {journal_seq: n, name: 'cebo.interaction', profile: 'cebo', code: null,
    severity: 'info', details, origin: 'real'},
});
const stateTemplate = JSON.parse(readFileSync(join(repo, 'contracts/fixtures/v2/states/cebo.valid.json'), 'utf8'));
const stateFor = module => {
  const state = structuredClone(stateTemplate);
  state.payload.module = module;
  // Las capacidades que anuncia el módulo con los dos servicios.
  state.payload.capabilities.honeypot_http = true;
  state.payload.capabilities.honeypot_telnet = true;
  return state;
};

let events = 0, states = 0;
for (const line of lines) {
  const [label, ...rest] = line.split(' ');
  const json = JSON.parse(rest.join(' '));
  if (label === 'details') {
    decodeModuleMessage(JSON.stringify(eventFor(json, ++events)));
    // El contrato es cerrado: un campo extra con un secreto debe rechazarse.
    assert.throws(() => decodeModuleMessage(JSON.stringify(eventFor({...json, password: 'x'}, 99))));
    assert.throws(() => decodeModuleMessage(JSON.stringify(eventFor({...json, path: '/login?x=1'}, 99))));
  } else if (label === 'module') {
    decodeModuleMessage(JSON.stringify(stateFor(json)));
    states++;
  } else {
    assert.fail('unexpected dump line: ' + label);
  }
}
assert(events >= 6 && states >= 4, `expected samples, got ${events} events and ${states} states`);
console.log(`Cebo C++ events and state -> shared v2 codec: PASS (${events} events, ${states} states; host)`);
