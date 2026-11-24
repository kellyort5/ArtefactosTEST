// Carga y validación de la configuración privada del componente. El archivo de
// configuración vive fuera de Git (ver README). Los mensajes de error nombran el
// campo, nunca su valor: una clave, un bearer o un destino mal escritos no deben
// acabar en un log.
import {readFileSync} from 'node:fs';
import {isAbsolute, resolve} from 'node:path';

export class ConfigError extends Error {
  constructor(field, problem) {
    super(`configuración inválida: ${field}: ${problem}`);
    this.name = 'ConfigError';
  }
}

const ID = /^[A-Za-z0-9._:-]{1,64}$/;
const ORIGIN = new RegExp('^[a-z][a-z0-9+.-]{0,15}://[a-z0-9.-]{1,253}(:[0-9]{1,5})?$');
const SEVERITIES = ['info', 'warning', 'critical'];
export const SEVERITY_RANK = Object.freeze({info: 0, warning: 1, critical: 2});
const FORBIDDEN_HEADERS = new Set(['host', 'content-length', 'content-type', 'transfer-encoding',
  'connection', 'user-agent', 'idempotency-key']);
const LOOPBACK = new Set(['localhost', '127.0.0.1', '::1', '[::1]']);

const isObject = value => value !== null && typeof value === 'object' && !Array.isArray(value);
function only(object, field, allowed) {
  for (const key of Object.keys(object))
    if (!allowed.includes(key)) throw new ConfigError(`${field}.${key}`, 'campo desconocido');
}
function integer(value, field, min, max, fallback) {
  if (value === undefined) return fallback;
  if (!Number.isInteger(value) || value < min || value > max)
    throw new ConfigError(field, `entero entre ${min} y ${max}`);
  return value;
}
function text(value, field, max, {optional = false} = {}) {
  if (value === undefined || value === null) {
    if (optional) return null;
    throw new ConfigError(field, 'obligatorio');
  }
  if (typeof value !== 'string' || !value.length || value.length > max || /[\u0000-\u001f\u007f]/.test(value))
    throw new ConfigError(field, `texto de 1 a ${max} caracteres sin controles`);
  return value;
}
function bool(value, field, fallback) {
  if (value === undefined) return fallback;
  if (typeof value !== 'boolean') throw new ConfigError(field, 'booleano');
  return value;
}
function path(value, field, base) {
  if (value === undefined || value === null) return null;
  const given = text(value, field, 512);
  return isAbsolute(given) ? given : resolve(base, given);
}
function url(value, field, schemes) {
  const raw = text(value, field, 2048);
  let parsed;
  try { parsed = new URL(raw); } catch { throw new ConfigError(field, 'URL inválida'); }
  if (!schemes.includes(parsed.protocol)) throw new ConfigError(field, `esquema ${schemes.join(' o ')}`);
  if (parsed.username || parsed.password) throw new ConfigError(field, 'sin credenciales en la URL');
  if (parsed.hash) throw new ConfigError(field, 'sin fragmento');
  return parsed;
}

