import assert from 'node:assert/strict';
import {test} from 'node:test';
import {BACKOFF_MAX_MS, Channel, backoff} from '../src/channel.js';
import {LIMITS, StateStore} from '../src/state.js';
import {NODE_A, NODE_B, clock, recordingLog, tempDir} from './helpers.mjs';

const alertOf = (id, device = NODE_A) => ({event_key: `${device}/${id}`, body: {device_id: device, event_id: id}});

function rig(results = []) {
  const t = tempDir();
  const c = clock();
  const log = recordingLog();
  const store = new StateStore({dir: t.dir, now: c.now, log});
  store.load();
  const calls = [];
  const sender = {send: async (alert, attempt) => { calls.push({id: alert.event_id, attempt, at: c.now()});
    const next = results.length ? results.shift() : {ok: true};
    if (next instanceof Error) throw next;
    return next; }};
  const channel = new Channel({name: 'webhook', store, sender, now: c.now, log});
  return {t, c, store, channel, calls, log, results, sender};
}

test('la espera entre reintentos crece de 1 s a 30 s y no más', () => {
  assert.deepEqual([1, 2, 3, 4, 5, 6, 7, 20].map(backoff), [1000, 2000, 4000, 8000, 16000, 30000, 30000, 30000]);
  assert.equal(BACKOFF_MAX_MS, 30000);
});

test('una alerta se entrega, se retira de la cola y se registra como aceptada, no como leída', async () => {
  const r = rig();
  r.channel.enqueue(alertOf('a:1'));
  assert.equal(r.channel.status(NODE_A).pending, 1);
  assert.equal(await r.channel.process(), 1);
  const status = r.channel.status(NODE_A);
  assert.equal(status.pending, 0);
  assert.deepEqual(status.last_delivery, {event_id: 'a:1', timestamp: new Date(r.c.now()).toISOString(),
    status: 'provider_accepted', read_status: 'unknown', error: null});
  r.t.cleanup();
});

test('un destino caído se reintenta con espera creciente y recupera sin perder ni duplicar', async () => {
  const r = rig([{ok: false, error: 'provider_unavailable'}, {ok: false, error: 'provider_unavailable'},
    {ok: false, error: 'provider_unavailable'}]);
  r.channel.enqueue(alertOf('a:1'));
  r.channel.enqueue(alertOf('a:2'));
  const start = r.c.now();
  assert.equal(await r.channel.process(), 0);            // fallo 1 → espera 1 s
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'provider_unavailable');
  assert.equal(r.channel.status(NODE_A).pending, 2);
  r.c.advance(999);
  assert.equal(await r.channel.process(), 0);            // todavía en espera: no se intenta
  assert.equal(r.calls.length, 1);
  r.c.advance(1);
  assert.equal(await r.channel.process(), 0);            // fallo 2 → espera 2 s
  r.c.advance(1999);
  await r.channel.process();
  assert.equal(r.calls.length, 2);
  r.c.advance(1);
  assert.equal(await r.channel.process(), 0);            // fallo 3 → espera 4 s
  r.c.advance(4000);
  assert.equal(await r.channel.process(), 2);            // el destino vuelve: se entregan las dos, en orden
  assert.deepEqual(r.calls.map(c => c.id), ['a:1', 'a:1', 'a:1', 'a:1', 'a:2']);
  assert.deepEqual(r.calls.map(c => c.attempt).slice(0, 4), [1, 2, 3, 4]);
  assert.equal(r.channel.status(NODE_A).pending, 0);
  assert.equal(r.channel.failures, 0); // el éxito reinicia la espera del canal
  assert.equal(r.calls[0].at, start);
  r.t.cleanup();
});

test('Retry-After del destino se respeta con el tope de 30 s', async () => {
  const r = rig([{ok: false, error: 'rate_limited', retryAfterMs: 20000}, {ok: false, error: 'rate_limited', retryAfterMs: 999999}]);
  r.channel.enqueue(alertOf('a:1'));
  await r.channel.process();
  r.c.advance(19999);
  await r.channel.process();
  assert.equal(r.calls.length, 1);
  r.c.advance(1);
  await r.channel.process(); // segundo fallo con Retry-After enorme → 30 s como máximo
  r.c.advance(29999);
  await r.channel.process();
  assert.equal(r.calls.length, 2);
  r.c.advance(1);
  assert.equal(await r.channel.process(), 1);
  r.t.cleanup();
});

test('una excepción del envío se trata como error interno, nunca como éxito ni con su mensaje', async () => {
  const r = rig([new Error('FICTIONAL-SECRET en el mensaje')]);
  r.channel.enqueue(alertOf('a:1'));
  assert.equal(await r.channel.process(), 0);
  const d = r.channel.status(NODE_A).last_delivery;
  assert.deepEqual([d.status, d.error], ['failed', 'internal_error']);
  assert.ok(!JSON.stringify([r.log.entries, r.channel.status(NODE_A)]).includes('FICTIONAL-SECRET'));
  r.t.cleanup();
});

