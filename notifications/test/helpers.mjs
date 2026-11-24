// Utilidades de las pruebas: fixtures del contrato, reloj controlado, receptor
// de webhook local y configuración de ejemplo. Todos los valores son ficticios.
import {createHash} from 'node:crypto';
import {mkdtempSync, readFileSync, rmSync} from 'node:fs';
import http from 'node:http';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {parseConfig} from '../src/config.js';

export const FIXTURES = new URL('../../contracts/fixtures/v2/', import.meta.url);
export const fixture = path => JSON.parse(readFileSync(new URL(path, FIXTURES), 'utf8'));
export const sha256hex = text => createHash('sha256').update(text).digest('hex');

export const NODE_A = 'node-example';
export const NODE_B = 'node-other';
export const TOKEN_A = 'FICTIONAL-TOKEN-NODE-A-0000000000000000';
export const TOKEN_B = 'FICTIONAL-TOKEN-NODE-B-1111111111111111';
export const WEBHOOK_SECRET = 'FICTIONAL-WEBHOOK-SECRET-7f3a91';

export function tempDir() {
  const dir = mkdtempSync(join(tmpdir(), 'sentinel-notify-'));
  return {dir, cleanup: () => rmSync(dir, {recursive: true, force: true})};
}

/** Reloj controlado: now() devuelve `t`; advance() lo mueve. */
export function clock(start = Date.UTC(2026, 10, 24, 12, 0, 0)) {
  const c = {t: start, now: () => c.t, advance(ms) { c.t += ms; return c.t; }};
  return c;
}

/** Configuración válida de pruebas; `webhookPort` apunta al receptor local. */
export function makeConfig({webhookPort = 1, stateDir, overrides = {}, env = {}} = {}) {
  return parseConfig({
    mqtt: {url: 'mqtt://127.0.0.1:1883'},
    nodes: [{device_id: NODE_A, token_sha256: sha256hex(TOKEN_A)}, {device_id: NODE_B, token_sha256: sha256hex(TOKEN_B)}],
    alerts: {min_severity: 'warning', include_simulated: true, repeat_window_ms: 30000},
    webhook: {url: `http://127.0.0.1:${webhookPort}/hook`, timeout_ms: 2000, allow_insecure_loopback: true,
      auth_header: {name: 'Authorization', value: `Bearer ${WEBHOOK_SECRET}`}},
    status_api: {host: '127.0.0.1', port: 8443, allow_insecure_loopback: true, cors_origins: ['https://localhost']},
    state_dir: stateDir ?? tmpdir(),
    ...overrides,
  }, {env});
}

/** Evento Cebo v2 del contrato con identidad y severidad a medida (sigue siendo válido). */
export function ceboEvent({device = NODE_A, id = 'boot-1:1', severity = 'warning', ip = '192.0.2.2',
  protocol = 'http', origin = 'real'} = {}) {
  const base = fixture(protocol === 'http' ? 'events/cebo.http-repeated-auth.valid.json'
    : 'events/cebo.telnet-repeated-auth.valid.json');
  const suspicion = {info: 'informational', warning: 'medium', critical: 'high'}[severity];
  const evidence = {info: ['auth_attempt'], warning: ['auth_attempt', 'repeated_auth'],
    critical: ['auth_attempt', 'rate_threshold']}[severity];
  return {
    ...base, device_id: device, message_id: id,
    payload: {...base.payload, severity, origin,
      details: {...base.payload.details, source_ipv4: ip, suspicion, evidence,
        count: severity === 'info' ? 1 : base.payload.details.count,
        window_ms: severity === 'info' ? 1000 : 60000}},
  };
}
export const topicFor = (device = NODE_A, version = 2) => `sentinel/v${version}/nodes/${device}/events`;
export const bytes = value => Buffer.from(typeof value === 'string' ? value : JSON.stringify(value));

/** Receptor de webhook en bucle local; `respond` decide el estado de cada petición. */
export async function webhookServer(respond = () => ({status: 200})) {
  const requests = [];
  const server = http.createServer((req, res) => {
    const chunks = [];
    req.on('data', c => chunks.push(c));
    req.on('end', () => {
      const body = Buffer.concat(chunks).toString('utf8');
      const record = {method: req.method, url: req.url, headers: req.headers, body, json: safeJson(body)};
      requests.push(record);
      const out = respond(record, requests.length);
      if (out.hang) return; // no responde: prueba de tiempo agotado
      res.writeHead(out.status ?? 200, out.headers ?? {});
      res.end(out.body ?? '');
    });
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  return {requests, port: server.address().port, server,
    close: () => new Promise(resolve => { server.close(() => resolve()); server.closeAllConnections?.(); })};
}
function safeJson(text) { try { return JSON.parse(text); } catch { return null; } }

/** Espera a que `predicate()` sea verdadero o falla tras `ms`. */
export async function until(predicate, ms = 3000, step = 10) {
  const limit = Date.now() + ms;
  while (Date.now() < limit) {
    if (await predicate()) return;
    await new Promise(resolve => setTimeout(resolve, step));
  }
  throw new Error('until: condición no cumplida a tiempo');
}
export const noLog = () => {};
export function recordingLog() {
  const entries = [];
  const log = entry => entries.push(entry);
  log.entries = entries;
  return log;
}
