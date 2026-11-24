// Ensamblado del componente: MQTT (solo lectura de eventos) → validación →
// enrutador → canales con cola → webhook, más la API HTTPS de estado.
//
// Garantía de entrega: el manejador de mensajes es síncrono y persiste el
// estado (ID visto y cola) ANTES de volver. mqtt.js envía el PUBACK de QoS 1
// después de ese retorno y la sesión es persistente en el broker, de modo que un
// corte entre la recepción y el guardado provoca una retransmisión, no una
// pérdida; la deduplicación absorbe el duplicado.
import mqtt from 'mqtt';
import {Channel} from './channel.js';
import {createStatusApi} from './http-api.js';
import {eventTopics, ingest} from './ingest.js';
import {Router} from './router.js';
import {StateStore} from './state.js';
import {createWebhook} from './webhook.js';

const TICK_MS = 1000;

/** Registro en JSON por línea. Los campos son códigos e IDs de evento: nunca cargas, URL ni cabeceras. */
export function jsonLog(entry) {
  const line = JSON.stringify({ts: new Date().toISOString(), ...entry});
  (entry.level === 'error' || entry.level === 'warn' ? process.stderr : process.stdout).write(`${line}\n`);
}

export function createService(config, deps = {}) {
  const now = deps.now ?? Date.now;
  const log = deps.log ?? jsonLog;
  const store = new StateStore({dir: config.stateDir, now, log});
  const loaded = store.load();
  const sender = config.webhook ? (deps.webhook ?? createWebhook(config.webhook)) : null;
  const channels = {
    webhook: new Channel({name: 'webhook', store, sender, now, log}),
    // El envío push se añade en su propio cambio: hoy el canal existe, sin configurar.
    push: new Channel({name: 'push', store, sender: null, now, log}),
  };
  const router = new Router({store, channels, config: config.alerts, now, log});
  const allowed = new Set(config.nodes.map(node => node.deviceId));
  const counters = {events: 0, ignored: 0, invalid: 0, foreign: 0, errors: 0};
  const api = createStatusApi({config: config.statusApi, nodes: config.nodes, statusFor});
  let client = null, server = null, timer = null, delivering = false, mqttState = 'stopped';

  function statusFor(device) {
    return {webhook: channels.webhook.status(device), push: channels.push.status(device)};
  }
  function flush() { if (store.dirty) store.save(); }

  async function deliver() {
    if (delivering) return;
    delivering = true;
    try {
      for (const channel of Object.values(channels)) await channel.process();
      flush();
    } finally {
      delivering = false;
    }
  }

  // Síncrono a propósito: ver la garantía de entrega al principio del archivo.
  function onMessage(topic, payload) {
    try {
      const result = ingest(topic, payload, allowed);
      if (result.kind === 'event') {
        counters.events++;
        const out = router.handle(result.event);
        log({level: 'info', msg: 'event', device_id: result.event.device_id, event_id: result.event.message_id,
          severity: result.event.severity, outcome: out.outcome, channels: out.channels});
        flush();
        if (out.channels.length) queueMicrotask(() => { deliver().catch(() => counters.errors++); });
      } else {
        counters[result.kind]++;
        if (result.kind === 'invalid') log({level: 'warn', msg: 'event_invalid', code: result.code});
        else log({level: 'info', msg: 'message_skipped', kind: result.kind});
      }
    } catch {
      counters.errors++;
      log({level: 'error', msg: 'event_processing_failed'});
    }
  }

  return {
    store, channels, router, counters, statusFor,
    health: () => ({mqtt: mqttState, counters: {...counters}, router: {...router.counters},
      state: loaded, save_failures: store.saveFailures}),
    onMessage, deliver,

    async start() {
      server = api.build();
      await new Promise((resolve, reject) => {
        server.once('error', reject);
        server.listen(config.statusApi.port, config.statusApi.host, () => { server.off('error', reject); resolve(); });
      });
      const connect = deps.mqttConnect ?? mqtt.connect;
      mqttState = 'connecting';
      // Sesión persistente (clean:false) con QoS 1: el broker conserva los
      // eventos mientras el componente está caído.
      const mine = client = connect(config.mqtt.url, {clientId: config.mqtt.clientId, clean: false, protocolVersion: 4,
        reconnectPeriod: 5000, connectTimeout: 10000, keepalive: 30, resubscribe: true});
      mine.on('connect', () => {
        if (mqttState === 'stopped') return; // un 'connect' tardío tras stop()
        mqttState = 'connected';
        mine.subscribe(eventTopics(config.nodes.map(n => n.deviceId)), {qos: 1}, error => {
          if (error) log({level: 'error', msg: 'mqtt_subscribe_failed'});
        });
        log({level: 'info', msg: 'mqtt_connected'});
      });
      mine.on('close', () => { if (mqttState !== 'stopped') { mqttState = 'reconnecting'; } });
      // Los errores de mqtt.js pueden citar la URL: solo se registra que ocurrió.
      mine.on('error', () => log({level: 'warn', msg: 'mqtt_error'}));
      mine.on('message', (topic, payload) => { if (mqttState !== 'stopped') onMessage(topic, payload); });
      timer = setInterval(() => { deliver().catch(() => counters.errors++); }, TICK_MS);
      deliver().catch(() => counters.errors++); // reanuda lo que quedó pendiente en el estado
      return {port: server.address().port};
    },

    async stop() {
      mqttState = 'stopped';
      if (timer) clearInterval(timer);
      timer = null;
      if (client) await new Promise(resolve => client.end(false, {}, resolve));
      client = null;
      if (server) await new Promise(resolve => { server.close(() => resolve()); server.closeAllConnections?.(); });
      server = null;
      sender?.close?.();
      flush();
    },
  };
}
