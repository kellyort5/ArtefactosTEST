// Compilación común de las pruebas de host de Cebo. Solo necesita un compilador
// C++17: no depende de cJSON, OpenSSL, radio ni NVS.
import {mkdtempSync, rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {dirname, join, resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';

const here = dirname(fileURLToPath(import.meta.url));
export const repo = resolve(here, '../../..');
const sketch = join(repo, 'firmware/SentinelNode');
const sanitize = process.env.SENTINEL_SANITIZE === '1'
  ? ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] : [];

function compiler() {
  for (const candidate of [process.env.CXX, 'c++', 'g++', 'clang++'].filter(Boolean)) {
    if (spawnSync(candidate, ['--version'], {stdio: 'ignore'}).status === 0) return candidate;
  }
  throw new Error('No C++ compiler found (set CXX).');
}

export function buildCebo() {
  const cache = mkdtempSync(join(tmpdir(), 'sentinel-cebo-host-'));
  process.once('exit', () => rmSync(cache, {recursive: true, force: true}));
  const binary = join(cache, process.platform === 'win32' ? 'cebo_test.exe' : 'cebo_test');
  const build = spawnSync(compiler(), [
    ...sanitize, '-std=c++17', '-g', '-Wall', '-Wextra',
    '-I' + join(repo, 'firmware/test/stubs/cebo'),
    '-I' + join(sketch, 'src/sentinel_api/include'),
    join(sketch, 'src/cebo/interaction.cpp'),
    join(sketch, 'src/cebo/http_policy.cpp'),
    join(sketch, 'src/cebo/module.cpp'),
    join(here, 'cebo_test.cpp'),
    '-o', binary,
  ], {cwd: repo, stdio: 'inherit'});
  if (build.error) throw build.error;
  if (build.status !== 0) throw new Error('Cebo host build failed');
  return binary;
}

export function runCebo(binary, args, options = {}) {
  const run = spawnSync(binary, args, {cwd: repo, encoding: 'utf8', ...options});
  if (run.error) throw run.error;
  if (run.status !== 0) throw new Error(`Cebo host binary failed (${run.signal ?? run.status})`);
  return run.stdout ?? '';
}
