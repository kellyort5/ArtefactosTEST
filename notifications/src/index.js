// Punto de entrada: `NOTIFY_CONFIG=/ruta/privada/notifications.json node src/index.js`.
import {ConfigError, loadConfig} from './config.js';
import {createService, jsonLog} from './service.js';

let config;
try {
  config = loadConfig();
} catch (error) {
  // Los mensajes de ConfigError nombran el campo, nunca su valor.
  jsonLog({level: 'error', msg: 'config_invalid', detail: error instanceof ConfigError ? error.message : 'error de lectura'});
  process.exit(1);
}

const service = createService(config);
try {
  const {port} = await service.start();
  jsonLog({level: 'info', msg: 'started', status_api_port: port, nodes: config.nodes.length,
    webhook: Boolean(config.webhook), push: false});
} catch {
  jsonLog({level: 'error', msg: 'start_failed'});
  process.exit(1);
}

let stopping = false;
async function stop(signal) {
  if (stopping) return;
  stopping = true;
  jsonLog({level: 'info', msg: 'stopping', signal});
  await service.stop();
  process.exit(0);
}
process.on('SIGINT', () => stop('SIGINT'));
process.on('SIGTERM', () => stop('SIGTERM'));
