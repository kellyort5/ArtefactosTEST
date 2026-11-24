// API HTTPS de estado (contrato de notificaciones). Solo `GET /v1/status`:
// el registro y la baja de instalaciones push pertenecen al cambio del envío
// push y hoy responden 404. Cada llamada exige `Authorization: Bearer <credencial
// del nodo>`; el nodo autorizado lo determina esa credencial, nunca el cuerpo ni
// la consulta. Los errores llevan cuerpo vacío, como en los fixtures del contrato.
import {createHash, timingSafeEqual} from 'node:crypto';
import {readFileSync} from 'node:fs';
import http from 'node:http';
import https from 'node:https';
import {NotificationContractError, decodeNotificationStatusRequest, decodeNotificationStatusResponse}
  from '@sentinel/contracts';

const MAX_BODY_BYTES = 4096;      // límite del contrato para cuerpos
const MAX_TOKEN_CHARS = 512;
const STATUS_PATH = '/v1/status';

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

export function createStatusApi({config, nodes, statusFor}) {
  const origins = new Set(config.corsOrigins);

  function cors(req, res) {
    const origin = req.headers.origin;
    if (origin === undefined) return true;
    res.setHeader('vary', 'Origin');
    if (!origins.has(origin)) return false;
    res.setHeader('access-control-allow-origin', origin);
    return true;
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

  function handle(req, res) {
    let url;
    try { url = new URL(req.url, 'http://status.invalid'); } catch { return reply(res, 400); }
    if (req.method === 'OPTIONS') {
      // Preflight: solo para los orígenes configurados, nunca con comodín.
      const origin = req.headers.origin;
      if (origin === undefined || !origins.has(origin)) return reply(res, 403);
      return reply(res, 204, undefined, {
        'access-control-allow-origin': origin, vary: 'Origin',
        'access-control-allow-methods': 'GET, OPTIONS',
        'access-control-allow-headers': 'Authorization', 'access-control-max-age': '600',
      });
    }
    if (url.pathname !== STATUS_PATH) return reply(res, 404);
    if (req.method !== 'GET') return reply(res, 405, undefined, {allow: 'GET, OPTIONS'});
    cors(req, res); // sin cabecera para un origen no configurado: el navegador lo bloquea

    // GET y DELETE no admiten cuerpo.
    const length = req.headers['content-length'];
    if (req.headers['transfer-encoding'] !== undefined || (length !== undefined && length !== '0')) {
      const size = Number(length);
      return reply(res, Number.isFinite(size) && size > MAX_BODY_BYTES ? 413 : 400);
    }

    const device = authenticate(req.headers.authorization, nodes);
    if (!device) return reply(res, 401, undefined, {'www-authenticate': 'Bearer'});

    // Consulta: exactamente un `device_id`, sin repeticiones ni parámetros desconocidos.
    const keys = [...url.searchParams.keys()];
    if (keys.length !== 1 || keys[0] !== 'device_id' || url.searchParams.getAll('device_id').length !== 1)
      return reply(res, 400);
    try {
      decodeNotificationStatusRequest(JSON.stringify({device_id: url.searchParams.get('device_id')}), device);
    } catch (error) {
      if (error instanceof NotificationContractError)
        return reply(res, error.code === 'unauthorized_device' ? 403 : 400);
      return reply(res, 400);
    }

    const body = {device_id: device, channels: statusFor(device)};
    try {
      // La respuesta se valida con el mismo contrato antes de enviarse.
      decodeNotificationStatusResponse(JSON.stringify(body), device);
    } catch {
      return reply(res, 500);
    }
    return reply(res, 200, body);
  }

  function listener(req, res) {
    try { handle(req, res); } catch { if (!res.headersSent) reply(res, 500); else res.end(); }
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
