// Validate a complete private lab configuration in RAM; output sizes only.
import { readFile } from 'node:fs/promises';
import { encodeMessage } from '@sentinel/contracts';
try {
  const labPath = new URL('../.secrets/', import.meta.url);
  const lab = JSON.parse(await readFile(new URL('clients.json', labPath), 'utf8'));
  const command = {
    version: 1, type: 'command', device_id: lab.node_id, boot_id: 'boot-lab',
    message_id: 'configure-lab', seq: 1, uptime_ms: 1000, timestamp: null, time_quality: 'unknown',
    payload: {
      command_id: 'configure-lab', target_boot_id: 'boot-lab', expected_revision: 0,
      expires_at_uptime_ms: 31000, operation: 'configure_connectivity',
      args: {
        ssid: 'LAB-EXAMPLE', wifi_password: 'example-only', mqtt_uri: 'mqtt://192.0.2.10:1883',
        mqtt_username: lab.accounts.node.username, mqtt_password: lab.accounts.node.password,
      },
    },
  };
  const bytes = encodeMessage(command);
  console.log(`PASS private provisioning contract: message=${bytes.length} bytes`);
} catch {
  console.error('FAIL provisioning: revisar material privado y límites del contrato; no se imprime configuración.');
  process.exitCode = 1;
}
