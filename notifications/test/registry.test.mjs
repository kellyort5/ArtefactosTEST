import assert from 'node:assert/strict';
import {readFileSync, readdirSync, statSync, writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {test} from 'node:test';
import {MAX_REGISTRATIONS, REGISTRATION_TTL_MS, RegistrationStore} from '../src/registry.js';
import {NODE_A, NODE_B, TOKEN_ANDROID_1, TOKEN_ANDROID_2, clock, recordingLog, tempDir} from './helpers.mjs';

function rig() {
  const t = tempDir();
  const c = clock();
  const log = recordingLog();
  const registry = new RegistrationStore({dir: t.dir, now: c.now, log});
  registry.load();
  return {t, c, log, registry};
}

test('un alta, una renovación con otro token y una baja respetan el par nodo-instalación', () => {
  const r = rig();
  assert.equal(r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1), 'registered');
  assert.equal(r.registry.get(NODE_A, 'inst-1').token, TOKEN_ANDROID_1);
  // Renovar con otro token sustituye al anterior: no quedan dos activos.
  assert.equal(r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_2), 'registered');
  assert.deepEqual(r.registry.forDevice(NODE_A).map(x => x.token), [TOKEN_ANDROID_2]);
  assert.equal(r.registry.size, 1);
  // La misma instalación en otro nodo es otro registro.
  r.registry.register(NODE_B, 'inst-1', TOKEN_ANDROID_1);
  assert.equal(r.registry.size, 2);
  assert.equal(r.registry.forDevice(NODE_B)[0].token, TOKEN_ANDROID_1);
  // La baja solo afecta al nodo indicado y es idempotente.
  assert.equal(r.registry.remove(NODE_A, 'inst-1'), true);
  assert.equal(r.registry.remove(NODE_A, 'inst-1'), false);
  assert.equal(r.registry.get(NODE_B, 'inst-1').token, TOKEN_ANDROID_1);
  r.t.cleanup();
});

test('la capacidad es de 32 registros; renovar un par existente no consume una plaza', () => {
  const r = rig();
  for (let i = 0; i < MAX_REGISTRATIONS; ++i)
    assert.equal(r.registry.register(NODE_A, `inst-${i}`, `FICTIONAL-TOKEN-${i}`), 'registered');
  assert.equal(r.registry.register(NODE_A, 'inst-extra', 'FICTIONAL-TOKEN-X'), 'full');
  assert.equal(r.registry.register(NODE_B, 'inst-0', 'FICTIONAL-TOKEN-Y'), 'full'); // la capacidad es del componente
  assert.equal(r.registry.size, MAX_REGISTRATIONS);
  assert.equal(r.registry.register(NODE_A, 'inst-5', 'FICTIONAL-TOKEN-RENEWED'), 'registered');
  assert.equal(r.registry.get(NODE_A, 'inst-5').token, 'FICTIONAL-TOKEN-RENEWED');
  // Una baja libera la plaza.
  r.registry.remove(NODE_A, 'inst-1');
  assert.equal(r.registry.register(NODE_A, 'inst-extra', 'FICTIONAL-TOKEN-X'), 'registered');
  r.t.cleanup();
});

test('cada alta o renovación vale 30 días; después se retira', () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.c.advance(REGISTRATION_TTL_MS - 1);
  assert.ok(r.registry.get(NODE_A, 'inst-1'));
  // Renovar reinicia el plazo.
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.c.advance(REGISTRATION_TTL_MS - 1);
  assert.ok(r.registry.get(NODE_A, 'inst-1'));
  r.c.advance(1);
  assert.equal(r.registry.get(NODE_A, 'inst-1'), null);
  assert.equal(r.registry.forDevice(NODE_A).length, 0);
  // Lo vencido libera su plaza y se retira con purge().
  for (let i = 0; i < MAX_REGISTRATIONS; ++i) r.registry.register(NODE_A, `inst-${i}`, `FICTIONAL-TOKEN-${i}`);
  r.c.advance(REGISTRATION_TTL_MS);
  assert.equal(r.registry.purge(), MAX_REGISTRATIONS);
  assert.equal(r.registry.size, 0);
  r.t.cleanup();
});