test('lo pendiente vence a las 24 h y se informa como expirado', async () => {
  const r = rig();
  r.channel.enqueue(alertOf('a:1'));
  r.c.advance(LIMITS.outboxTtlMs - 1);
  r.channel.expire();
  assert.equal(r.channel.status(NODE_A).pending, 1);
  r.c.advance(1);
  assert.equal(await r.channel.process(), 0);
  assert.equal(r.calls.length, 0, 'no se intenta entregar lo vencido');
  assert.equal(r.channel.status(NODE_A).pending, 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'expired');
  assert.equal(r.channel.stats.expired, 1);
  assert.ok(r.log.entries.some(e => e.msg === 'alert_expired' && e.event_id === 'a:1'));
  r.t.cleanup();
});

test('con la cola llena se descarta la más antigua y se avisa', () => {
  const r = rig();
  for (let i = 0; i < LIMITS.outbox + 5; ++i) r.channel.enqueue(alertOf(`a:${i}`));
  assert.equal(r.channel.queue.length, LIMITS.outbox);
  assert.equal(r.channel.queue[0].alert.event_id, 'a:5');
  assert.equal(r.channel.stats.dropped, 5);
  assert.equal(r.channel.status(NODE_A).last_delivery.error, 'expired');
  assert.equal(r.log.entries.filter(e => e.msg === 'outbox_full_dropped').length, 5);
  r.t.cleanup();
});

test('encolar el mismo evento dos veces no lo duplica', () => {
  const r = rig();
  r.channel.enqueue(alertOf('a:1'));
  r.channel.enqueue(alertOf('a:1'));
  assert.equal(r.channel.queue.length, 1);
  r.t.cleanup();
});

test('las colas sobreviven a un reinicio y se reanudan con su recuento de intentos', async () => {
  const r = rig([{ok: false, error: 'provider_unavailable'}]);
  r.channel.enqueue(alertOf('a:1'));
  await r.channel.process();
  assert.ok(r.store.save());
  // «Reinicio»: otro almacén y otro canal sobre el mismo directorio.
  const store2 = new StateStore({dir: r.t.dir, now: r.c.now, log: r.log});
  assert.equal(store2.load(), 'loaded');
  const calls = [];
  const channel2 = new Channel({name: 'webhook', store: store2, now: r.c.now, log: r.log,
    sender: {send: async (alert, attempt) => { calls.push([alert.event_id, attempt]); return {ok: true}; }}});
  assert.equal(channel2.status(NODE_A).pending, 1);
  assert.equal(channel2.status(NODE_A).last_delivery.error, 'provider_unavailable');
  assert.equal(await channel2.process(), 1);
  assert.deepEqual(calls, [['a:1', 2]]);
  r.t.cleanup();
});

test('el estado de un nodo no incluye datos de otro', async () => {
  const r = rig();
  r.channel.enqueue(alertOf('a:1', NODE_A));
  await r.channel.process();
  r.channel.enqueue(alertOf('b:1', NODE_B));
  assert.equal(r.channel.status(NODE_A).pending, 0);
  assert.equal(r.channel.status(NODE_A).last_delivery.event_id, 'a:1');
  assert.equal(r.channel.status(NODE_B).pending, 1);
  assert.equal(r.channel.status(NODE_B).last_delivery, null);
  r.t.cleanup();
});

test('un canal sin configurar lo indica sin afirmar entrega y no encola', async () => {
  const t = tempDir();
  const store = new StateStore({dir: t.dir});
  store.load();
  const channel = new Channel({name: 'push', store, sender: null});
  assert.equal(channel.enqueue(alertOf('a:1')), false);
  assert.equal(await channel.process(), 0);
  assert.deepEqual(channel.status(NODE_A), {configured: false, last_delivery: null, pending: 0, suppressed: 0});
  t.cleanup();
});

test('solo hay un intento en vuelo por canal', async () => {
  const t = tempDir();
  const store = new StateStore({dir: t.dir});
  store.load();
  let inFlight = 0, peak = 0;
  const channel = new Channel({name: 'webhook', store, sender: {send: async () => {
    peak = Math.max(peak, ++inFlight); await new Promise(r => setTimeout(r, 15)); inFlight--; return {ok: true}; }}});
  channel.enqueue(alertOf('a:1'));
  channel.enqueue(alertOf('a:2'));
  await Promise.all([channel.process(), channel.process(), channel.process()]);
  assert.equal(peak, 1);
  assert.equal(channel.queue.length, 0);
  t.cleanup();
});
