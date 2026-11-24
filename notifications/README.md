# Sentinel Notifications

Componente de alertas externas de Sentinel Node. Lee los eventos de interacción de Cebo que el nodo publica por MQTT, deduplica, controla las repeticiones y los entrega por **webhook HTTPS**; además registra las instalaciones Android de cada nodo y les envía un **push FCM** con la misma identidad de evento, y publica su **estado** por una API HTTPS autenticada por nodo. El push y el webhook son canales independientes con su propia cola, y el push solo se activa si configuras una credencial de Firebase propia.

Documentación del módulo, decisiones y estado de verificación: [docs/modules/notifications.md](../docs/modules/notifications.md) (webhook) y [docs/modules/push-server.md](../docs/modules/push-server.md) (push). Contratos: [contracts/notifications.md](../contracts/notifications.md) y [protocolo v2](../contracts/module-protocol.md).

```text
ESP32 ──MQTT QoS 1──▶ Mosquitto anónimo ──▶ ingest (contrato v1/v2) ──▶ router ──┬─▶ cola webhook ──▶ webhook HTTPS (destino fijo)
 (events)                (LAN de confianza)   dedup (device,message_id)   severidad +  └─▶ cola push ─────▶ FCM ──▶ Android (app cerrada)
                                                                           ventana 30 s   (una por instalación, 1–30 s, 24 h)
                                              API HTTPS: estado y registro de instalaciones ◀── app (Bearer por nodo, CORS explícito)
```

## Requisitos

- Node **24** (el repositorio fija `.nvmrc` 24.13.1), npm 11.
- El broker MQTT del MVP ([broker/README.md](../broker/README.md)) accesible por TCP 1883 sin usuario ni contraseña.
- Un destino HTTPS de webhook **propio o autorizado**.
- Un certificado TLS para la API de estado, si la app la va a consultar.

## Instalación y pruebas

El paquete consume el de contratos por `file:../contracts`, que se instala y compila desde el repositorio.

```sh
npm ci                                    # raíz: dependencias de contratos (ajv)
npm run build -w contracts                # compila contracts/dist (ignorado por Git)
cd notifications
npm ci                                    # dependencias propias y lockfile propio
npm test                                  # 117 pruebas; usa un broker MQTT real en proceso (aedes)
```

`npm run setup` vuelve a compilar los contratos con el TypeScript de este paquete. Las pruebas HTTPS necesitan el binario `openssl` (se omiten si no está).

## Configuración

Todo está en **un archivo JSON privado**, fuera de Git (`notifications/config/*` está ignorado salvo el ejemplo). Se indica con `NOTIFY_CONFIG`.

```sh
cp config/notifications.example.json config/notifications.json
```

| Campo | Significado |
| --- | --- |
| `mqtt.url`, `mqtt.client_id` | Broker anónimo (`mqtt://IP-DEL-PC:1883`). El `client_id` identifica la sesión persistente |
| `nodes[]` | `device_id` autorizado y `token_sha256`: el hash de su credencial para la API de estado |
| `alerts.min_severity` | `info`, `warning` (por defecto) o `critical` |
| `alerts.include_simulated` | Si es `false` (por defecto) los eventos de origen `simulated` no alertan |
| `alerts.repeat_window_ms` | Ventana de control de repeticiones por canal y origen (30000 por defecto) |
| `webhook` | `url` HTTPS fija, `timeout_ms`, `auth_header` opcional y `ca_file` si el destino usa una CA privada. Sin este campo el canal queda «sin configurar» |
| `push` | `credential_file` (JSON de la cuenta de servicio de Firebase, **fuera de Git**) y `ttl_seconds` (vida del mensaje en FCM; 3600 por defecto, de 60 a 86400). Sin este campo el canal push queda «sin configurar» y el registro de instalaciones responde 503 |
| `status_api` | `host`, `port`, `tls.cert_file` y `tls.key_file`, y `cors_origins` explícitos |
| `state_dir` | Directorio privado del estado (`./state` por defecto) |

El secreto del webhook también puede venir de `NOTIFY_WEBHOOK_AUTH_VALUE`. Un valor inválido produce un error que nombra el **campo**, nunca su valor. `http:` solo se admite para el bucle local y con `allow_insecure_loopback` (pruebas).

