#!/usr/bin/env node
// Genera la credencial de un nodo para la API de estado. El bearer (`token`) se
// entrega UNA vez a quien consume la API (la app); en la configuración privada
// solo se escribe su hash. No guardar el bearer en Git, registros ni capturas.
import {createHash, randomBytes} from 'node:crypto';

const token = randomBytes(32).toString('base64url');
const hash = createHash('sha256').update(token).digest('hex');
process.stdout.write(`token (entregar al cliente, no se vuelve a mostrar):\n${token}\n\n` +
  `token_sha256 (copiar a nodes[].token_sha256 de la configuración privada):\n${hash}\n`);
