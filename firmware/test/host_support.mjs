// Host tests reuse pinned cJSON sources; an IDF installation is only an optional cache.
import {createHash} from 'node:crypto';
import {existsSync, readFileSync, mkdirSync, writeFileSync, mkdtempSync, rmSync} from 'node:fs';
import {homedir, tmpdir} from 'node:os';
import {resolve, join} from 'node:path';
import {spawnSync} from 'node:child_process';
import {repo, validatorPath, generateValidator} from '../tools/generate_validator.mjs';
export {repo};
export const firmware = join(repo, 'firmware');
export const sketch = join(firmware, 'SentinelNode');
export const sha256 = data => createHash('sha256').update(data).digest('hex');
export const sanitizeFlags = process.env.SENTINEL_SANITIZE === '1'
  ? ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] : [];
if (sanitizeFlags.length)
  process.env.UBSAN_OPTIONS = [process.env.UBSAN_OPTIONS, 'halt_on_error=1'].filter(Boolean).join(':');

export function run(command, args = [], options = {}) {
  const result = spawnSync(command, args, {cwd: repo, encoding: 'utf8', stdio: 'inherit', ...options});
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(`${command} failed (${result.signal ?? result.status})`);
  return result.stdout;
}
export const capture = (command, args = [], options = {}) => run(command, args, {stdio:'pipe', ...options});
export function temporary(prefix, callback) {
  const directory = mkdtempSync(join(tmpdir(), prefix));
  try { return callback(directory); } finally { rmSync(directory, {recursive:true, force:true}); }
}
export function verifyValidator() {
  if (readFileSync(validatorPath, 'utf8') !== generateValidator())
    throw new Error('Distributed validator differs from shared schemas; regenerate and review it.');
}
export async function cjsonSources() {
  const hashes = {
    'cJSON.c':'298581a04a36c0165da4b0aade235c23088cb2faa58651d720ea2f3706ed0b0d',
    'cJSON.h':'25b0145150d500498e4d209cec69c18c42cf818bffcc54690be3b895a2a16dee',
  };
  const valid = directory => Object.entries(hashes).every(([name, digest]) =>
    existsSync(join(directory, name)) && sha256(readFileSync(join(directory, name))) === digest);
  if (process.env.SENTINEL_CJSON_DIR) {
    const directory = resolve(process.env.SENTINEL_CJSON_DIR);
    if (!valid(directory)) throw new Error('SENTINEL_CJSON_DIR must contain pinned cJSON 1.7.19 sources');
    return directory;
  }
  const existing = join(homedir(), '.platformio/packages/framework-espidf/components/json/cJSON');
  if (valid(existing)) return existing;
  const cache = join(tmpdir(), 'sentinel-cjson-1.7.19');
  mkdirSync(cache, {recursive:true});
  if (!valid(cache)) for (const [name, digest] of Object.entries(hashes)) {
    const response = await fetch('https://raw.githubusercontent.com/DaveGamble/cJSON/v1.7.19/' + name,
      {signal:AbortSignal.timeout(30000)});
    if (!response.ok) throw new Error(`cJSON download failed (${response.status}): ${name}`);
    const data = Buffer.from(await response.arrayBuffer());
    if (sha256(data) !== digest) throw new Error('Unexpected cJSON source digest: ' + name);
    writeFileSync(join(cache, name), data);
  }
  return cache;
}
export function cryptoLibrary() {
  // Runtime-only ABI is sufficient; do not require development headers or a new SDK.
  const result = spawnSync('ldconfig', ['-p'], {encoding:'utf8'});
  const library = result.stdout?.match(/\b(libcrypto\.so(?:\.\d+)+)\s/);
  if (!library) throw new Error('Install the system OpenSSL runtime for host digest checks.');
  return '-l:' + library[1];
}

export async function buildHost(suite) {
  verifyValidator();
  // Parallel agent/test invocations must not overwrite each other's objects/binaries.
  const cache = mkdtempSync(join(tmpdir(), `sentinel-${suite}-host-`));
  process.once('exit', () => rmSync(cache, {recursive:true, force:true}));
  const cjson = await cjsonSources();
  run('cc', [...sanitizeFlags, '-std=c99', '-g', '-c', join(cjson,'cJSON.c'), '-o', join(cache,'cJSON.o')]);
  const includes = [cjson, join(sketch,'src/sentinel_api/include'), join(sketch,'src/sentinel/include')];
  const sources = ['core.cpp','protocol_v2.cpp','module_runtime.cpp','wire.cpp','digest.cpp']
    .map(s => join(sketch,'src/sentinel',s));
  if (suite === 'escudo') {
    includes.push(join(sketch,'src/escudo/include'), join(firmware,'test/stubs/connectivity'));
    sources.push(join(sketch,'src/sentinel/escudo_target.cpp'));
    sources.push(...['discovery.cpp','module.cpp','capture.cpp','frame_parser.cpp','channel_scheduler.cpp','detectors.cpp']
      .map(s => join(sketch,'src/escudo',s)));
  }
  const flags = [...sanitizeFlags, '-std=c++17', '-g', '-Wall', '-Wextra', '-Wno-misleading-indentation',
    ...includes.map(p => '-I' + p)];
  const names = suite === 'core' ? ['core_test','network_stop_test']
    : ['discovery_test','capture_test','detectors_test','recovery_test','target_persistence_test'];
  const crypto = cryptoLibrary();
  for (const name of names) {
    // Standalone regressions have their own main; only core_test links the module fixtures.
    const tests = name === 'core_test' ? ['core_test.cpp','module_consumer.cpp','module_test.cpp']
      .map(s => join(firmware,'test',s))
      : [join(firmware, suite === 'core' ? 'test' : 'test/escudo', name + '.cpp')];
    if (suite === 'escudo' && name !== 'target_persistence_test')
      tests.push(join(firmware,'test/stubs/escudo_target_nvs.cpp'));
    const binary = join(cache,name);
    run('c++', [...flags, ...sources, ...tests, join(cache,'cJSON.o'), crypto, '-o', binary]);
    if (name === 'recovery_test') temporary('sentinel-recovery-', directory => {
      for (const phase of ['write','recover','checks']) run(binary,[phase,directory]);
    });
    else run(binary);
  }
  if (suite === 'core') {
    // Compile the real read-only preflight against NVS/NimBLE stubs with no erase/write API.
    const binary = join(cache,'ble_storage_test');
    run('c++', [...flags, '-I'+join(firmware,'test/stubs/ble_storage'),
      join(sketch,'src/sentinel/ble_storage.cpp'), join(firmware,'test/ble_storage_test.cpp'), '-o', binary]);
    run(binary);
    const connectivityBinary = join(cache,'connectivity_test');
    run('c++', [...flags, '-I'+join(firmware,'test/stubs/connectivity'),
      '-I'+join(firmware,'test/transports/include'),
      ...sources, join(sketch,'src/sentinel/connectivity_config.cpp'),
      join(sketch,'src/sentinel/network.cpp'), join(firmware,'test/connectivity_test.cpp'),
      join(cache,'cJSON.o'), crypto, '-o', connectivityBinary]);
    run(connectivityBinary, [join(firmware,'test/transports/fixtures/root.pem'),
      join(firmware,'test/transports/fixtures/valid.pem')]);
  }
  console.log(`Firmware ${suite}: PASS (host; no physical radio/NVS claims)`);
  return cache;
}
