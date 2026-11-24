#!/usr/bin/env node
// Pruebas de host de Cebo (HTTP). Corren igual en Linux, WSL o Windows con
// MinGW. No sustituyen la compilación con Arduino IDE ni las pruebas en LAN real.
import {join} from 'node:path';
import {buildCebo, runCebo, repo} from './build.mjs';

const binary = buildCebo();
process.stdout.write(runCebo(binary, [join(repo, 'contracts/fixtures/v2')]));
console.log('Firmware Cebo HTTP: PASS (host; no physical radio, sockets or board claims)');
