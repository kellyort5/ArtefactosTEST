#!/usr/bin/env node
// Compila el paquete compartido de contratos (`contracts/dist`, ignorado por Git)
// que este componente consume por `file:../contracts`. Requiere haber instalado
// las dependencias del repositorio (`npm ci` en la raíz): contratos usa ajv.
import {spawnSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {dirname, resolve} from 'node:path';
import {fileURLToPath} from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const contracts = resolve(here, '../../contracts');
const requireFromContracts = createRequire(resolve(contracts, 'package.json'));
try {
  requireFromContracts.resolve('ajv');
} catch {
  console.error('Falta ajv para compilar contratos. Ejecuta `npm ci` en la raíz del repositorio y vuelve a intentarlo.');
  process.exit(1);
}
const tsc = createRequire(import.meta.url).resolve('typescript/bin/tsc');
const run = spawnSync(process.execPath, [tsc, '-p', resolve(contracts, 'tsconfig.json')], {stdio: 'inherit'});
process.exit(run.status ?? 1);