test('se guarda de forma atómica, con permisos privados, y se recupera tras reiniciar', () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.registry.register(NODE_B, 'inst-2', TOKEN_ANDROID_2);
  assert.equal(r.registry.save(), true);
  assert.deepEqual(readdirSync(r.t.dir), ['registrations.json']);
  if (process.platform !== 'win32') assert.equal(statSync(join(r.t.dir, 'registrations.json')).mode & 0o077, 0, 'solo el propietario');
  const again = new RegistrationStore({dir: r.t.dir, now: r.c.now, log: r.log});
  assert.equal(again.load(), 'loaded');
  assert.equal(again.get(NODE_A, 'inst-1').token, TOKEN_ANDROID_1);
  assert.equal(again.get(NODE_B, 'inst-2').token, TOKEN_ANDROID_2);
  // Lo vencido no se restaura.
  r.c.advance(REGISTRATION_TTL_MS);
  const late = new RegistrationStore({dir: r.t.dir, now: r.c.now, log: r.log});
  late.load();
  assert.equal(late.size, 0);
  r.t.cleanup();
});

test('un archivo dañado se aparta sin borrarlo; uno manipulado no introduce datos inválidos', () => {
  const r = rig();
  const file = join(r.t.dir, 'registrations.json');
  writeFileSync(file, '{"version":1,"registrations":[');
  assert.equal(r.registry.load(), 'corrupt');
  assert.ok(readdirSync(r.t.dir).some(f => f.startsWith('registrations.json.corrupt-')));
  assert.ok(r.log.entries.some(e => e.msg === 'registrations_unreadable'));

  const now = r.c.now();
  const many = Array.from({length: 100}, (_, i) => ({device_id: NODE_A, installation_id: `inst-${i}`, token: `FICTIONAL-T-${i}`, expires: now + 1000}));
  writeFileSync(file, JSON.stringify({version: 1, registrations: [
    ...many,
    {device_id: 'bad id', installation_id: 'x', token: 'FICTIONAL-T', expires: now + 1000},
    {device_id: NODE_A, installation_id: 'x', token: 'con espacios', expires: now + 1000},
    {device_id: NODE_A, installation_id: 'y', token: 'a'.repeat(2049), expires: now + 1000},
    {device_id: NODE_A, installation_id: 'z', token: 'FICTIONAL-T', expires: now - 1},
    {device_id: NODE_A, installation_id: 'w', token: 'FICTIONAL-T', expires: 'mañana'},
  ], extra: 'campo'}));
  const t2 = new RegistrationStore({dir: r.t.dir, now: r.c.now, log: r.log});
  assert.equal(t2.load(), 'loaded');
  assert.equal(t2.size, MAX_REGISTRATIONS); // el tope se aplica también al leer
  assert.equal(t2.get(NODE_A, 'z'), null);
  r.t.cleanup();
});

test('los registros nunca escriben tokens en el log', () => {
  const r = rig();
  r.registry.register(NODE_A, 'inst-1', TOKEN_ANDROID_1);
  r.registry.save();
  r.registry.file = join(r.t.dir, 'no-existe', 'registrations.json');
  assert.equal(r.registry.save(), false);
  writeFileSync(join(r.t.dir, 'registrations.json'), 'no json');
  const again = new RegistrationStore({dir: r.t.dir, now: r.c.now, log: r.log});
  again.load();
  assert.ok(r.log.entries.length >= 1);
  assert.ok(!JSON.stringify(r.log.entries).includes(TOKEN_ANDROID_1));
  assert.ok(!JSON.stringify(r.log.entries).includes('FICTIONAL'));
  r.t.cleanup();
});

test('lo vencido no ocupa plazas al restaurar: se conservan las vigentes', () => {
  const r = rig();
  const now = r.c.now();
  const expired = Array.from({length: 20}, (_, i) => ({device_id: NODE_A, installation_id: `old-${i}`, token: `FICTIONAL-OLD-${i}`, expires: now - 1}));
  const live = Array.from({length: 20}, (_, i) => ({device_id: NODE_A, installation_id: `live-${i}`, token: `FICTIONAL-LIVE-${i}`, expires: now + 1000}));
  writeFileSync(join(r.t.dir, 'registrations.json'), JSON.stringify({version: 1, registrations: [...expired, ...live]}));
  const again = new RegistrationStore({dir: r.t.dir, now: r.c.now, log: r.log});
  assert.equal(again.load(), 'loaded');
  assert.equal(again.size, 20);
  assert.ok(again.get(NODE_A, 'live-19') && !again.get(NODE_A, 'old-0'));
  r.t.cleanup();
});
