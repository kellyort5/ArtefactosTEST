# Contrato limitado de notificaciones

Este contrato de `sentinel-module-contracts` define datos ejecutables y ejemplos para
Persona 3 (componente de notificaciones) y Persona 4 (Android). No implementa un
servidor, autenticación bearer, almacenamiento, renovación efectiva, outbox,
Firebase ni envío. Las pruebas de fixtures no acreditan entrega push ni recepción
con la app cerrada. Esas funciones pertenecen a los cambios de sus consumidores.

## HTTP y autorización

La futura API estará en HTTPS de LAN con CA confiada y CORS de orígenes explícitos.
Cada llamada exige `Authorization: Bearer <credencial propia del nodo>` configurada
fuera de Git. El adaptador autentica esa credencial y obtiene el nodo autorizado;
no debe tomarlo del cuerpo para establecer permisos. Nunca se usan las credenciales
de MQTT, del proveedor o del Cebo como bearer de esta API.

Todos los helpers reciben explícitamente `authorizedDeviceId` y rechazan una
identidad distinta. Solo validan el contexto del contrato: pasarles un nodo no
autentica ningún bearer. El adaptador debe rechazar parámetros query repetidos o
desconocidos, métodos/rutas desconocidos y cuerpos en GET/DELETE. Los helpers de
GET/DELETE reciben su representación JSON normalizada, no interpretan HTTP crudo.

| Operación | Entrada validada | Respuesta HTTP 200 |
| --- | --- | --- |
| `POST /v1/registrations` | `{device_id,installation_id,platform:"android",token}` | `{device_id,installation_id,status:"registered"}` |
| `DELETE /v1/registrations/<installation_id>` | `{device_id,installation_id}` normalizado por el adaptador desde ruta/contexto | `{device_id,installation_id,status:"removed"}` |
| `GET /v1/status?device_id=...` | `{device_id}` desde query | `{device_id,channels:{webhook,telegram,push}}` |

DELETE opera dentro del nodo autorizado, sobre el par `(device_id,installation_id)`.
Si se resuelve una instalación existente, el `device_id` suministrado al helper debe
ser su propietario real y se compara contra el nodo autenticado. El servidor no
debe buscar una instalación global y borrar el registro de otro nodo. Una baja
repetida o de un par inexistente es idempotente y responde `removed`, sin divulgar
si existe un registro para otro nodo. Los IDs de respuesta deben corresponder a
los de la petición; los helpers validan estructura y nodo, y la correlación de la
instalación sigue siendo responsabilidad del adaptador HTTP.

El futuro servidor responde 400 ante entrada inválida, 401 ante bearer ausente o
inválido, 403 ante cruce de nodo, 413 ante cuerpo mayor de 4096 bytes y 409 al
agotar capacidad para un nuevo registro. Los fixtures de error usan cuerpo vacío:
este incremento no introduce un formato de error HTTP. Los errores de los helpers
son internos y no incluyen datos de entrada ni excepciones del proveedor.

## Tamaño, forma y ciclo de vida

- El cuerpo JSON completo admite hasta **4096 bytes UTF-8**, incluido whitespace.
  Este límite es independiente del sobre ESP/BLE/MQTT de 2048 bytes. Los helpers
  aplican también 4096 a las representaciones normalizadas y respuestas/push.
- El token admite **1–2048 bytes UTF-8**, sin controles ni espacios ASCII; es opaco,
  sin expresar un formato propio de FCM. JSON Schema limita longitud y forma; el
  codec añade el límite de bytes. Un token Unicode puede alcanzar el límite de
  bytes antes que el de caracteres. El llamante no debe registrar el resultado del
  decoder del registro porque contiene el token secreto.
- IDs: 1–64 caracteres ASCII de `[A-Za-z0-9._:-]`, sin espacios, separadores de
  ruta, controles ni salto de línea final. Solo se admite plataforma `android`.
- Todos los objetos son cerrados, incluidos objetos anidados. No hay metadatos
  libres, destinos, IP, chat privado, headers, cuerpos de solicitudes o detalles
  del proveedor en respuestas, estado o push. La notificación visible tiene texto
  genérico fijo, por lo que tampoco admite texto arbitrario con secretos.
- Los helpers aceptan `string | Uint8Array`. Rechazan UTF-8 inválido, BOM inicial,
  JSON inválido y surrogates aislados, también escapados en JSON. Los errores
  `NotificationContractError` solo exponen nombre/código fijo; no adjuntan tokens,
  mensajes de parseo ni detalles AJV. No registrar peticiones ni tokens en logs.
- Contrato del futuro servidor: **máximo 32 registros activos por componente**,
  con claves por nodo e instalación; una renovación del par existente no consume
  una plaza nueva. **TTL de 30 días** desde cada POST aceptado. Renovar al cambiar
  token y antes de vencer; reemplazar el token anterior sin mantenerlo activo.
  Al desvincular, eliminar con DELETE. Eliminar también tokens inválidos/expirados.
  Estas constantes documentan el contrato: no existe un registro en memoria ni
  persistente en este paquete y los helpers no simulan capacidad/TTL.

Solo el futuro componente guardará token y metadatos mínimos de propietario,
instalación, plataforma y vencimiento, en su directorio privado ignorado con
permisos restrictivos. Nunca en Git, respuesta, evento o log. Las respuestas
omiten token, conteos de registros y fechas de renovación para mantenerlas mínimas;
el cliente usa el TTL documentado, no una supuesta fecha retornada.

