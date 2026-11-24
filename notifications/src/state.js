// Estado persistente acotado: IDs ya vistos (deduplicación), cola de entrega
// por canal y el último resultado. Un único archivo JSON escrito de forma
// atómica (temporal + fsync + renombrado) en un directorio privado. No es una
// base de datos ni una plataforma histórica: los límites se aplican siempre.
import {closeSync, existsSync, fsyncSync, mkdirSync, openSync, readFileSync, renameSync, writeSync} from 'node:fs';
import {join} from 'node:path';

export const LIMITS = Object.freeze({
  seenIds: 512,                // IDs de evento recordados
  seenTtlMs: 24 * 3600 * 1000, // y durante cuánto tiempo
  outbox: 128,                 // alertas pendientes por canal
  outboxTtlMs: 24 * 3600 * 1000,
});
export const CHANNELS = Object.freeze(['webhook', 'push']);
const MAX_FILE_BYTES = 2 * 1024 * 1024;
const MAX_ID = 200; // identidad de evento (129) más la instalación en el canal push (#id de 64)
const COUNTER_MAX = Number.MAX_SAFE_INTEGER;

const ID = /^[A-Za-z0-9._:-]{1,64}$/;
const MAX_NODES = 16;
function emptyNode() { return {last_delivery: null, suppressed: 0}; }
function fresh() {
  return {
    version: 1,
    seen: {},
    outbox: Object.fromEntries(CHANNELS.map(c => [c, []])),
    // Resultado y contadores por canal y por nodo: un nodo nunca ve datos de otro.
    channels: Object.fromEntries(CHANNELS.map(c => [c, {}])),
  };
}

// Reconstruye el estado leído descartando todo lo que no cumpla la forma
// esperada: un archivo manipulado o dañado no puede introducir campos ni
// superar los límites.
function sanitize(raw, now) {
  const state = fresh();
  if (!raw || typeof raw !== 'object' || raw.version !== 1) return state;
  if (raw.seen && typeof raw.seen === 'object') {
    for (const [key, at] of Object.entries(raw.seen))
      if (key.length <= 140 && Number.isSafeInteger(at) && at > 0 && now - at < LIMITS.seenTtlMs) state.seen[key] = at;
  }
  for (const channel of CHANNELS) {
    const items = raw.outbox?.[channel];
    if (Array.isArray(items))
      for (const item of items.slice(-4 * LIMITS.outbox)) { // recorrido acotado; luego se conservan las más recientes
        const valid = item && typeof item === 'object' && typeof item.id === 'string' && item.id.length <= MAX_ID &&
          Number.isSafeInteger(item.created) && Number.isSafeInteger(item.attempts) && item.attempts >= 0 &&
          item.alert && typeof item.alert === 'object' && now - item.created < LIMITS.outboxTtlMs;
        if (valid) state.outbox[channel].push({id: item.id, created: item.created, attempts: item.attempts, alert: item.alert});
      }
    state.outbox[channel] = state.outbox[channel].slice(-LIMITS.outbox);
    const perNode = raw.channels?.[channel];
    if (perNode && typeof perNode === 'object')
      for (const [device, saved] of Object.entries(perNode).slice(0, MAX_NODES)) {
        if (!ID.test(device) || !saved || typeof saved !== 'object') continue;
        const node = state.channels[channel][device] = emptyNode();
        if (Number.isSafeInteger(saved.suppressed) && saved.suppressed >= 0) node.suppressed = saved.suppressed;
        const d = saved.last_delivery;
        if (d && typeof d === 'object' && typeof d.event_id === 'string' && d.event_id.length <= 64 &&
            typeof d.timestamp === 'string' && d.timestamp.length <= 32)
          node.last_delivery = {event_id: d.event_id, timestamp: d.timestamp,
            status: d.status === 'failed' ? 'failed' : 'provider_accepted', read_status: 'unknown',
            error: d.status === 'failed' && typeof d.error === 'string' ? d.error : null};
      }
  }
  return state;
}

/** Escritura atómica y síncrona: temporal 0600 + fsync + renombrado. Lanza si falla. */
export function atomicWriteSync(file, text) {
  const temp = `${file}.tmp`;
  const fd = openSync(temp, 'w', 0o600);
  try { writeSync(fd, text); fsyncSync(fd); } finally { closeSync(fd); }
  renameSync(temp, file);
}

export class StateStore {
  constructor({dir, now = Date.now, log = () => {}}) {
    this.dir = dir;
    this.file = join(dir, 'state.json');
    this.now = now;
    this.log = log;
    this.data = fresh();
    this.saveFailures = 0;
    this.dirty = false;
  }

  /** Carga el estado. Un archivo ilegible se aparta, no se borra ni se sobrescribe a ciegas. */
  load() {
    mkdirSync(this.dir, {recursive: true, mode: 0o700});
    if (!existsSync(this.file)) { this.data = fresh(); return 'empty'; }
    try {
      const text = readFileSync(this.file, 'utf8');
      if (text.length > MAX_FILE_BYTES) throw new Error('large');
      this.data = sanitize(JSON.parse(text), this.now());
      return 'loaded';
    } catch {
      const moved = `${this.file}.corrupt-${this.now()}`;
      try { renameSync(this.file, moved); } catch { /* se informa igualmente */ }
      this.log({level: 'warn', msg: 'state_unreadable', kept_as: 'state.json.corrupt-*'});
      this.data = fresh();
      return 'corrupt';
    }
  }

  /** Aplica los límites de IDs vistos antes de guardar. */
  prune() {
    const now = this.now();
    const entries = Object.entries(this.data.seen).filter(([, at]) => now - at < LIMITS.seenTtlMs);
    entries.sort((a, b) => a[1] - b[1]);
    this.data.seen = Object.fromEntries(entries.slice(-LIMITS.seenIds));
  }

  /** Escritura atómica y síncrona: la llamada devuelve cuando el estado ya está en disco. */
  save() {
    this.prune();
    const text = JSON.stringify(this.data);
    try {
      atomicWriteSync(this.file, text);
      this.saveFailures = 0;
      this.dirty = false;
      return true;
    } catch {
      // Sin disco no se pierde la alerta: sigue en memoria y se informa el fallo.
      this.saveFailures = Math.min(COUNTER_MAX, this.saveFailures + 1);
      this.log({level: 'error', msg: 'state_save_failed', consecutive: this.saveFailures});
      return false;
    }
  }

  hasSeen(key) {
    const at = this.data.seen[key];
    return at !== undefined && this.now() - at < LIMITS.seenTtlMs;
  }
  markSeen(key) { this.data.seen[key] = this.now(); this.dirty = true; }
  touch() { this.dirty = true; }
  /** Estado de un canal para un nodo concreto (se crea al usarlo). */
  channel(name, device) { return this.data.channels[name][device] ??= emptyNode(); }
  outbox(name) { return this.data.outbox[name]; }
}
