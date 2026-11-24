// Cliente del webhook: un único destino fijo por HTTPS con verificación de
// certificado, tiempo de espera, sin seguir redirecciones y sin conservar ni
// registrar nada de la respuesta. 2xx significa «aceptado por el destino»,
// nunca «leído». Los fallos se reducen a códigos del contrato; no se devuelve
// ningún mensaje de error de red, URL ni cabecera.
import {readFileSync} from 'node:fs';
import http from 'node:http';
import https from 'node:https';

const MAX_BODY_BYTES = 16 * 1024;     // una alerta ocupa unos cientos de bytes
const MAX_DRAIN_BYTES = 64 * 1024;    // respuesta que se descarta como máximo

/** Clasifica un estado HTTP en el resultado de entrega. */
export function classify(status, retryAfterHeader) {
  if (status >= 200 && status < 300) return {ok: true};
  if (status === 429) {
    const seconds = Number(retryAfterHeader);
    return {ok: false, error: 'rate_limited', retryAfterMs: Number.isFinite(seconds) && seconds > 0 ? seconds * 1000 : 0};
  }
  if (status === 408 || status >= 500) return {ok: false, error: 'provider_unavailable'};
  return {ok: false, error: 'provider_rejected'}; // 3xx (sin seguir) y 4xx
}

/**
 * @param config  {url, timeoutMs, authHeader:{name,value}|null, caFile}
 * @param request función (opciones, callback) como https.request; inyectable en pruebas
 */
export function createWebhook(config, {request} = {}) {
  const target = new URL(config.url);
  const secure = target.protocol === 'https:';
  const send = request ?? (secure ? https.request : http.request);
  const ca = config.caFile ? readFileSync(config.caFile) : undefined;
  const agent = new (secure ? https : http).Agent({keepAlive: false, maxSockets: 1});

  return {
    configured: true,
    send(alert, attempt) {
      const body = Buffer.from(JSON.stringify(alert));
      if (body.length > MAX_BODY_BYTES) return Promise.resolve({ok: false, error: 'internal_error'});
      const headers = {
        'content-type': 'application/json',
        'content-length': String(body.length),
        'user-agent': 'sentinel-notifications/1',
        // Permite al destino deduplicar si recibe dos veces la misma alerta.
        'idempotency-key': `${alert.device_id}/${alert.event_id}`.slice(0, 140),
        'x-sentinel-attempt': String(attempt),
      };
      if (config.authHeader) headers[config.authHeader.name] = config.authHeader.value;
      return new Promise(resolve => {
        let settled = false;
        const done = result => { if (!settled) { settled = true; resolve(result); } };
        const req = send({
          protocol: target.protocol, hostname: target.hostname.replace(/^\[|\]$/g, ''),
          port: target.port || undefined, path: `${target.pathname}${target.search}`,
          method: 'POST', headers, agent, timeout: config.timeoutMs,
          // Verificación completa y TLS moderno; nunca se desactiva.
          ...(secure ? {rejectUnauthorized: true, minVersion: 'TLSv1.2', ca} : {}),
        }, res => {
          let drained = 0;
          res.on('data', chunk => { drained += chunk.length; if (drained > MAX_DRAIN_BYTES) res.destroy(); });
          res.on('end', () => done(classify(res.statusCode, res.headers['retry-after'])));
          res.on('close', () => done(classify(res.statusCode, res.headers['retry-after'])));
          res.on('error', () => done({ok: false, error: 'provider_unavailable'}));
        });
        // Un fallo de red, de TLS o un tiempo agotado se trata como destino no disponible.
        req.on('timeout', () => { req.destroy(); done({ok: false, error: 'provider_unavailable'}); });
        req.on('error', () => done({ok: false, error: 'provider_unavailable'}));
        req.end(body);
      });
    },
    close() { agent.destroy(); },
  };
}