## Estado de canales y entrega

Cada canal obligatorio (`webhook`, `telegram`, `push`) tiene exactamente:

```json
{
  "configured": true,
  "last_delivery": {
    "event_id": "event-cebo-example",
    "timestamp": "2026-11-10T12:00:00Z",
    "status": "provider_accepted",
    "read_status": "unknown",
    "error": null
  },
  "pending": 0,
  "suppressed": 2
}
```

`last_delivery:null` significa que no hay intento registrado. Su timestamp es UTC
canónico de un intento/resultado conocido, con calendario validado y hasta 11
decimales de segundo (máximo 32 caracteres). Los contadores son enteros seguros
no negativos. `configured` señala configuración, no conectividad ni ejecución.

`provider_accepted` significa aceptación por el endpoint/proveedor del canal,
no recepción del teléfono ni lectura. `read_status` siempre es `unknown`; este
contrato no admite `read`, `delivered` ni confirma lectura. Un fallo usa
`status:"failed"` y uno de `provider_unavailable`, `provider_rejected`,
`invalid_registration`, `rate_limited`, `expired`, `internal_error`.
La aceptación exige `error:null`; un fallo exige código. No se devuelven mensajes
arbitrarios, IDs de proveedor, URLs privadas ni tokens.

## Push e identidad original

`notification` contiene exactamente el título `Sentinel Node` y el cuerpo
`Actividad detectada en Cebo. Abre la app para consultar el incidente.`.
`data` contiene únicamente strings `{device_id,event_id,severity,route:"cebo"}`;
`severity` es `info|warning|critical`. No incluye token, instalación ni destinos.
El adaptador del proveedor seleccionará el destino por separado y en privado.

`makePushNotification(originalEvent, authorizedDeviceId)` proyecta exclusivamente
identidad y severidad de un evento Cebo ya validado. `decodePushNotification`
requiere también ese evento para comparar nodo, `message_id` original y severidad.
Nunca genera un ID: `data.event_id === originalEvent.message_id`; deduplicar por
`(device_id,event_id)`. `NotificationEventReference` describe la proyección requerida
de un evento con `payload.name:"cebo.interaction"`, `profile:"cebo"` y severidad.
No reemplaza al validador del sobre de evento; acepta estructuralmente el evento
completo, sin copiar otros campos. El fixture `origin-reference.json` es esa
proyección, explícitamente **no** un sobre de firmware completo.
`origin-event.json` contiene el evento v2 completo del ejemplo, validado contra su
esquema, y usa exactamente el mismo `message_id` que el `event_id` del push.

Al tocar la notificación, Android abre Cebo por su ID y consulta el historial
autorizado cuando haya canal. Si el evento salió de retención, debe mostrarlo;
la notificación no garantiza disponibilidad del detalle ni reconstruye el evento.

## Exportaciones y ejemplos

El módulo autónomo [src/notifications.ts](src/notifications.ts) no importa runtime
del índice; la integración reexporta desde allí. Sus ocho funciones públicas son:

- `decodeNotificationRegistrationRequest`, `decodeNotificationRegistrationResponse`.
- `decodeNotificationUnregistrationRequest`, `decodeNotificationUnregistrationResponse`.
- `decodeNotificationStatusRequest`, `decodeNotificationStatusResponse`.
- `decodePushNotification`, `makePushNotification`.

Cada forma tiene su esquema `schemas/v2/notification-*.schema.json`:
`registration-request`, `registration-response`, `unregistration-request`,
`unregistration-response`, `status-request`, `status-response`, `push`.
Los esquemas solos no autentican, no comparan contexto/origen ni cuentan bytes
UTF-8: usar los helpers además de JSON Schema para esas invariantes.

Los tipos públicos exactos son `NotificationInput`, `NotificationSeverity`,
`NotificationContractErrorCode`, `NotificationRegistrationRequest`,
`NotificationRegistrationResponse`, `NotificationUnregistrationRequest`,
`NotificationUnregistrationResponse`, `NotificationStatusRequest`,
`NotificationDeliveryError`, `NotificationDelivery`, `NotificationChannelStatus`,
`NotificationStatusResponse`, `NotificationEventReference`, `PushNotification`.
Además se exporta la clase `NotificationContractError` y las constantes
`MAX_NOTIFICATION_BODY_BYTES`, `MAX_NOTIFICATION_TOKEN_BYTES`,
`MAX_NOTIFICATION_REGISTRATIONS`, `NOTIFICATION_REGISTRATION_TTL_DAYS`.
Sus códigos sanitizados son `invalid_notification`, `notification_too_large`,
`token_too_large`, `unauthorized_device`, `event_identity_mismatch`.

[fixtures/v2/notifications/](fixtures/v2/notifications/) contiene cuerpos JSON
válidos/invalidos, referencia original y transcripciones HTTP de registro,
renovación, baja, estado y rechazo. Todos los valores `FICTIONAL-*` y hosts
`.invalid` son ficticios y no utilizables. Renovación y baja son ejemplos de
contrato, sin llamadas de red ni operaciones reales.

Verificar desde la raíz con Node 24:

```sh
node node_modules/typescript/bin/tsc -p contracts/tsconfig.json
node --test --test-isolation=none contracts/test/notifications.test.mjs
```

Las pruebas verifican fixtures, objetos cerrados, límites exactos UTF-8/cuerpo,
errores sanitizados, aislamiento entre nodos, correlación original y la distinción
entre aceptación y lectura. Ningún servidor ni proveedor se ejecuta.
