# Contratos Sentinel v1 y v2

La API v1 de la base se conserva: sus esquemas, tipos y funciones siguen rechazando mensajes v2. Para nuevas funciones usar las entradas explícitas de [protocolo v2](module-protocol.md); para historial mixto, `decodeAnyMessage` conserva versión e identidad originales. Los [contratos HTTP y push](notifications.md) son independientes del sobre ESP32 y no implementan un servidor ni envío de notificaciones.

[Evidencia, revisión técnica y pendientes de cierre](evidence-v2.md).

Este paquete define y valida mensajes. La ejecución de modos, el cache de resultados del firmware, la autenticación efectiva de transportes y el servidor de notificaciones corresponden a sus cambios de implementación. No se presentan pruebas de contrato como evidencia física o entrega push real.

## Base v1

Interfaz compartida de la base aprobada D-04/D-05/D-06/D-07. JSON UTF-8, **máximo 2048 bytes** por mensaje completo (no caracteres). JSON Schema 2020-12 en `schemas/v1/`. Campos no declarados se rechazan, incluidos secretos en salidas públicas. Ejemplos exclusivamente ficticios. No hay detectores, captura, honeypot, NAT o notificaciones en esta base.

## Sobre y payloads exactos

Todo mensaje lleva `version:1`, `type:"state"|"metrics"|"event"|"command"|"ack"`, `device_id`, `boot_id`, `message_id` (identificadores ASCII `[A-Za-z0-9._:-]`, 1–64), `seq` y `uptime_ms` (enteros 0..9007199254740991), `timestamp` (UTC ISO8601 `...Z` o null), `time_quality:"unknown"|"app_sync"|"network_sync"` y `payload`. Con `unknown`, timestamp es null. Hora conocida y `set_time.args.timestamp` exigen UTC canónico `YYYY-MM-DDTHH:mm:ss[.fracción]Z`, con T/Z mayúsculas, años 0001..9999, calendario gregoriano real y segundos 00..59. La fracción solo admite dígitos y todo el timestamp tiene máximo 32 caracteres (hasta 11 dígitos fraccionarios dentro de ese límite); no se aceptan espacios, offsets, año 0000 ni leap seconds. Ninguna salida contiene argumentos privados.

Payload `state` (todos obligatorios):

```json
{"profile":"idle","phase":"idle","revision":0,"last_command_id":null,"transport":{"ble":"advertising","wifi":"disconnected","mqtt":"disconnected"},"peripherals":{"oled":false,"button":true},"capabilities":{"ble_control":true,"mqtt_control":true,"event_journal":true,"oled":false,"button":true,"capture":false,"detectors":false,"honeypot_http":false,"honeypot_telnet":false,"notifications":false,"sanitizador":false},"error":null}
```

`profile`: idle/escudo/cebo. `phase`: idle/transitioning/ready/error. `ble`: stopped/advertising/connected; wifi y mqtt: disconnected/connecting/connected/error. `error`: null o un código de la lista inferior. Capacidades futuras siempre false en v1 de base. Revisión solo cambia al aplicar perfil/configuración, no por diagnóstico/conectividad. `ready` significa recursos preparados.

Payload `metrics`: `{"free_heap_bytes":123456,"min_free_heap_bytes":120000,"journal_count":1}`; diagnóstico real, enteros ≥0, journal_count ≤64.

Payload `event`: `{"journal_seq":1,"name":"system.booted","profile":"idle","code":null}`. journal_seq entero ≥1 persistente; name es system.booted/system.profile_changed/system.error; profile enum anterior, code null o error. La identidad del evento es `(device_id,message_id)` y no cambia al replay/reboot. No hay texto libre para errores ni argumentos.

Payload `command`: campos obligatorios `command_id` (ID), `target_boot_id` (ID), `expected_revision` (entero ≥0), `expires_at_uptime_ms` (entero ≥0), `operation` y `args`:

| operation | args exactos | Canal |
| --- | --- | --- |
| get_state | `{}` | BLE/MQTT |
| set_mode | `{"mode":"idle"}` (idle/escudo/cebo/sanitizador) | BLE/MQTT |
| configure_connectivity | `{"ssid":"LAB-EXAMPLE","wifi_password":"example-only","mqtt_uri":"mqtts://broker.example:8883","mqtt_username":"node-example","mqtt_password":"example-only","ca_certificate":"-----BEGIN CERTIFICATE-----\nEXAMPLE\n-----END CERTIFICATE-----\n"}` | solo BLE protegido |
| set_time | `{"timestamp":"2026-01-01T00:00:00Z"}` | solo BLE protegido |
| read_events | `{"after_cursor":0,"limit":8}` (limit 1..8) | BLE/MQTT |
| start_capture / start_honeypot | `{}` | responde not_implemented |

