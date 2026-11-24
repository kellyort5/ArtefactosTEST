#!/usr/bin/env node
// Cross-check the real C++ serializer/parser and the existing shared AJV codec.
import assert from 'node:assert/strict';
import {readFileSync, readdirSync} from 'node:fs';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {buildHost, run, capture, repo} from './host_support.mjs';

assert.equal(Number(process.versions.node.split('.')[0]), 24, 'Use Node 24 in PATH');
const cache = await buildHost('core'), binary = join(cache,'core_test');
run('npm',['run','build','-w','contracts']);
const {decodeMessage, decodeModuleMessage, decodeModuleMetadata} = await import('@sentinel/contracts');
function checkWire(mode, decode) {
  for (const line of capture(binary,[mode]).trim().split('\n')) decode(line);
  console.log(`C++ ${mode} output -> shared AJV codec: PASS`);
}
checkWire('wire',decodeMessage);
for (const name of readdirSync(join(repo,'contracts/fixtures')).filter(s => s.endsWith('.json')).sort()) {
  const path = join(repo,'contracts/fixtures',name);
  if (JSON.parse(readFileSync(path,'utf8')).type !== 'command') continue;
  const result = spawnSync(binary,['parse',path],{cwd:repo, encoding:'utf8'});
  assert.ifError(result.error);
  assert.equal(result.signal, null, name + ': parser crashed');
  assert.equal(result.status, name.includes('.valid.') ? 0 : 2, name);
}
console.log('Shared command fixtures -> C++ cJSON parser: PASS');
let checked = 0;
for (const name of readdirSync(join(repo,'contracts/fixtures/v2'),{recursive:true})
  .filter(s => s.endsWith('.json') && !s.split('/').includes('notifications')).sort()) {
  const path = join(repo,'contracts/fixtures/v2',name), data = JSON.parse(readFileSync(path,'utf8'));
  if (!data.type && !('max_message_bytes' in data)) continue;
  const decode = data.type ? decodeModuleMessage : decodeModuleMetadata;
  let accepted = false;
  try { decode(readFileSync(path,'utf8')); accepted = true; } catch { /* The expected invalid fixture is also rejected by AJV. */ }
  assert.equal(accepted,name.includes('.valid.'),'Shared AJV fixture: '+name);
  const result = spawnSync(binary,['v2',path],{cwd:repo, encoding:'utf8'});
  assert.ifError(result.error);
  assert.equal(result.signal, null, name + ': validator crashed');
  assert.equal(result.status, name.includes('.valid.') ? 0 : 2, name);
  checked++;
}
console.log(`Shared v2 fixtures -> AJV and native schema/semantic validation: PASS (${checked})`);
checkWire('wire2',decodeModuleMessage);
