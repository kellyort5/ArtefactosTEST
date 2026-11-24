// Enrutador común: decide qué evento Cebo se convierte en alerta y a qué
// canales va. Solo hace tres cosas, en este orden:
//   1. deduplica por (device_id, message_id), de modo que una retransmisión MQTT
//      (QoS 1 puede duplicar) no multiplica alertas;
//   2. filtra por severidad y origen (simulado/real) según la configuración;
//   3. controla las repeticiones por canal y origen: dentro de la ventana se
//      suprime y se cuenta, salvo que la severidad suba.
import {SEVERITY_RANK} from './config.js';

const MAX_WINDOWS = 256; // orígenes recordados para el control de repeticiones

export class Router {
  constructor({store, channels, config, now = Date.now, log = () => {}}) {
    this.store = store;
    this.channels = channels;      // {webhook: Channel, push?: Channel}
    this.config = config;
    this.now = now;
    this.log = log;
    this.windows = new Map();      // "canal|nodo|origen|protocolo" → {at, rank}
    this.counters = {received: 0, duplicates: 0, below_threshold: 0, simulated: 0, routed: 0, suppressed: 0};
  }

  /**
   * @param event {identity, device_id, message_id, boot_id, severity, origin, name, uptime_ms,
   *               timestamp, time_quality, details}   evento Cebo ya validado por el contrato
   * @returns {outcome, channels}
   */
  handle(event) {
    this.counters.received++;
    if (this.store.hasSeen(event.identity)) {
      this.counters.duplicates++;
      return {outcome: 'duplicate', channels: []};
    }
    this.store.markSeen(event.identity);
    const rank = SEVERITY_RANK[event.severity];
    if (event.origin === 'simulated' && !this.config.includeSimulated) {
      this.counters.simulated++;
      return {outcome: 'simulated', channels: []};
    }
    if (rank < SEVERITY_RANK[this.config.minSeverity]) {
      this.counters.below_threshold++;
      return {outcome: 'below_threshold', channels: []};
    }
    const sent = [];
    const body = this.body(event);
    for (const [name, channel] of Object.entries(this.channels)) {
      if (!channel.configured) continue;
      if (this.repeated(name, event, rank)) {
        channel.suppress(event.device_id);
        this.counters.suppressed++;
        continue;
      }
      if (channel.enqueue({event_key: event.identity, body})) sent.push(name);
    }
    if (sent.length) this.counters.routed++;
    return {outcome: sent.length ? 'routed' : 'suppressed', channels: sent};
  }

  // Cuerpo de la alerta. Solo contiene lo que el contrato ya validó del evento
  // (metadatos permitidos: nunca credenciales, cuerpos ni líneas del cliente).
  body(event) {
    return {
      schema: 'sentinel.alert.v1',
      device_id: event.device_id,
      event_id: event.message_id,
      boot_id: event.boot_id,
      name: event.name,
      severity: event.severity,
      origin: event.origin,
      uptime_ms: event.uptime_ms,
      timestamp: event.timestamp,
      time_quality: event.time_quality,
      details: event.details,
    };
  }

  repeated(channel, event, rank) {
    const window = this.config.repeatWindowMs;
    if (!window) return false;
    const key = `${channel}|${event.device_id}|${event.details.source_ipv4}|${event.details.protocol}`;
    const at = this.now();
    const last = this.windows.get(key);
    const inWindow = last !== undefined && at - last.at < window;
    this.windows.delete(key);
    // Dentro de la ventana solo pasa una escalada de severidad, que es
    // información nueva; la ventana se cuenta desde la última alerta enviada.
    if (inWindow && rank <= last.rank) {
      this.windows.set(key, last);
      return true;
    }
    this.windows.set(key, {at, rank});
    if (this.windows.size > MAX_WINDOWS) this.windows.delete(this.windows.keys().next().value);
    return false;
  }
}
