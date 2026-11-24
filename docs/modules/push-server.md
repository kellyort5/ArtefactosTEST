# Push: registro de instalaciones y envío FCM

Responsable: Persona 3. Cambio: [sentinel-alertas-push-servidor](../../openspec/changes/sentinel-alertas-push-servidor/proposal.md) ([ficha](../encargos/sentinel-alertas-push-servidor.md), [diseño](../../openspec/changes/sentinel-alertas-push-servidor/design.md), [spec](../../openspec/changes/sentinel-alertas-push-servidor/specs/push-delivery/spec.md), [tareas](../../openspec/changes/sentinel-alertas-push-servidor/tasks.md)). Rama: `feat/sentinel-alertas-push-servidor`, apilada sobre [el componente del webhook](notifications.md). El código está en [`notifications/`](../../notifications/README.md).

Una app Android cerrada no mantiene una conexión MQTT. Este cambio añade al componente de notificaciones el segundo canal obligatorio de R-10: guarda las instalaciones Android autorizadas de cada nodo y, cuando llega un incidente de Cebo, pide a **Firebase Cloud Messaging** que despierte la app con una notificación visible. La app abre el incidente por su ID y consulta el historial autorizado: el push **no** transporta el evento ni sus datos.

**Alcance honesto.** Este cambio entrega el servidor. El envío a FCM está probado con un proveedor simulado y con la carga real del SDK oficial a partir de una cuenta de servicio ficticia; **no** se ha enviado ningún mensaje real porque no existe todavía un proyecto de Firebase, una credencial ni una instalación Android (ver [la comprobación pendiente](#comprobación-física-pendiente-tareas-21-y-22)). «FCM aceptó el envío» significa entregado al proveedor, nunca recibido ni leído en el teléfono: esa evidencia es del encargo Android.

## Flujo

1. **Registro.** La app obtiene su token FCM y lo registra con `POST /v1/registrations`. El servidor lo guarda, con un máximo de 32 instalaciones y 30 días de vigencia desde cada alta o renovación. Renovar el par `(nodo, instalación)` con otro token **sustituye** al anterior; no quedan dos activos.
2. **Incidente.** El enrutador del [componente](notifications.md) deduplica el evento por `(device_id, message_id)`, aplica la severidad mínima y la ventana de 30 s del canal push, y encola **un elemento por instalación del nodo**. La cola solo contiene identificadores: ni el token ni la IP de origen se copian a ella ni al estado.
3. **Envío.** Al enviar se lee el token del registro y se compone el mensaje con `makePushNotification` del contrato: `notification` con el texto fijo `Sentinel Node` / `Actividad detectada en Cebo. Abre la app para consultar el incidente.` y `data` con exactamente `device_id`, `event_id` (el `message_id` original, sin generar otro), `severity` y `route:"cebo"`. Se pide prioridad alta y una vida limitada (`ttl_seconds`, 3600 por defecto).
4. **Resultado.** FCM acepta, rechaza o falla. Cada caso actualiza el estado del nodo (`last_delivery`, `pending`, `suppressed`) sin marcar lectura ni recepción: `read_status` es siempre `unknown`.

## API de registro

Contrato en [contracts/notifications.md](../../contracts/notifications.md); todas las llamadas con `Authorization: Bearer <credencial del nodo>`, por HTTPS.

| Operación | Respuesta 200 |
| --- | --- |
| `POST /v1/registrations` con `{device_id, installation_id, platform:"android", token}` | `{device_id, installation_id, status:"registered"}` |
| `DELETE /v1/registrations/<installation_id>` | `{device_id, installation_id, status:"removed"}` |

- El `device_id` del cuerpo debe ser el del nodo de la credencial (403 si no). La baja actúa solo sobre el par del nodo autenticado, es idempotente (también para un par inexistente) y no revela si la instalación existe en otro nodo.
- El cuerpo admite hasta 4096 bytes (413 si no; se corta en cuanto se excede, sin esperar al final de la transmisión) y el token 1–2048 bytes sin controles ni espacios (400 si no). El contenido debe ser `application/json`.
- 409 si el par es nuevo y ya hay 32 instalaciones; 503 si el canal push no tiene credencial configurada; 500 si no se pudo guardar en disco, y en ese caso **no** se confirma el alta al cliente.
- Los errores llevan cuerpo vacío y ninguna respuesta contiene el token.

## Errores del proveedor

El adaptador (`src/fcm.js`) reduce los errores del SDK a los códigos del contrato y descarta su mensaje, que puede citar el token o la credencial.

| Error de FCM | Código del estado | Efecto |
| --- | --- | --- |
| `registration-token-not-registered`, `invalid-registration-token` | `invalid_registration` | Se **borra la instalación** y no se reintenta |
| `invalid-argument`, `invalid-recipient`, `invalid-payload`, `payload-size-limit-exceeded`… | `provider_rejected` | No se reintenta y la instalación se conserva: el contenido es fijo y está probado |
| `message-rate-exceeded`, `device-message-rate-exceeded` | `rate_limited` | Se reintenta con espera creciente |
| `authentication-error`, `mismatched-credential`, `third-party-auth-error`… | `provider_rejected` | Se reintenta: el operador puede corregir la credencial o el proyecto |
| `server-unavailable`, `internal-error`, red y cualquier otro | `provider_unavailable` | Se reintenta de 1 s a 30 s hasta que la alerta vence (24 h) |

Si la instalación se dio de baja o venció mientras la alerta esperaba, el elemento se descarta (`invalid_registration`) sin llamar al proveedor. Un token que falla no retrasa a las demás instalaciones del nodo, y el fallo de un canal no afecta al otro.

## Configuración y credencial

El bloque `push` del [archivo privado](../../notifications/README.md#configuración) solo indica dónde está la credencial y la vida del mensaje:

```json
"push": {"credential_file": "/run/notifications/fcm-service-account.json", "ttl_seconds": 3600}
```

1. En un **proyecto de Firebase propio** (el equipo lo crea; no existe uno ya creado), **Configuración del proyecto → Cuentas de servicio → Generar nueva clave privada**, y guarda el JSON como `notifications/config/fcm-service-account.json`. `notifications/config/*` está ignorado por Git: no lo añadas a un commit ni lo pegues en chats, registros o capturas.
2. Comprueba que la API de Firebase Cloud Messaging (v1) está habilitada en ese proyecto. La app Android (P4) necesita el `google-services.json` del mismo proyecto; eso es de su encargo y no se entrega al servidor.
3. En Linux, restringe el archivo (`chmod 600`) y su carpeta. Con Compose el archivo se monta de solo lectura en `/run/notifications`; el usuario sin privilegios del contenedor debe poder leerlo, así que ajusta su propietario en lugar de abrirlo a todos. El componente avisa con `push_credential_permissions` si es legible por otros usuarios (solo POSIX).
4. Arranca el componente. Con una credencial válida, `GET /v1/status` informa `push.configured: true`. Con una inválida registra `push_credential_unusable` con el **campo** que falla, nunca su valor, y el canal queda sin configurar: no afirma entrega y el registro responde 503. `configured` indica configuración, no conectividad con FCM.

Los tokens se guardan en `registrations.json` dentro del directorio de estado (volumen `notifications-state`), con permisos 0600 y escritura atómica. NVS no interviene: es un servidor. No se promete cifrado en reposo; protege el volumen como cualquier secreto.

## Decisiones (propuestas del diseño, a revisar)

El diseño pide revisar la elección del proveedor antes de añadir dependencias. Se aplica lo propuesto; conviene confirmarlo con el equipo antes de integrar.

| Decisión | Motivo y alternativa |
| --- | --- |
| FCM con el **SDK Admin oficial** (`firebase-admin` 14.5.1, solo `app` y `messaging`) | Es lo que propone el diseño y la vía oficial para enviar a Android. Añade unas 120 dependencias transitivas; la alternativa más ligera sería la API HTTP v1 con `google-auth-library`, a costa de mantener el cliente y el mapa de errores a mano |
| Todo el SDK detrás de `src/fcm.js` y de `provider.send({token, push})` | El resto del componente no conoce el proveedor; cambiarlo, o simularlo en pruebas, es una sustitución local |
| Un elemento de cola por (alerta, instalación) | Aísla fallos y reintentos de cada instalación; la cola guarda identificadores, no tokens |
| `invalid-argument` no borra la instalación | Esa respuesta puede deberse al contenido; borrar tokens por un fallo propio sería destructivo |
| Registro rechazado con 503 sin credencial | El contrato no lo lista; evita guardar secretos sin uso. Es una extensión a confirmar con P4 |
| Mismo estado por nodo, enrutador y cola del webhook | Lo pide el diseño: «reintentos/dedup por evento/canal reutilizados» |

## Pruebas

```sh
npm ci && npm run build -w contracts
cd notifications && npm ci && npm test
```

Las 117 pruebas del paquete incluyen, para este canal: el registro (alta, renovación con otro token, capacidad de 32, TTL de 30 días, persistencia con permisos privados, archivo dañado o manipulado); la API (acceso cruzado 403, 401, 400, 413 —declarado y transmitido—, 409, 503, 500 sin guardado, baja idempotente y limitada al nodo, ausencia del token en respuestas y registros); el canal (un push por instalación, contenido exacto y validado con el contrato, cola sin token ni IP, token inválido, fallo temporal, instalación dada de baja mientras espera, renovación con token nuevo, deduplicación); el adaptador (mapa de errores, mensaje a FCM, error sin token, carga de una cuenta de servicio **ficticia** con una clave RSA efímera en el SDK oficial, credenciales inválidas sin filtrar valores); y la integración con un broker MQTT real en proceso (webhook y push con el mismo ID original, fallos independientes, reinicio con instalaciones y envíos pendientes, credencial inutilizable). Se comprobó además que 22 defectos introducidos a propósito en estas piezas hacen fallar la suite. El proveedor de las pruebas es un **simulacro**: no acredita ningún envío real.

## Comprobación física pendiente (tareas 2.1 y 2.2)

Requiere un proyecto de Firebase propio con su credencial, el componente en ejecución con HTTPS, y una instalación Android de P4 con la app registrada. Registra PASS/FAIL/PENDIENTE y los IDs de evento, sin tokens.

1. **2.1.** Con la app instalada y registrada (`POST /v1/registrations`), provoca un incidente real del Cebo (seis intentos de login en 60 s). Esperado: `GET /v1/status` con `push.last_delivery.status: provider_accepted` y el mismo `event_id` que el webhook. La **recepción con la app cerrada** y la apertura del incidente al tocarla se registran en el encargo Android, por separado.
2. **2.2.** Provoca un fallo del proveedor (por ejemplo, retirando la credencial o sin red) y comprueba `push.last_delivery.error` y `pending`; restablécelo y comprueba que llega una sola notificación. Renueva el registro con otro token y comprueba que se usa el nuevo; elimínalo con `DELETE` y comprueba que no se envía más. Revisa que el registro del componente no contiene tokens.

## Estado de verificación

| Comprobación | Resultado |
| --- | --- |
| Pruebas del paquete (117; 1 se omite en Windows por ser de permisos POSIX) | PASS (Windows, Node 24) |
| 22 defectos introducidos a propósito en registro, API, canal y adaptador | Todos detectados |
| Carga de una cuenta de servicio ficticia con el SDK oficial, sin red | PASS |
| Mensaje a FCM compuesto con el contrato y probado con un proveedor simulado | PASS |
| Envío real a FCM y a una instalación Android | PENDIENTE (no hay proyecto, credencial ni instalación) |
| 2.1 Aceptación de FCM de un incidente real | PENDIENTE |
| 2.2 Fallo del proveedor, renovación y baja con un proveedor real | PENDIENTE |
| Recepción con la app cerrada y apertura del incidente | PENDIENTE (encargo Android) |

## Puntos para coordinar

- **P4 (app):** el flujo esperado es (1) obtener el token FCM con el plugin de Capacitor, (2) `POST /v1/registrations` con un `installation_id` estable por instalación, (3) **renovar antes de 30 días y cada vez que cambie el token**, (4) `DELETE` al desvincular el nodo y (5) tratar 409 (capacidad) y 503 (push sin configurar) como estados visibles. Al tocar la notificación, `data.route` es `cebo` y `data.event_id` identifica el incidente: abrirlo por su ID y consultar el historial autorizado; si el evento salió de retención, mostrarlo. Su origen debe figurar en `cors_origins`. Hace falta el `google-services.json` del proyecto de Firebase y permiso de notificaciones del sistema.
- **P1 (contratos):** no se pide ningún cambio; el 503 sin credencial es la única extensión sobre los códigos del contrato.
- **Operación:** quien crea el proyecto de Firebase administra la credencial y es quien debe revocarla si se filtra.