### Credencial de cada nodo

```sh
npm run token
```

Imprime un bearer aleatorio de 256 bits y su SHA-256. El **bearer** se entrega una sola vez a quien consulta la API (la app) y **no se guarda**; solo el hash va en `nodes[].token_sha256`. No es una credencial MQTT ni del proveedor y no sustituye a ninguna.

### Certificado de la API de estado

La API solo se sirve por HTTPS. Para una LAN de pruebas, una CA propia y un certificado con la IP del PC:

```sh
openssl req -x509 -newkey rsa:2048 -nodes -keyout config/tls/server.key -out config/tls/server.crt -days 365 \
  -subj "/CN=sentinel-notifications" -addext "subjectAltName=IP:192.0.2.10,DNS:localhost"
```

La app Android debe confiar en esa CA (instalarla en el teléfono es una decisión de P4); no se desactiva la verificación en ningún lado.

## Arranque

```sh
NOTIFY_CONFIG=config/notifications.json npm start
```

o con Docker Compose (imagen y volumen propios; construcción desde la raíz del repositorio):

```sh
docker compose -f notifications/compose.yaml up -d --build
```

El estado vive en el volumen `notifications-state`; **no ejecutar `down -v`**. Dentro del contenedor, `mqtt.url` apunta a `host.docker.internal`.

## API HTTPS

Tres operaciones, todas con `Authorization: Bearer <credencial del nodo>`. El nodo lo determina la credencial, nunca el cuerpo ni la ruta.

| Operación | Efecto |
| --- | --- |
| `GET /v1/status?device_id=<id>` | Estado de los canales del nodo |
| `POST /v1/registrations` | Alta o renovación de una instalación Android: `{device_id, installation_id, platform:"android", token}` → `{device_id, installation_id, status:"registered"}` |
| `DELETE /v1/registrations/<installation_id>` | Baja idempotente dentro del nodo → `{device_id, installation_id, status:"removed"}` |

Un `device_id` distinto del de la credencial responde 403, y uno inexistente también, para no revelar qué nodos existen.

| Código | Cuándo |
| --- | --- |
| 200 | `{device_id, channels:{webhook, push}}`, validado con el contrato antes de enviarse |
| 400 | Entrada inválida: consulta con parámetros repetidos o desconocidos, cuerpo que no cumple el contrato (campo extra, plataforma distinta de `android`, token vacío, con espacios o de más de 2048 bytes), tipo de contenido que no es JSON, o cuerpo en un GET o DELETE |
| 401 | Bearer ausente o inválido |
| 403 | `device_id` de otro nodo |
| 404, 405 | Ruta o método desconocido |
| 409 | Registro nuevo con las 32 instalaciones ocupadas (renovar un par existente no consume plaza) |
| 503 | Registro push sin credencial de Firebase configurada |
| 413 | Cuerpo de más de 4096 bytes, declarado o transmitido |
| 500 | No se pudo guardar el registro en disco: no se confirma al cliente |

Los errores llevan cuerpo vacío. CORS solo responde a los orígenes de `cors_origins`, nunca con comodín; el preflight de otro origen responde 403. La respuesta no contiene tokens, destinos, URL, IP ni mensajes de error: cada canal informa `configured`, `last_delivery` (código `provider_accepted` o `failed` con un código del contrato), `pending` y `suppressed` del nodo que consulta. `provider_accepted` significa aceptado por el destino, **nunca leído** (`read_status` es siempre `unknown`).

## Qué recibe el webhook

`POST` con `Content-Type: application/json`, `Idempotency-Key: <device_id>/<event_id>`, `X-Sentinel-Attempt` y la cabecera de autenticación configurada. Cuerpo:

```json
{"schema":"sentinel.alert.v1","device_id":"sentinel-example","event_id":"boot-1:42","boot_id":"boot-1","name":"cebo.interaction","severity":"critical","origin":"real","uptime_ms":123456,"timestamp":null,"time_quality":"unknown","details":{"source_ipv4":"192.0.2.77","source_port":45000,"destination_port":23,"protocol":"telnet","interaction":"auth_attempt","path":null,"count":25,"window_ms":60000,"suspicion":"high","evidence":["auth_attempt","rate_threshold"]}}
```

