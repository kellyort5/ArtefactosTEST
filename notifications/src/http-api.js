// API HTTPS del componente (contrato de notificaciones):
//   GET    /v1/status?device_id=…            estado de los canales del nodo
//   POST   /v1/registrations                 alta o renovación de una instalación push
//   DELETE /v1/registrations/<installation>  baja idempotente
// Cada llamada exige `Authorization: Bearer <credencial del nodo>`; el nodo
// autorizado lo determina esa credencial, nunca el cuerpo, la ruta ni la consulta.
// Los errores llevan cuerpo vacío, como en los fixtures del contrato, y ninguna
// respuesta contiene tokens, destinos ni mensajes del proveedor.
import {createHash, timingSafeEqual} from 'node:crypto';
import {readFileSync} from 'node:fs';
import http from 'node:http';
import https from 'node:https';
import {NotificationContractError, decodeNotificationRegistrationRequest, decodeNotificationRegistrationResponse,
  decodeNotificationStatusRequest, decodeNotificationStatusResponse, decodeNotificationUnregistrationRequest,
  decodeNotificationUnregistrationResponse} from '@sentinel/contracts';

const MAX_BODY_BYTES = 4096;      // límite del contrato para cuerpos
const MAX_TOKEN_CHARS = 512;
const STATUS_PATH = '/v1/status';
const REGISTRATIONS_PATH = '/v1/registrations';
const INSTALLATION_PATH = /^\/v1\/registrations\/([A-Za-z0-9._:-]{1,64})$/;

export const sha256 = text => createHash('sha256').update(text).digest();

/** Devuelve el device_id al que pertenece el bearer, o null. Comparación en tiempo constante. */
export function authenticate(header, nodes) {
  if (typeof header !== 'string') return null;
  const match = /^Bearer ([\x21-\x7e]{1,512})$/.exec(header);
  if (!match || match[1].length > MAX_TOKEN_CHARS) return null;
  const given = sha256(match[1]);
  let found = null;
  // Se recorren todos los nodos para no revelar por tiempo cuál coincidió.
  for (const node of nodes)
    if (timingSafeEqual(given, Buffer.from(node.tokenSha256, 'hex'))) found = node.deviceId;
  return found;
}

const statusOf = error => {
  if (!(error instanceof NotificationContractError)) return 400;
  if (error.code === 'unauthorized_device') return 403;
  return error.code === 'notification_too_large' ? 413 : 400;
};

/**
 * @param config        statusApi de la configuración
 * @param nodes         nodos autorizados (hash de su credencial)
 * @param statusFor     device → {webhook, push}
 * @param registrations {configured, register(device, installation, token), remove(device, installation), save()}
 */
