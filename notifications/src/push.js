// Canal push: reparte cada alerta entre las instalaciones registradas del nodo.
// Cada (alerta, instalación) es un elemento propio de la cola: así un token que
// falla no repite el envío a los demás ni bloquea al resto. La cola solo guarda
// identificadores; el token se lee del registro en el momento de enviar y no se
// copia a la cola ni al estado.
import {makePushNotification} from '@sentinel/contracts';
import {Channel} from './channel.js';

export class PushChannel extends Channel {
  constructor({registry, ...options}) {
    super({name: 'push', ...options});
    this.registry = registry;
  }

  /** Un elemento por instalación del nodo, con lo mínimo para componer el push. */
  enqueue({event_key, body}) {
    if (!this.configured) return false;
    let queued = false;
    for (const installation of this.registry.forDevice(body.device_id)) {
      const accepted = super.enqueue({
        event_key: `${event_key}#${installation.installation_id}`,
        body: {device_id: body.device_id, event_id: body.event_id, severity: body.severity,
          installation_id: installation.installation_id},
      });
      queued = queued || accepted;
    }
    return queued;
  }
}

/**
 * Envío de un elemento de la cola. Devuelve el resultado del contrato; los
 * fallos definitivos llevan `drop` y, si el token es inválido, se borra la
 * instalación.
 */
export function createPushSender({registry, provider, log = () => {}}) {
  return {
    async send(item) {
      const installation = registry.get(item.device_id, item.installation_id);
      // La instalación se dio de baja o venció mientras esperaba: nada que enviar.
      if (!installation) return {ok: false, error: 'invalid_registration', drop: true};
      // Identidad y severidad originales del evento: el contrato no genera ningún ID nuevo.
      const push = makePushNotification({device_id: item.device_id, message_id: item.event_id,
        payload: {name: 'cebo.interaction', profile: 'cebo', severity: item.severity}}, item.device_id);
      const result = await provider.send({token: installation.token, push});
      if (result.remove) {
        registry.remove(item.device_id, item.installation_id);
        registry.save();
        log({level: 'warn', msg: 'registration_removed', device_id: item.device_id,
          installation_id: item.installation_id, reason: result.error});
      }
      return result;
    },
  };
}