/** Valida un objeto de configuración ya leído. `base` resuelve rutas relativas. */
export function parseConfig(raw, {base = process.cwd(), env = {}} = {}) {
  if (!isObject(raw)) throw new ConfigError('raíz', 'se esperaba un objeto');
  only(raw, 'raíz', ['mqtt', 'nodes', 'alerts', 'webhook', 'push', 'status_api', 'state_dir']);

  // MQTT: broker anónimo del MVP en LAN de confianza (sin usuario ni contraseña).
  const mqtt = raw.mqtt;
  if (!isObject(mqtt)) throw new ConfigError('mqtt', 'obligatorio');
  only(mqtt, 'mqtt', ['url', 'client_id']);
  const mqttUrl = url(mqtt.url, 'mqtt.url', ['mqtt:']);
  const clientId = mqtt.client_id === undefined ? 'sentinel-notifications' : text(mqtt.client_id, 'mqtt.client_id', 64);
  if (!ID.test(clientId)) throw new ConfigError('mqtt.client_id', 'caracteres A-Z a-z 0-9 . _ : -');

  // Nodos autorizados: el bearer de cada uno se guarda como hash SHA-256.
  if (!Array.isArray(raw.nodes) || !raw.nodes.length || raw.nodes.length > 16)
    throw new ConfigError('nodes', 'lista de 1 a 16 nodos');
  const seen = new Set();
  const nodes = raw.nodes.map((node, index) => {
    const field = `nodes[${index}]`;
    if (!isObject(node)) throw new ConfigError(field, 'se esperaba un objeto');
    only(node, field, ['device_id', 'token_sha256']);
    const deviceId = text(node.device_id, `${field}.device_id`, 64);
    if (!ID.test(deviceId)) throw new ConfigError(`${field}.device_id`, 'ID de nodo inválido');
    if (seen.has(deviceId)) throw new ConfigError(`${field}.device_id`, 'repetido');
    seen.add(deviceId);
    const hash = text(node.token_sha256, `${field}.token_sha256`, 64);
    if (!/^[0-9a-f]{64}$/.test(hash)) throw new ConfigError(`${field}.token_sha256`, 'SHA-256 en hexadecimal minúscula');
    return Object.freeze({deviceId, tokenSha256: hash});
  });
  if (new Set(nodes.map(n => n.tokenSha256)).size !== nodes.length)
    throw new ConfigError('nodes', 'dos nodos comparten credencial');

  // Alertas: qué eventos se envían y la ventana de control de repeticiones.
  const alerts = raw.alerts ?? {};
  if (!isObject(alerts)) throw new ConfigError('alerts', 'se esperaba un objeto');
  only(alerts, 'alerts', ['min_severity', 'include_simulated', 'repeat_window_ms']);
  const minSeverity = alerts.min_severity === undefined ? 'warning' : alerts.min_severity;
  if (!SEVERITIES.includes(minSeverity)) throw new ConfigError('alerts.min_severity', SEVERITIES.join(' | '));

  // Webhook: un único destino fijo, HTTPS salvo bucle local en pruebas.
  let webhook = null;
  if (raw.webhook !== undefined && raw.webhook !== null) {
    const w = raw.webhook;
    if (!isObject(w)) throw new ConfigError('webhook', 'se esperaba un objeto');
    only(w, 'webhook', ['url', 'timeout_ms', 'auth_header', 'ca_file', 'allow_insecure_loopback']);
    const allowInsecure = bool(w.allow_insecure_loopback, 'webhook.allow_insecure_loopback', false);
    const target = url(w.url, 'webhook.url', allowInsecure ? ['https:', 'http:'] : ['https:']);
    if (target.protocol === 'http:' && !LOOPBACK.has(target.hostname))
      throw new ConfigError('webhook.url', 'http solo se admite en bucle local');
    let header = null;
    if (w.auth_header !== undefined && w.auth_header !== null) {
      const h = w.auth_header;
      if (!isObject(h)) throw new ConfigError('webhook.auth_header', 'se esperaba un objeto');
      only(h, 'webhook.auth_header', ['name', 'value']);
      const name = text(h.name, 'webhook.auth_header.name', 64);
      if (!/^[A-Za-z0-9-]+$/.test(name) || FORBIDDEN_HEADERS.has(name.toLowerCase()))
        throw new ConfigError('webhook.auth_header.name', 'nombre de cabecera no permitido');
      // El secreto puede venir del entorno para no escribirlo en el archivo.
      const value = env.NOTIFY_WEBHOOK_AUTH_VALUE ?? h.value;
      header = Object.freeze({name, value: text(value, 'webhook.auth_header.value', 2048)});
    }
    webhook = Object.freeze({
      url: target.href,
      timeoutMs: integer(w.timeout_ms, 'webhook.timeout_ms', 1000, 30000, 5000),
      authHeader: header,
      caFile: path(w.ca_file, 'webhook.ca_file', base),
    });
  }

  // Push (FCM): la credencial de la cuenta de servicio es un archivo privado fuera de Git.
  let push = null;
  if (raw.push !== undefined && raw.push !== null) {
    const p = raw.push;
    if (!isObject(p)) throw new ConfigError('push', 'se esperaba un objeto');
    only(p, 'push', ['credential_file', 'ttl_seconds']);
    const credentialFile = path(p.credential_file, 'push.credential_file', base);
    if (!credentialFile) throw new ConfigError('push.credential_file', 'obligatorio');
    push = Object.freeze({credentialFile, ttlMs: integer(p.ttl_seconds, 'push.ttl_seconds', 60, 86400, 3600) * 1000});
  }

  // API de estado: HTTPS con CORS explícito. Sin TLS solo en bucle local.
  const api = raw.status_api ?? {};
  if (!isObject(api)) throw new ConfigError('status_api', 'se esperaba un objeto');
  only(api, 'status_api', ['host', 'port', 'tls', 'cors_origins', 'allow_insecure_loopback']);
  const host = api.host === undefined ? '0.0.0.0' : text(api.host, 'status_api.host', 255);
  const insecure = bool(api.allow_insecure_loopback, 'status_api.allow_insecure_loopback', false);
  let tls = null;
  if (api.tls !== undefined && api.tls !== null) {
    if (!isObject(api.tls)) throw new ConfigError('status_api.tls', 'se esperaba un objeto');
    only(api.tls, 'status_api.tls', ['cert_file', 'key_file']);
    tls = Object.freeze({certFile: path(api.tls.cert_file, 'status_api.tls.cert_file', base),
      keyFile: path(api.tls.key_file, 'status_api.tls.key_file', base)});
    if (!tls.certFile || !tls.keyFile) throw new ConfigError('status_api.tls', 'cert_file y key_file obligatorios');
  }
  if (!tls && !(insecure && LOOPBACK.has(host)))
    throw new ConfigError('status_api.tls', 'HTTPS obligatorio (sin TLS solo en bucle local con allow_insecure_loopback)');
  const origins = api.cors_origins ?? [];
  if (!Array.isArray(origins) || origins.length > 16) throw new ConfigError('status_api.cors_origins', 'lista de hasta 16 orígenes');
  const corsOrigins = origins.map((origin, index) => {
    const field = `status_api.cors_origins[${index}]`;
    const given = text(origin, field, 256);
    // Un origen es esquema://host[:puerto] en minúsculas, sin ruta ni comodines. No se usa
    // URL.origin porque devuelve 'null' para esquemas como capacitor://localhost.
    if (!ORIGIN.test(given))
      throw new ConfigError(field, 'origen exacto esquema://host[:puerto]');
    return given;
  });

  return Object.freeze({
    mqtt: Object.freeze({url: mqttUrl.href, clientId}),
    nodes: Object.freeze(nodes),
    alerts: Object.freeze({
      minSeverity,
      includeSimulated: bool(alerts.include_simulated, 'alerts.include_simulated', false),
      repeatWindowMs: integer(alerts.repeat_window_ms, 'alerts.repeat_window_ms', 0, 3600000, 30000),
    }),
    webhook,
    push,
    statusApi: Object.freeze({
      host,
      port: integer(api.port, 'status_api.port', 1, 65535, 8443),
      tls,
      corsOrigins: Object.freeze(corsOrigins),
    }),
    stateDir: path(raw.state_dir ?? './state', 'state_dir', base),
  });
}

/** Lee el archivo apuntado por NOTIFY_CONFIG (o la ruta dada). */
export function loadConfig(file = process.env.NOTIFY_CONFIG, env = process.env) {
  if (!file) throw new ConfigError('NOTIFY_CONFIG', 'define la ruta del archivo de configuración privado');
  const absolute = resolve(file);
  let raw;
  try { raw = JSON.parse(readFileSync(absolute, 'utf8')); }
  catch { throw new ConfigError('archivo', 'no se pudo leer o no es JSON válido'); }
  return parseConfig(raw, {base: resolve(absolute, '..'), env});
}
