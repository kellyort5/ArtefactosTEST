# Notificaciones: componente de alertas y webhook

Responsable: Persona 3. Cambio: [sentinel-alertas-webhook](../../openspec/changes/sentinel-alertas-webhook/proposal.md) ([ficha](../encargos/sentinel-alertas-webhook.md), [diseño](../../openspec/changes/sentinel-alertas-webhook/design.md), [spec](../../openspec/changes/sentinel-alertas-webhook/specs/notification-dispatch/spec.md), [tareas](../../openspec/changes/sentinel-alertas-webhook/tasks.md)). Rama: `feat/sentinel-alertas-webhook`, sobre `develop`. El código, el paquete, el lockfile y el Compose están en [`notifications/`](../../notifications/README.md), cuyo README es la referencia de configuración y arranque.

MQTT y una app cerrada no envían por sí solos las alertas externas obligatorias de R-10. Este componente cierra el primer canal: toma los eventos de interacción de Cebo que el nodo ya publica por MQTT y los entrega a un **webhook HTTPS** propio, y expone su estado a la app por una API HTTPS autenticada. El **envío push** es el siguiente cambio ([push servidor](../encargos/sentinel-alertas-push-servidor.md)) y se apoyará en este componente: hoy su canal existe en el estado y se informa como no configurado.

## Flujo

1. **Entrada.** Cliente MQTT de solo lectura, TCP anónimo, con sesión persistente y QoS 1, suscrito únicamente a `sentinel/v1|v2/nodes/<id>/events` de los nodos configurados. Cada mensaje se valida con el decodificador compartido (`decodeAnyMessage` de `@sentinel/contracts`) y su nodo y versión deben coincidir con los del topic. Un evento v2 `cebo.interaction` pasa al enrutador; un evento válido de otro tipo, o de v1, que no define interacciones Cebo, se ignora; lo demás se descarta contando categorías, nunca contenido.
2. **Enrutado.** Deduplica por `(device_id, message_id)` con `eventIdentity` del contrato, aplica la severidad mínima (`warning` por defecto) y el filtro de origen simulado, y controla las repeticiones por canal, nodo, IP de origen y protocolo en una ventana de 30 s. Una escalada de severidad no se suprime; lo suprimido se cuenta.
3. **Cola y reintentos.** Cada canal tiene una cola persistente de 128 alertas con vencimiento de 24 h y reintentos de 1 s que se duplican hasta 30 s. Un destino caído no se martillea: la espera es del canal entero.
4. **Webhook.** `POST` JSON a un destino HTTPS fijo, con verificación completa del certificado, sin redirecciones y sin conservar la respuesta. 2xx significa «aceptado por el destino», no «leído».
5. **Estado.** `GET /v1/status` por HTTPS con `Authorization: Bearer` de cada nodo, CORS explícito y respuesta validada con el contrato antes de enviarse.

## Decisiones de implementación (propuestas del diseño, a revisar)

El diseño marca estas elecciones como propuestas de implementación; se aplican tal cual salvo lo indicado. Conviene revisarlas con el equipo antes de integrarlas.

| Decisión | Motivo |
| --- | --- |
| Node 24 con JavaScript y módulos ES, sin framework ni compilación | Mismo runtime del repositorio; un servicio de unos cientos de líneas no justifica otra cadena de construcción |
| Paquete y lockfile propios, fuera de los workspaces; consume contratos por `file:../contracts` | No compite con P4 por el lockfile raíz y no copia esquemas ni tipos. Requiere `npm ci` en la raíz y compilar contratos antes |
| Dependencias: `mqtt` 5.16.1 (cliente); solo para desarrollo, `aedes` 1.2.0 (broker en proceso para las pruebas) y `typescript` (compilar contratos) | Cliente MQTT estándar; las pruebas usan un broker real en proceso en lugar de simular el transporte |
| MQTT 3.1.1 con sesión persistente y estado guardado **antes** del PUBACK | Entrega al menos una vez ante cortes del componente; la deduplicación absorbe los duplicados |
| Configuración en un archivo JSON privado | Un único punto de secretos fuera de Git, con validación estricta y mensajes que nunca citan valores |
| Bearer por nodo guardado solo como hash SHA-256, comparado en tiempo constante | La API no necesita conocer la credencial, y el bearer determina el nodo, no el cuerpo ni la consulta |
| Estado, contadores y último resultado **por nodo** | Con un único webhook y varios nodos, un nodo nunca ve eventos ni contadores de otro |
| Un único destino de webhook | «Lista fija de destino» del diseño; ampliarla es una extensión, no una necesidad del MVP |
| API de estado solo `GET /v1/status`; sin registro push | Registro, baja y TTL de instalaciones pertenecen al cambio del envío push |

