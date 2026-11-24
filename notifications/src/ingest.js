// Entrada de eventos: valida cada mensaje MQTT con el contrato compartido y lo
// reduce a un evento Cebo listo para el enrutador. Todo lo demás se descarta
// sin dejar rastro del contenido: solo se cuentan categorías.
import {ContractError, decodeAnyMessage, eventIdentity, mqttTopic} from '@sentinel/contracts';

const TOPIC = /^sentinel\/v([12])\/nodes\/([A-Za-z0-9._:-]{1,64})\/events$/;

/** Topics de eventos de cada nodo (v1 y v2), construidos con la función del contrato. */
export function eventTopics(deviceIds) {
  return deviceIds.flatMap(id => [mqttTopic(1, id, 'events'), mqttTopic(2, id, 'events')]);
}

/**
 * @param topic   topic MQTT recibido
 * @param payload Buffer del mensaje
 * @param allowed Set de device_id autorizados
 * @returns {kind, event?}
 *   kind: 'event' (Cebo v2 válido) | 'ignored' (válido pero no es un evento Cebo)
 *       | 'invalid' (no cumple el contrato) | 'foreign' (topic o nodo no autorizado)
 */
export function ingest(topic, payload, allowed) {
  const match = TOPIC.exec(topic);
  if (!match || !allowed.has(match[2])) return {kind: 'foreign'};
  const [, version, topicDevice] = match;
  let message;
  try {
    message = decodeAnyMessage(payload);
  } catch (error) {
    // ContractError lleva solo un código; cualquier otra excepción se trata igual.
    return {kind: 'invalid', code: error instanceof ContractError ? error.code : 'invalid_message'};
  }
  // El topic enruta pero no autentica: el nodo declarado en el mensaje debe
  // coincidir con el del topic y con la versión.
  if (message.device_id !== topicDevice || String(message.version) !== version) return {kind: 'invalid', code: 'topic_mismatch'};
  if (message.type !== 'event') return {kind: 'ignored'};
  // v1 no define eventos de interacción de Cebo: se valida y se descarta.
  if (message.version !== 2 || message.payload.name !== 'cebo.interaction' || message.payload.profile !== 'cebo')
    return {kind: 'ignored'};
  const p = message.payload;
  return {kind: 'event', event: {
    identity: eventIdentity(message),
    device_id: message.device_id,
    message_id: message.message_id,
    boot_id: message.boot_id,
    name: p.name,
    severity: p.severity,
    origin: p.origin,
    uptime_ms: message.uptime_ms,
    timestamp: message.timestamp,
    time_quality: message.time_quality,
    details: p.details,
  }};
}
