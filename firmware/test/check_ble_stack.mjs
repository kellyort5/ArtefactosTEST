#!/usr/bin/env node
import assert from 'node:assert/strict';
import {readFileSync, readdirSync} from 'node:fs';
import {join} from 'node:path';

const directory = process.argv[2];
assert(directory, 'Usage: node firmware/test/check_ble_stack.mjs <GCC .su directory>');
const budgets = {access:768, digest_command:128, submit:512, busy:768};
const frames = new Map();
for (const file of readdirSync(directory).filter(name => name.endsWith('.su'))) {
  for (const line of readFileSync(join(directory,file),'utf8').split('\n')) {
    const [symbol,size,kind] = line.trim().split('\t');
    for (const name of Object.keys(budgets)) {
      if (symbol?.includes('sentinel::'+name+'(')) {
        assert.equal(kind,'static',name+': stack frame must have a static bound');
        frames.set(name,Number(size));
      }
    }
  }
}
for (const [name,budget] of Object.entries(budgets)) {
  assert(frames.has(name),name+': native stack report missing');
  assert(frames.get(name)<=budget,name+': BLE callback stack budget exceeded');
}
const total = [...frames.values()].reduce((sum,size)=>sum+size,0);
assert(total<=2048,'Application callback frames must leave room in the 5120-byte NimBLE stack');
console.log('BLE native application frames: '+JSON.stringify(Object.fromEntries(frames)));
console.log('BLE callback stack budget: PASS ('+total+'/2048 bytes; excludes SDK/library frames)');