## Límites y retención

| Concepto | Valor | Efecto al agotarse |
| --- | --- | --- |
| IDs de evento recordados | 512 durante 24 h | Se olvidan los más antiguos |
| Cola por canal | 128 alertas | Se descarta la más antigua y se informa `expired` |
| Vencimiento de una alerta | 24 h | Se retira y se informa `expired` |
| Reintentos | 1 s, 2 s, 4 s… hasta 30 s por canal | Un éxito reinicia la espera |
| Control de repeticiones | 30 s por canal, nodo, IP y protocolo | Se suprime y se cuenta en `suppressed` |
| Respuesta del webhook | descartada (máximo 64 KiB leídos) | — |
| Estado en disco | un JSON atómico de menos de 2 MiB | Un archivo ilegible se aparta, no se borra |

Códigos de fallo del estado: `provider_unavailable` (red, TLS, tiempo agotado, 5xx), `provider_rejected` (3xx sin seguir y 4xx), `rate_limited` (429, con `Retry-After` hasta 30 s), `expired` (vencida o descartada por cola llena) e `internal_error`. Un fallo se sigue reintentando hasta que la alerta vence.

## Privacidad y seguridad

- **Datos enviados.** El webhook recibe el `message_id` original del evento (`event_id`), severidad, origen, tiempos y los `details` del evento tal como el contrato v2 los permite: IP y puertos de origen, tipo, ruta canónica de una lista cerrada, cantidades y códigos de evidencia. El firmware no conserva credenciales, cuerpos ni líneas del cliente, de modo que no hay nada de eso que enviar. La IP de origen sí sale hacia el destino configurado.
- **Secretos.** La URL, la cabecera de autenticación y los hashes de los bearer solo están en el archivo de configuración privado o en `NOTIFY_WEBHOOK_AUTH_VALUE`. No aparecen en Git, en registros, en el estado ni en la API; los registros son JSON por línea con códigos e IDs de evento. Los errores de red del webhook se reducen a un código: no se conserva ningún mensaje.
- **Sin aislamiento por ACL.** El broker del MVP es anónimo: cualquier cliente de la LAN puede publicar en el topic de un nodo y, por tanto, un evento falso. El componente valida el contrato y que el nodo coincida con el topic, y deduplica, pero **no puede probar quién emitió el evento**. Uso solo en LAN de confianza.
- **TLS.** El webhook exige HTTPS con certificado válido (`ca_file` solo para una CA privada propia) y TLS 1.2 o superior; la API de estado solo se sirve por HTTPS. Ninguna verificación se desactiva. `http:` queda limitado al bucle local en pruebas.
- **No es un histórico.** La app lee el historial del nodo por MQTT/BLE. Este componente solo retiene lo necesario para deduplicar y reintentar.

## Pruebas

```sh
npm ci && npm run build -w contracts
cd notifications && npm ci && npm test
```

Las 72 pruebas incluyen un broker MQTT real en proceso y un receptor de webhook local, y cubren: configuración (rechazo de campos desconocidos, HTTP no local, credenciales en la URL, valores que no deben aparecer en los errores, el archivo de ejemplo); deduplicación y retransmisión; umbrales, ventanas y escalada; cola, reintentos, vencimiento, desbordamiento, reinicio y recuperación; estado atómico, corrupto o manipulado; HTTPS real con certificado autofirmado (se rechaza sin su CA y con nombre distinto); redirecciones no seguidas; API de estado (401, 403, 400, 413, 404, 405, CORS, aislamiento entre nodos, ausencia de secretos, respuesta validada con el contrato); persistencia antes de devolver el control al cliente MQTT; sesión persistente del broker con el componente caído; y el proceso real arrancado desde un archivo de configuración. Se comprobó además que 21 defectos introducidos a propósito (deduplicación, escalada, umbrales, espera de reintentos, límites, autenticación, CORS, verificación TLS, persistencia antes del ACK, etc.) hacen fallar la suite. Los eventos de las pruebas son los fixtures del contrato: son ficticios y **no** acreditan un incidente real del ESP32.

