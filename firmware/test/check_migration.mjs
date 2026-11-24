#!/usr/bin/env node
// Preserve immutable test cases and one entry; the approved USB layout is checked separately.
import assert from 'node:assert/strict';
import {existsSync, readFileSync, readdirSync} from 'node:fs';
import {join, basename} from 'node:path';
import {repo, sketch, sha256} from './host_support.mjs';
const index = process.argv.indexOf('--baseline');
if (index < 0 || !process.argv[index + 1]) throw new Error('Use --baseline <immutable pre-change SHA-256 map>');
const baseline = JSON.parse(readFileSync(process.argv[index + 1],'utf8'));
const checked = [];
for (const [old, expected] of Object.entries(baseline)) {
  // sentinel-mvp-standard-persistence explicitly authorizes replacing the old layout.
  const immutable = old.startsWith('firmware/test/') && /\.(cpp|hpp)$/.test(old);
  if (!immutable) continue;
  const moved = old;
  const actual = sha256(readFileSync(join(repo,moved)));
  assert.equal(actual,expected,`Unexpected test change: ${old} -> ${moved}`);
  checked.push({before:old,after:moved,sha256:actual});
}
assert(checked.length > 0,'The baseline must contain immutable tests');
for (const legacy of ['firmware/components','firmware/src']) if (existsSync(join(repo,legacy)))
  assert(!readdirSync(join(repo,legacy),{recursive:true}).some(p => /\.(cpp|c|ino)$/.test(p)),legacy);
const entries = readdirSync(sketch,{recursive:true}).filter(p => p.endsWith('.ino'));
assert.deepEqual(entries,['SentinelNode.ino']);
assert.equal(basename(sketch),'SentinelNode');
console.log(JSON.stringify({status:'PASS',scope:'immutable tests and single Arduino entry; approved USB layout requires check_config; functional refactors require suites',unchanged_files:checked},null,2));