Los campos de configuración no admiten NUL. Límites UTF-8 adicionales al esquema: ssid 1..32 bytes; wifi_password 0..63 bytes; mqtt_uri 1..256 caracteres y solo mqtts://host[:puerto], sin credenciales/ruta/query; mqtt_username 1..64; mqtt_password 1..128; ca_certificate 1..1536. El conjunto debe caber en 2048 bytes: una CA demasiado grande requiere un certificado de laboratorio compacto, no desactivar TLS ni fragmentar comandos lógicos mayores. Secretos solo RAM; no guardar comando completo en store/logs/caché de resultados. Operaciones futuras se reconocen solo para rechazarlas; no indican capacidad implementada.

Payload `ack`: `{"command_id":"cmd-1","status":"applied","revision":1,"error":null,"result":null}`. status accepted/applied/rejected; rejected exige error, otros error null. Para read_events applied, result es `{"next_cursor":8,"retained_from":1,"retained_to":8,"more":false,"gap":false}`. En ring vacío retained_from/to son 0. gap también puede señalar huecos internos de slots no recuperables; no exigir secuencias contiguas cuando el nodo reporta esa pérdida. Los eventos se entregan como mensajes individuales ANTES del ack applied de la página; no embutir una página en un mensaje. get_state entrega state individual y ack. Otros resultados null.

Errores: invalid_message, unsupported_version, unauthorized, wrong_boot, expired, revision_conflict, busy, id_conflict, not_implemented, resource_error. Rechazo autorizado lleva boot/revisión vigentes y correlación sin repetir args.

## Vigencia y reconciliación

Primero leer metadata BLE protegida o state MQTT reciente. Calcular expires_at_uptime_ms ≤ uptime recibido + tiempo transcurrido local + 30000; el firmware exige vencimiento futuro y como máximo 30 s desde su uptime. Orden de control: autorización/identidad/versión/tamaño/boot/vigencia → duplicados (digest y resultado sanitizado, 32) → revisión si nuevo → cola (8) → aplicación. Duplicado idéntico vigente devuelve resultado conocido; ID con otro contenido id_conflict. No reintentar órdenes vencidas ni de otro boot. Las consultas no modifican revisión. accepted/publicación/PUBACK no equivalen a applied. Si se pierde canal, consultar estado por otro autorizado: last_command_id y revisión permiten reconciliar; sin evidencia queda pendiente.

## BLE

Servicio `a90d0001-6d64-4f43-9e6d-9db51f1f0000`; metadata read `a90d0002-6d64-4f43-9e6d-9db51f1f0000`; entrada write con respuesta `a90d0003-6d64-4f43-9e6d-9db51f1f0000`; salida indicate `a90d0004-6d64-4f43-9e6d-9db51f1f0000`. Todas protegidas con LE Secure Connections + MITM + bond; createBond no demuestra por sí solo el nivel negociado, que impone firmware. Nuevo bond solo con ventana física 120 s y passkey OLED.

Metadata exacta ≤256 bytes UTF-8: `{"version":1,"device_id":"node-example","boot_id":"boot-example","revision":0,"uptime_ms":1000,"max_message_bytes":2048}`. Estado completo por get_state.

Cada fragmento tiene 8 bytes LE: transfer_id uint32 en 0, offset uint16 en 4, total uint16 en 6; siguen datos hasta MTU−11. Con MTU23 son 12 bytes, p.ej. datos de 25 bytes → offsets 0/12/24, total25, tamaños de paquete20/20/9. Un transfer por dirección/conexión, offsets contiguos y mismo total/ID, total 1..2048; timeout absoluto 10 s desde primer fragmento y limpieza al desconectar. Desorden/exceso/intercalado descarta ensamblado; nunca decodificar JSON parcial. MTU grande es opcional.

## MQTT 5

Prefijo `sentinel/v1/nodes/<device_id>/`. state/availability QoS1 retained, metrics QoS0 no retained, events/commands/acks QoS1 no retained. availability es texto ASCII `online`/`offline` (LWT), no un sexto tipo de sobre. Sesión Clean Start y Session Expiry 0. Rechazar comandos retained en firmware. Estado periódico5 s; app marca antiguo tras15 s y un retained aislado no demuestra presencia. Operador publica solo commands de su nodo; observador solo recibe. configure_connectivity/set_time prohibidos por MQTT. WSS exige CA/hostname/fecha confiables, nunca fallback claro. Credenciales app solo sesión en RAM.

## API TypeScript

Workspace `@sentinel/contracts`: tipos `Message`, `StateMessage`, `EventMessage`, `AckMessage`, `CommandMessage`, `CommandRequest`, `Metadata`, `StatePayload`; constantes `MAX_MESSAGE_BYTES`, `BLE_UUIDS`; `encodeMessage(message):Uint8Array`, `decodeMessage(bytes|string):Message`, `decodeMetadata(bytes):Metadata`, `makeCommand(metadata,request,commandId,elapsedMs?):CommandMessage`, `fragmentMessage(bytes,transferId,mtu?):Uint8Array[]`, `new BleAssembler().push(fragment,nowMs):Uint8Array|null`, `.reset()`. Errores del codec son códigos sanitizados, jamás contenido recibido. Tests/fixtures: `npm run check:contracts`. Replay local app guarda últimos256 eventos/nodo y cursor confirmado por página en IndexedDB; no adelantar cursor solo por recibir eventos en vivo (podrían faltar anteriores).