## Comprobación física pendiente (tareas 2.1 y 2.2)

Requiere el ESP32 con el Cebo del cambio [eventos e historial](../encargos/sentinel-cebo-eventos-historial.md) en una LAN propia, el broker del MVP, el componente en ejecución y un destino de webhook **propio** (por ejemplo, un receptor de prueba o un servicio HTTPS que controles). Registra PASS/FAIL/PENDIENTE, IDs de evento y conteos, sin tokens ni URL.

1. **2.1.** Con la configuración privada preparada y el componente en marcha, provoca un incidente real: seis `curl.exe -d "user=a&pass=b" http://IP/login` en menos de 60 s. Esperado: un único `POST` al webhook con `severity: warning` y el `event_id` del evento del nodo, y `GET /v1/status` con `last_delivery.status: provider_accepted`. Repite el incidente desde el mismo origen dentro de 30 s y comprueba que no llega otra alerta (el nodo ya suprime repeticiones durante 30 s y el componente añade su propia ventana, de modo que `suppressed` solo sube si llegan eventos de más).
2. **2.2.** Interrumpe el destino (o cámbialo a uno que responda 503), provoca otro incidente y comprueba `pending: 1` y `last_delivery.error: provider_unavailable`; restablécelo y verifica que llega **una sola** alerta con el mismo `event_id`. Reinicia el componente con la alerta pendiente y comprueba que se entrega tras arrancar. Detén el componente, genera un incidente y vuelve a arrancarlo: el broker conserva el evento y se entrega. La sesión persistente se probó con el broker en proceso de las pruebas; con Mosquitto es parte de esta comprobación.
3. La API de estado desde la app (P4) requiere un certificado en el que confíe el teléfono; hasta entonces solo se ha probado con clientes Node en las pruebas automáticas, no desde un teléfono ni con `curl`.

## Estado de verificación

| Comprobación | Resultado |
| --- | --- |
| Pruebas del paquete (72) con broker MQTT en proceso y receptor local | PASS (Windows, Node 24) |
| 21 defectos introducidos a propósito | Todos detectados |
| HTTPS del webhook y de la API con certificado autofirmado (OpenSSL 3.5) | PASS |
| `docker compose config` del Compose propio | PASS (sintaxis) |
| Construcción de la imagen y arranque con Docker | PENDIENTE (el motor de Docker no estaba en ejecución en este equipo) |
| 2.1 Alerta de un incidente real del ESP32 en el destino webhook | PENDIENTE |
| 2.2 Corte y restablecimiento del webhook con un evento real | PENDIENTE |
| Consulta de estado desde la app Android de P4 con certificado confiado | PENDIENTE |

## Puntos para coordinar

- **P4 (app, broker):** el endpoint es `https://IP-DEL-PC:8443/v1/status?device_id=<id>` con `Authorization: Bearer <credencial>`; P3 entrega la URL y la credencial por un canal privado. La app debe confiar en el certificado configurado y estar entre los `cors_origins`. El registro push (`/v1/registrations`) no existe todavía. El broker debe ser alcanzable desde el contenedor (`host.docker.internal` en el ejemplo) y no se usa ninguna credencial MQTT.
- **P1 (contratos):** no se pide ningún cambio. `NotificationStatusResponse` y los helpers se usan tal cual; el estado final expone únicamente `channels:{webhook,push}`.
- **Siguiente cambio:** el envío push añade registros, TTL, FCM y su propio estado sobre el mismo enrutador y la misma cola; no cambia el contrato de este componente.