export function createStatusApi({config, nodes, statusFor, registrations}) {
  const origins = new Set(config.corsOrigins);

  function cors(req, res) {
    const origin = req.headers.origin;
    if (origin === undefined) return;
    res.setHeader('vary', 'Origin');
    if (origins.has(origin)) res.setHeader('access-control-allow-origin', origin);
  }
  function reply(res, status, body, headers = {}) {
    const payload = body === undefined ? undefined : JSON.stringify(body);
    res.writeHead(status, {
      'cache-control': 'no-store', 'x-content-type-options': 'nosniff',
      ...(payload === undefined ? {} : {'content-type': 'application/json', 'content-length': Buffer.byteLength(payload)}),
      ...headers,
    });
    res.end(payload);
  }
  // Un cuerpo que no se va a leer se descarta al terminar la respuesta.
  const refuse = (req, res, status, headers) => {
    reply(res, status, undefined, {connection: 'close', ...headers});
    res.once('finish', () => req.destroy());
  };

  function readBody(req) {
    return new Promise((resolve, reject) => {
      const chunks = [];
      let size = 0;
      req.on('data', chunk => {
        size += chunk.length;
        if (size > MAX_BODY_BYTES) { reject(new NotificationContractError('notification_too_large')); return; }
        chunks.push(chunk);
      });
      req.on('end', () => resolve(Buffer.concat(chunks)));
      req.on('error', reject);
    });
  }

  async function register(req, res, device) {
    if (!registrations?.configured) return reply(res, 503); // el canal push no está configurado
    if (!/^application\/json(?:\s*;\s*charset=utf-8)?$/i.test(req.headers['content-type'] ?? '')) return refuse(req, res, 400);
    const length = Number(req.headers['content-length']);
    if (Number.isFinite(length) && length > MAX_BODY_BYTES) return refuse(req, res, 413);
    let request;
    try {
      request = decodeNotificationRegistrationRequest(await readBody(req), device);
    } catch (error) {
      const status = statusOf(error);
      return status === 413 ? refuse(req, res, 413) : reply(res, status);
    }
    if (registrations.register(device, request.installation_id, request.token) === 'full') return reply(res, 409);
    if (!registrations.save()) return reply(res, 500); // sin confirmar en disco no se acepta
    const body = {device_id: device, installation_id: request.installation_id, status: 'registered'};
    decodeNotificationRegistrationResponse(JSON.stringify(body), device); // misma validación que consume la app
    return reply(res, 200, body);
  }

  function unregister(req, res, device, installation) {
    try {
      decodeNotificationUnregistrationRequest(JSON.stringify({device_id: device, installation_id: installation}), device);
    } catch { return reply(res, 400); }
    // Solo dentro del nodo autenticado: nunca se busca una instalación de otro nodo.
    if (registrations.remove(device, installation) && !registrations.save()) return reply(res, 500);
    const body = {device_id: device, installation_id: installation, status: 'removed'};
    decodeNotificationUnregistrationResponse(JSON.stringify(body), device);
    return reply(res, 200, body); // idempotente: igual exista o no, sin revelar nada de otros nodos
  }

  function status(req, res, device, url) {
    // Consulta: exactamente un `device_id`, sin repeticiones ni parámetros desconocidos.
    const keys = [...url.searchParams.keys()];
    if (keys.length !== 1 || keys[0] !== 'device_id' || url.searchParams.getAll('device_id').length !== 1)
      return reply(res, 400);
    try {
      decodeNotificationStatusRequest(JSON.stringify({device_id: url.searchParams.get('device_id')}), device);
    } catch (error) { return reply(res, statusOf(error) === 403 ? 403 : 400); }
    const body = {device_id: device, channels: statusFor(device)};
    try {
      // La respuesta se valida con el mismo contrato antes de enviarse.
      decodeNotificationStatusResponse(JSON.stringify(body), device);
    } catch { return reply(res, 500); }
    return reply(res, 200, body);
  }

  async function handle(req, res) {
    let url;
    try { url = new URL(req.url, 'http://status.invalid'); } catch { return reply(res, 400); }
    const installation = INSTALLATION_PATH.exec(url.pathname)?.[1];
    const allowed = url.pathname === STATUS_PATH ? 'GET' : url.pathname === REGISTRATIONS_PATH ? 'POST'
      : installation ? 'DELETE' : null;
    if (req.method === 'OPTIONS') {
      // Preflight: solo para los orígenes configurados, nunca con comodín.
      const origin = req.headers.origin;
      if (origin === undefined || !origins.has(origin)) return reply(res, 403);
      return reply(res, 204, undefined, {
        'access-control-allow-origin': origin, vary: 'Origin',
        'access-control-allow-methods': 'GET, POST, DELETE, OPTIONS',
        'access-control-allow-headers': 'Authorization, Content-Type', 'access-control-max-age': '600',
      });
    }
    if (!allowed) return reply(res, 404);
    if (req.method !== allowed) return reply(res, 405, undefined, {allow: `${allowed}, OPTIONS`});
    cors(req, res); // sin cabecera para un origen no configurado: el navegador lo bloquea

    // GET y DELETE no admiten cuerpo; se rechaza antes de autenticar y no se lee.
    if (allowed !== 'POST') {
      const length = req.headers['content-length'];
      if (req.headers['transfer-encoding'] !== undefined || (length !== undefined && length !== '0')) {
        const size = Number(length);
        return refuse(req, res, Number.isFinite(size) && size > MAX_BODY_BYTES ? 413 : 400);
      }
    }
    const device = authenticate(req.headers.authorization, nodes);
    if (!device) return reply(res, 401, undefined, {'www-authenticate': 'Bearer'});
    if (allowed === 'POST') return register(req, res, device);
    if (allowed === 'DELETE') {
      if (url.search) return reply(res, 400);
      return unregister(req, res, device, installation);
    }
    return status(req, res, device, url);
  }

  function listener(req, res) {
    Promise.resolve().then(() => handle(req, res)).catch(() => {
      if (!res.headersSent) reply(res, 500); else res.end();
    });
  }

  function build() {
    const server = config.tls
      ? https.createServer({cert: readFileSync(config.tls.certFile), key: readFileSync(config.tls.keyFile),
        minVersion: 'TLSv1.2'}, listener)
      : http.createServer(listener);
    server.headersTimeout = 10000;
    server.requestTimeout = 15000;
    server.keepAliveTimeout = 5000;
    server.maxHeadersCount = 50;
    server.maxConnections = 32;
    server.on('clientError', (error, socket) => {
      if (socket.writable) socket.end('HTTP/1.1 400 Bad Request\r\nconnection: close\r\ncontent-length: 0\r\n\r\n');
      else socket.destroy();
    });
    return server;
  }
  return {build, handle: listener};
}