`event_id` es el `message_id` original del evento, sin generar otro. El cuerpo contiene únicamente los metadatos que el contrato v2 permite (IP y puertos de origen, tipo, ruta canónica, cantidades y códigos de evidencia): **nunca** contraseñas, usuarios, cuerpos ni líneas del cliente, porque el firmware no los conserva. La IP de origen sí sale hacia el destino que configuraste: elige un destino que tengas autorizado para recibirla.

## Límites y retención

| Concepto | Valor |
| --- | --- |
| IDs de evento recordados (deduplicación) | 512, durante 24 h |
| Cola de entrega por canal | 128 alertas, vencen a las 24 h; si se llena se descarta la más antigua y se informa como `expired` |
| Reintentos | espera del canal de 1 s que se duplica hasta 30 s; un éxito la reinicia; un único intento en vuelo |
| Control de repeticiones | 30 s por canal, nodo, IP de origen y protocolo; una escalada de severidad no se suprime; lo suprimido se cuenta |
| Instalaciones push | 32 por componente, 30 días desde cada alta o renovación; el token vive solo en `registrations.json` (0600) |
| Cola push | un elemento por instalación y alerta, solo con identificadores; un token inválido o una instalación ausente se descarta sin reintentos |
| Tiempo del webhook | `timeout_ms` (5 s por defecto), sin seguir redirecciones; la respuesta se descarta |
| Estado | un JSON de menos de 2 MiB escrito de forma atómica; un archivo ilegible se aparta (`state.json.corrupt-*`), nunca se borra |
| API de estado | 32 conexiones, cabeceras de 16 KiB, tiempos de espera de 10 y 15 s |

## Seguridad y límites aprobados

- **MQTT anónimo, sin ACL.** El broker del MVP no autentica a quien publica: cualquier cliente de la LAN puede publicar en el topic de un nodo, y por tanto un evento falso. El componente valida cada mensaje con el contrato, exige que el nodo del mensaje coincida con el del topic y deduplica, pero **no puede probar quién emitió el evento**. Usa una LAN de confianza y no expongas el broker ni este componente a Internet. Un tema distinto (`commands`, `acks`, otro nodo) se descarta.
- **Entrega al menos una vez.** El estado se guarda antes de que mqtt.js confirme el mensaje (PUBACK) y la sesión del broker es persistente, de modo que un corte entre la recepción y el guardado provoca una retransmisión, no una pérdida; la deduplicación absorbe el duplicado. Un disco lleno no pierde la alerta: sigue en memoria y se informa el fallo.
- **Los secretos no se registran.** Los registros son JSON por línea con códigos e IDs de evento; nunca URL, cabeceras, cuerpos recibidos, tokens, credenciales de Firebase ni mensajes de error de red o del proveedor.
- **Datos mínimos hacia FCM.** El proveedor recibe un título y un texto fijos y `data` con solo `device_id`, `event_id` (el `message_id` original), `severity` y `route:"cebo"`: nunca la IP de origen, rutas, puertos ni credenciales. Un envío aceptado por FCM significa «entregado al proveedor», no recibido ni leído.
- **Sin base de datos ni histórico.** No es una plataforma de historial: la app lee el historial del nodo por MQTT/BLE.
- **Una sola instancia** por directorio de estado.

## Estructura

```text
src/config.js    validación de la configuración privada
src/ingest.js    mensaje MQTT → evento Cebo (contrato compartido)
src/router.js    deduplicación, umbral de severidad y control de repeticiones
src/channel.js   cola persistente y reintentos por canal
src/webhook.js   cliente HTTPS del destino
src/registry.js  instalaciones push (token privado, 32, 30 días)
src/push.js      canal push: una entrega por instalación
src/fcm.js       adaptador del SDK Admin de Firebase y mapa de errores
src/http-api.js  API: estado y registro, Bearer y CORS
src/state.js     estado acotado y atómico
src/service.js   ensamblado (MQTT, canales, API)
test/            pruebas con broker MQTT real en proceso
```
