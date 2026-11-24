// Canal de entrega con cola persistente y reintentos acotados. Cada canal
// (webhook hoy; push en su propio cambio) tiene su cola y su recuperación: el
// fallo de uno no afecta a los demás.
//
//  - Cola de hasta 128 alertas con vencimiento de 24 h; si se llena se descarta
//    la más antigua y se registra como `expired`.
//  - Reintentos con espera de 1 s que se duplica hasta 30 s; es la espera del
//    canal entero (no de cada alerta), de modo que un destino caído no se
//    martillea. Un éxito la reinicia.
//  - Un único intento en vuelo por canal y orden de llegada.
//  - Un envío puede devolver `drop: true` para un fallo definitivo: la alerta se retira sin reintentos.
import {LIMITS} from './state.js';

export const BACKOFF_MIN_MS = 1000;
export const BACKOFF_MAX_MS = 30000;
const BATCH = 16; // alertas por pasada, para no acaparar el bucle de eventos

export const backoff = failures => Math.min(BACKOFF_MAX_MS, BACKOFF_MIN_MS * 2 ** Math.max(0, failures - 1));

export class Channel {
  /**
   * @param name        'webhook' | 'push'
   * @param store       StateStore (cola y último resultado)
   * @param sender      null (sin configurar) o {send(alert, attempt) → Promise<{ok}|{ok:false,error,retryAfterMs?}>}
   */
  constructor({name, store, sender, now = Date.now, log = () => {}}) {
    this.name = name;
    this.store = store;
    this.sender = sender;
    this.now = now;
    this.log = log;
    this.failures = 0;     // fallos consecutivos del canal
    this.nextAt = 0;       // no intentar antes de este instante
    this.busy = false;
    this.stats = {accepted: 0, failed: 0, dropped: 0, expired: 0};
  }

  get configured() { return Boolean(this.sender); }
  get queue() { return this.store.outbox(this.name); }
  /** Estado de este canal para un nodo; las colas son compartidas pero lo que se informa es solo de ese nodo. */
  status(device) {
    const s = this.store.channel(this.name, device);
    return {configured: this.configured, last_delivery: s.last_delivery ? {...s.last_delivery} : null,
      pending: this.configured ? this.queue.filter(item => item.alert.device_id === device).length : 0,
      suppressed: s.suppressed};
  }

  suppress(device) {
    const s = this.store.channel(this.name, device);
    s.suppressed = Math.min(Number.MAX_SAFE_INTEGER, s.suppressed + 1);
    this.store.touch();
  }

  record(alert, status, error) {
    this.store.channel(this.name, alert.device_id).last_delivery = {event_id: alert.event_id,
      timestamp: new Date(this.now()).toISOString(), status, read_status: 'unknown',
      error: status === 'failed' ? error : null};
    this.store.touch();
  }

  /** Encola una alerta. Falso si el canal no está configurado. */
  enqueue(alert) {
    if (!this.configured) return false;
    const queue = this.queue;
    if (queue.some(item => item.id === alert.event_key)) return true; // ya pendiente
    while (queue.length >= LIMITS.outbox) {
      const dropped = queue.shift();
      this.stats.dropped++;
      this.record(dropped.alert, 'failed', 'expired');
      this.log({level: 'warn', msg: 'outbox_full_dropped', channel: this.name, event_id: dropped.alert.event_id});
    }
    queue.push({id: alert.event_key, created: this.now(), attempts: 0, alert: alert.body});
    this.store.touch();
    return true;
  }

  /** Retira de la cola lo que superó el vencimiento. */
  expire(at = this.now()) {
    const queue = this.queue;
    for (let i = 0; i < queue.length;) {
      if (at - queue[i].created >= LIMITS.outboxTtlMs) {
        const [gone] = queue.splice(i, 1);
        this.stats.expired++;
        this.record(gone.alert, 'failed', 'expired');
        this.log({level: 'warn', msg: 'alert_expired', channel: this.name, event_id: gone.alert.event_id});
      } else i++;
    }
  }

  /** Intenta entregar lo pendiente. Devuelve el número de entregas aceptadas. */
  async process(at = this.now()) {
    if (!this.configured || this.busy) return 0;
    this.busy = true;
    let delivered = 0;
    try {
      this.expire(at);
      for (let n = 0; n < BATCH && this.queue.length && at >= this.nextAt; ++n) {
        const item = this.queue[0];
        item.attempts++;
        let result;
        try { result = await this.sender.send(item.alert, item.attempts); }
        catch { result = {ok: false, error: 'internal_error'}; }
        if (result.ok) {
          this.queue.shift();
          this.store.touch();
          this.failures = 0;
          this.nextAt = 0;
          this.stats.accepted++;
          delivered++;
          this.record(item.alert, 'provider_accepted');
        } else if (result.drop) {
          // Fallo definitivo (p. ej. la instalación ya no existe): no se reintenta ni frena el canal.
          this.queue.shift();
          this.store.touch();
          this.stats.dropped++;
          this.record(item.alert, 'failed', result.error);
          this.log({level: 'warn', msg: 'delivery_dropped', channel: this.name, event_id: item.alert.event_id, error: result.error});
        } else {
          this.failures++;
          this.stats.failed++;
          const wait = Math.max(backoff(this.failures),
            Math.min(BACKOFF_MAX_MS, Number.isFinite(result.retryAfterMs) ? result.retryAfterMs : 0));
          this.nextAt = at + wait;
          this.record(item.alert, 'failed', result.error);
          this.log({level: 'warn', msg: 'delivery_failed', channel: this.name, event_id: item.alert.event_id,
            error: result.error, attempt: item.attempts, retry_in_ms: wait});
          break;
        }
        at = this.now();
      }
    } finally {
      this.busy = false;
    }
    return delivered;
  }
}
