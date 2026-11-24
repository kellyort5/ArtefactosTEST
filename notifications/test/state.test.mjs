import assert from 'node:assert/strict';
import {existsSync, readFileSync, readdirSync, writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {test} from 'node:test';
import {LIMITS, StateStore} from '../src/state.js';
import {NODE_A, NODE_B, clock, recordingLog, tempDir} from './helpers.mjs';

const open = (dir, c = clock(), log = recordingLog()) => {
  const store = new StateStore({dir, now: c.now, log});
  return {store, c, log, status: store.load()};
};

test('el estado se guarda de forma atómica y se recupera tras reiniciar', () => {
  const t = tempDir();
  try {
    const first = open(t.dir);
    assert.equal(first.status, 'empty');
    first.store.markSeen('node-example/boot-1:1');
    first.store.outbox('webhook').push({id: 'node-example/boot-1:1', created: first.c.now(), attempts: 2,
      alert: {device_id: NODE_A, event_id: 'boot-1:1'}});
    first.store.channel('webhook', NODE_A).suppressed = 3;
    first.store.channel('webhook', NODE_A).last_delivery = {event_id: 'boot-1:1', timestamp: '2026-11-24T12:00:00.000Z',
      status: 'failed', read_status: 'unknown', error: 'provider_unavailable'};
    assert.ok(first.store.save());
    // No queda el temporal ni se escribe fuera del directorio privado.
    assert.deepEqual(readdirSync(t.dir), ['state.json']);

    const second = open(t.dir, first.c);
    assert.equal(second.status, 'loaded');
    assert.ok(second.store.hasSeen('node-example/boot-1:1'));
    assert.equal(second.store.outbox('webhook')[0].attempts, 2);
    assert.equal(second.store.channel('webhook', NODE_A).suppressed, 3);
    assert.equal(second.store.channel('webhook', NODE_A).last_delivery.error, 'provider_unavailable');
    // Otro nodo no hereda el resultado del primero.
    assert.equal(second.store.channel('webhook', NODE_B).last_delivery, null);
  } finally { t.cleanup(); }
});

test('los IDs vistos están acotados por cantidad y por tiempo', () => {
  const t = tempDir();
  try {
    const {store, c} = open(t.dir);
    for (let i = 0; i < LIMITS.seenIds + 40; ++i) { store.markSeen(`n/${i}`); c.advance(1); }
    store.save();
    assert.equal(Object.keys(store.data.seen).length, LIMITS.seenIds);
    assert.ok(!store.hasSeen('n/0') && store.hasSeen(`n/${LIMITS.seenIds + 39}`)); // se conservan los más recientes
    c.advance(LIMITS.seenTtlMs);
    assert.ok(!store.hasSeen(`n/${LIMITS.seenIds + 39}`)); // caducan a las 24 h
    store.save();
    assert.equal(Object.keys(store.data.seen).length, 0);
  } finally { t.cleanup(); }
});

test('un archivo dañado se aparta sin borrarlo y se arranca limpio', () => {
  const t = tempDir();
  try {
    writeFileSync(join(t.dir, 'state.json'), '{"version":1,"seen":');
    const {store, status, log} = open(t.dir);
    assert.equal(status, 'corrupt');
    assert.deepEqual(Object.keys(store.data.seen), []);
    const files = readdirSync(t.dir);
    assert.ok(files.some(f => f.startsWith('state.json.corrupt-')), 'se conserva el original apartado');
    assert.ok(!existsSync(join(t.dir, 'state.json')));
    assert.ok(log.entries.some(e => e.msg === 'state_unreadable'));
  } finally { t.cleanup(); }
});

test('un archivo manipulado no puede introducir campos ni superar los límites', () => {
  const t = tempDir();
  try {
    const c = clock();
    const items = Array.from({length: LIMITS.outbox + 50}, (_, i) => ({id: `n/${i}`, created: c.now(), attempts: 0,
      alert: {device_id: NODE_A, event_id: `${i}`}}));
    items.push({id: 'x', created: c.now() - LIMITS.outboxTtlMs - 1, attempts: 0, alert: {}}); // vencida
    items.push({id: 'y', created: 'ayer', attempts: 0, alert: {}});                          // forma inválida
    writeFileSync(join(t.dir, 'state.json'), JSON.stringify({
      version: 1, seen: {ok: c.now(), old: 1, bad: 'x', ['k'.repeat(200)]: c.now()},
      outbox: {webhook: items, evil: [{id: 'z'}]},
      channels: {webhook: {[NODE_A]: {suppressed: -5, last_delivery: {event_id: 'e', timestamp: 't', status: 'read', error: 'x', extra: 1}},
        'bad id': {suppressed: 1}}, evil: {}},
      token: 'FICTIONAL-SECRET', extra: true,
    }));
    const {store} = open(t.dir, c);
    assert.deepEqual(Object.keys(store.data.seen), ['ok']);
    assert.equal(store.outbox('webhook').length, LIMITS.outbox);
    assert.ok(store.outbox('webhook').every(i => i.alert.device_id === NODE_A));
    assert.equal(store.data.outbox.evil, undefined);
    const node = store.channel('webhook', NODE_A);
    assert.equal(node.suppressed, 0);
    // Un estado desconocido nunca se interpreta como lectura: read_status es siempre unknown.
    assert.deepEqual(node.last_delivery, {event_id: 'e', timestamp: 't', status: 'provider_accepted', read_status: 'unknown', error: null});
    assert.equal(store.data.channels.webhook['bad id'], undefined);
    store.save();
    assert.ok(!readFileSync(join(t.dir, 'state.json'), 'utf8').includes('FICTIONAL-SECRET'));
  } finally { t.cleanup(); }
});

test('si no se puede escribir, se informa y la alerta sigue en memoria', () => {
  const t = tempDir();
  try {
    const {store, log} = open(t.dir);
    store.markSeen('a/1');
    store.file = join(t.dir, 'no-existe', 'state.json'); // directorio inexistente: la escritura falla
    assert.equal(store.save(), false);
    assert.equal(store.saveFailures, 1);
    assert.ok(store.dirty && store.hasSeen('a/1'));
    assert.ok(log.entries.some(e => e.msg === 'state_save_failed'));
  } finally { t.cleanup(); }
});
