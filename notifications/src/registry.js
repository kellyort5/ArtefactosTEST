// Registro de instalaciones push. Guarda, por nodo e instalación, el token del
// proveedor: es un secreto, así que vive solo en un archivo privado (0600) dentro
// del directorio de estado y nunca sale en respuestas, estado, eventos ni
// registros. Capacidad y retención las fija el contrato de notificaciones:
// 32 registros activos y 30 días desde cada alta o renovación.
import {existsSync, mkdirSync, readFileSync, renameSync} from 'node:fs';
import {join} from 'node:path';
import {atomicWriteSync} from './state.js';

export const MAX_REGISTRATIONS = 32;
export const REGISTRATION_TTL_MS = 30 * 24 * 3600 * 1000;
const MAX_FILE_BYTES = 512 * 1024;
const ID = /^[A-Za-z0-9._:-]{1,64}$/;
const TOKEN = /^[^\s\u0000-\u001f\u007f]+$/u; // opaco: sin controles ni espacios
const utf8 = new TextEncoder();

export class RegistrationStore {
  constructor({dir, now = Date.now, log = () => {}}) {
    this.file = join(dir, 'registrations.json');
    this.dir = dir;
    this.now = now;
    this.log = log;
    this.entries = new Map(); // "nodo|instalación" → {device_id, installation_id, token, expires}
  }

  static key(device, installation) { return `${device}|${installation}`; }

  /** Lee el archivo; lo inválido se descarta y un archivo ilegible se aparta sin borrarlo. */
  load() {
    mkdirSync(this.dir, {recursive: true, mode: 0o700});
    this.entries.clear();
    if (!existsSync(this.file)) return 'empty';
    try {
      const text = readFileSync(this.file, 'utf8');
      if (text.length > MAX_FILE_BYTES) throw new Error('large');
      const raw = JSON.parse(text);
      if (!raw || raw.version !== 1 || !Array.isArray(raw.registrations)) throw new Error('shape');
      const now = this.now();
      for (const r of raw.registrations.slice(0, 4 * MAX_REGISTRATIONS)) {
        if (!r || typeof r !== 'object' || !ID.test(r.device_id) || !ID.test(r.installation_id) ||
            typeof r.token !== 'string' || !TOKEN.test(r.token) || utf8.encode(r.token).length > 2048 ||
            !Number.isSafeInteger(r.expires) || r.expires <= now) continue;
        this.entries.set(RegistrationStore.key(r.device_id, r.installation_id),
          {device_id: r.device_id, installation_id: r.installation_id, token: r.token, expires: r.expires});
        if (this.entries.size >= MAX_REGISTRATIONS) break;
      }
      return 'loaded';
    } catch {
      try { renameSync(this.file, `${this.file}.corrupt-${this.now()}`); } catch { /* se informa igualmente */ }
      this.log({level: 'warn', msg: 'registrations_unreadable'});
      this.entries.clear();
      return 'corrupt';
    }
  }

  /** Elimina lo vencido. Devuelve cuántos registros se retiraron. */
  purge() {
    const now = this.now();
    let removed = 0;
    for (const [key, r] of this.entries)
      if (r.expires <= now) { this.entries.delete(key); removed++; }
    return removed;
  }

  get size() { this.purge(); return this.entries.size; }

  /**
   * Alta o renovación del par (nodo, instalación). Sustituye el token anterior;
   * una renovación no consume una plaza nueva. 'full' si es un par nuevo sin capacidad.
   */
  register(device, installation, token) {
    this.purge();
    const key = RegistrationStore.key(device, installation);
    if (!this.entries.has(key) && this.entries.size >= MAX_REGISTRATIONS) return 'full';
    this.entries.delete(key); // el orden de inserción refleja la última renovación
    this.entries.set(key, {device_id: device, installation_id: installation, token, expires: this.now() + REGISTRATION_TTL_MS});
    return 'registered';
  }

  /** Baja idempotente dentro del nodo indicado. */
  remove(device, installation) { return this.entries.delete(RegistrationStore.key(device, installation)); }

  /** Instalaciones activas de un nodo (con su token, solo para el proveedor). */
  forDevice(device) {
    this.purge();
    return [...this.entries.values()].filter(r => r.device_id === device);
  }
  get(device, installation) {
    this.purge();
    return this.entries.get(RegistrationStore.key(device, installation)) ?? null;
  }

  /** Escritura atómica y síncrona con permisos restrictivos. Falso si no se pudo. */
  save() {
    try {
      atomicWriteSync(this.file, JSON.stringify({version: 1, registrations: [...this.entries.values()]}));
      return true;
    } catch {
      this.log({level: 'error', msg: 'registrations_save_failed'});
      return false;
    }
  }
}
