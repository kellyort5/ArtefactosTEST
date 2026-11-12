# Escudo: captura, parser y canales

Responsable: Persona 1. Cambio: [sentinel-escudo-captura](../../openspec/changes/sentinel-escudo-captura/proposal.md), sobre [descubrimiento archivado](../../openspec/changes/archive/2026-11-11-sentinel-escudo-descubrimiento/closure.md). Usa los contratos v2 existentes, el supervisor, el gestor y la cola de radio compartidos. No agrega servicios, instancias MQTT ni formatos de mensajes.

El componente implementa captura promiscua de administración, parser, control y cobertura. `capabilities.capture` pasa a `true`; `detectors` sigue en `false`. Los detectores de Deauth, desconexión sospechosa, Evil Twin y Karma conservan su alcance en el siguiente encargo; los contadores `alerts` quedan en cero porque todavía no se ejecutan esos detectores, no porque se haya demostrado que la red es segura.

## Uso desde BLE v2

Usar metadata y revisión actuales, un `command_id` nuevo por orden y el framing/autorización comunes de [module-protocol.md](../../contracts/module-protocol.md). Estos JSON son requests para `makeModuleCommand`, no mensajes completos ni escrituras GATT directas. La app necesita la adaptación v2 de Persona 4.

1. Entrar en Escudo, escanear y elegir una tupla observada conforme a [descubrimiento](escudo-discovery.md).
2. Iniciar la captura. `accepted` confirma recepción; `applied` confirma que el gestor habilitó la captura. Las observaciones llegan después, no se fabrican al confirmar.

```json
{"operation":"escudo.capture_start","args":{}}
```

Sin objetivo, se rechaza `invalid_state`. Con objetivo fuera del plan de país o ante un fallo SDK, se rechaza con error y el subestado muestra el fallo. Un inicio ya activo es idempotente y no cambia la generación.

3. Consultar el subestado, canal, cobertura, métricas y errores:

```json
{"operation":"get_state","args":{}}
```

4. Cambiar a una tupla presente en el catálogo. Si está capturando, se cierra la generación anterior y se reinicia sobre el canal elegido. La validación y construcción del resultado ocurren antes de modificar el objetivo. Una tupla inválida conserva objetivo y captura anteriores. Si falla el reinicio de una tupla válida, el objetivo anterior se conserva pero la captura queda en error; no se anuncia continuidad inexistente.

```json
{"operation":"escudo.select_target","args":{"ssid":"LAB","bssid":"02:00:00:00:00:02","channel":11}}
```

5. Detener sin salir de Escudo:

```json
{"operation":"escudo.capture_stop","args":{}}
```

Detener una captura ya parada es idempotente. Si el SDK no confirma la parada, se devuelve `resource_error`, se invalida la generación lógica y el estado es `error`; no se afirma que el hardware haya liberado el recurso. Al salir de modo, el supervisor reintenta la limpieza y aplica su timeout de 5 s sin iniciar otro modo.

6. También se puede volver a escanear durante captura. Se pausa la captura, se ejecuta el scan pasivo y se reanuda sobre el objetivo anterior al terminar o recibir una finalización SDK de error. `capture_stop` durante el scan cancela esa reanudación. Un timeout del scan exige pasar por Idle antes de recuperar, conforme a descubrimiento.

```json
{"operation":"escudo.scan","args":{}}
```

7. Salir a Idle detiene primero el componente y después libera la red por el gestor:

```json
{"operation":"set_mode","args":{"mode":"idle"}}
```

## Ingreso y parser

El callback SDK filtra administración. El gestor retira los cuatro bytes FCS declarados por `rx_ctrl.sig_len` y utiliza `CaptureIngress` para comprobar tamaño/estado, copiar hasta 512 bytes y marcar la generación. La cola FreeRTOS existente tiene ocho entradas, con envío sin espera. No hay JSON, flash, logs, asignación dinámica ni análisis de IE en el callback. Un paquete mayor que el límite, erróneo o sin cabecera completa se descarta y se contabiliza.

La generación se cierra antes de solicitar la desactivación al SDK. Una copia pendiente con generación anterior se rechaza tanto al ingresar a la cola como al consumirla. Inicio posterior, cambio de objetivo y reanudación tras scan usan una generación nueva; no se mezclan observaciones de sesiones cerradas.

El parser se ejecuta desde `tick`, fuera del callback, y produce únicamente `Observation` acotada:

- Beacon y Probe Response: direcciones, SSID de hasta 32 bytes, canal recibido y canal anunciado cuando existe, RSSI, secuencia/retry, privacidad y seguridad WPA/RSN observada.
- Deauthentication y Disassociation: direcciones, tipo, secuencia y razón cuando el cuerpo es legible. Una trama con bit Protected conserva el indicio de cabecera y no inventa una razón de un cuerpo cifrado; no se descifra ni verifica su autenticidad.
- Probe Request: SSID consultado —incluido wildcard vacío— y metadatos de cabecera. Sus IE no se presentan como configuración de seguridad de un AP. No se envían Probe Requests ni respuestas.

Se validan longitudes, tipo/subtipo, dirección individual de emisor/BSSID cuando corresponde, fragmentación, IE, contadores/listas WPA/RSN y campos opcionales. Truncamiento, SSID/RSN ambiguos, contador que excede los bytes disponibles o PMF requerido sin capacidad se rechazan. Las tramas fragmentadas no se reensamblan. Elementos desconocidos con longitud válida se omiten; suites no representables se informan `unknown`. No se considera que una coincidencia de SSID pruebe malicia.

PMF procede de los bits anunciados de RSN, no de inferirlo a partir del nombre WPA3. Se mantienen el bit Protected y `reason_known` por separado: no son una verificación criptográfica. SSID binario se conserva como bytes/longitud internos, sin asumir terminación NUL.

El punto `Escudo::observe(ObservationSink,void*)` permite que el próximo incremento conecte un consumidor de observaciones para todos los detectores. Corre en el supervisor; la referencia solo vive durante la llamada y el consumidor debe copiar de forma acotada si necesita conservarla. El punto no inicia drivers ni retiene payloads completos. No hay mensajes nuevos de tramas crudas por BLE: los contratos vigentes exponen métricas y cobertura; los incidentes estructurados corresponderán a los detectores.

Los formatos se contrastaron con las [definiciones de administración 802.11 de Linux v6.12](https://raw.githubusercontent.com/torvalds/linux/v6.12/include/linux/ieee80211.h), y con `esp_wifi_types_native.h`/`wpa_common.h` del ESP-IDF 5.5.3 instalado. La implementación del parser es propia; no copia código de esas definiciones.

## Canales, cobertura y pérdidas

El gestor obtiene el plan de `esp_wifi_get_country`: únicamente canales 1–14 que pertenezcan al país configurado. No se asocia para aprender otro país ni modifica el plan. Si no hay plan utilizable, no comienza captura. Cada inicio consulta nuevamente el plan y valida el canal objetivo.

El scheduler inicial dedica **800 ms al objetivo y 200 ms a otro canal permitido**, rotando los demás canales. Si solo está permitido el objetivo, permanece allí. Tras un retraso del supervisor se registra el tiempo realmente transcurrido y se programa el siguiente cambio; no se inventan intervalos de recuperación ni se realizan saltos acumulados para simular cobertura.

- `metrics.scope="escudo"`: contadores acumulados por boot de Beacon/Deauth/Disassoc, tiempo de captura/pausa y descartes. Probe Request/Response quedan disponibles en las observaciones y estadísticas internas para Karma, sin agregar campos al contrato.
- Las métricas se intentan cada segundo y al cerrar un tramo antes de saltar de canal. `channel` identifica ese canal observado; los contadores siguen siendo acumulados de todos los canales. No asumir una partición exacta por canal si se perdieron muestras.
- `escudo.coverage` se emite al iniciar, pausar, parar o reanudar y como máximo periódicamente cada 5 s. Se guarda por el journal compartido; no se escribe un evento por cada trama ni se llena el journal con cada salto automático.
- `coverage.started_uptime_ms/duration_ms` describen el tramo actual del controlador. En estado running, `channel_hop` indica observación de un canal alternativo y ausencia del objetivo; no significa que el receptor esté apagado. Durante un scan, el mismo motivo identifica radio dedicada a descubrimiento con captura pausada y canal null.
- `coverage_ms` suma tiempo del controlador en running, incluso sobre canales alternativos; `paused_ms` suma pausas solicitadas, como rescan. La parada manual se distingue de una pausa. Estos valores no miden recepción RF continua: BLE comparte radio, los saltos llevan tiempo y puede haber pérdidas.
- `dropped_frames` reúne errores/desbordamiento del ingreso, tramas malformadas y generaciones cerradas; `coverage.dropped_events` cuenta avisos de cobertura/diagnóstico que el núcleo no aceptó. Las métricas son efímeras; la cola de salida común también contabiliza saturación.

El firmware nunca inicia asociación, NAT, DNS ni MQTT desde Escudo. No persiste tramas ni cuerpos completos: solo estadísticas, cobertura y las salidas tipadas de los incrementos implementados. No atribuye una MAC a una persona ni declara maliciosa una red con estas observaciones.

## Verificación de software

```sh
pio run -d firmware
python3 firmware/test/check_config.py
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH python3 firmware/test/check_contracts.py
```

El build compila y enlaza `capture.cpp`, `frame_parser.cpp` y `channel_scheduler.cpp`. Las pruebas focalizadas reutilizan el código de ingreso y el componente real con un recurso de radio de host: cola acotada, generaciones cerradas, IE/RSN truncados/válidos, cuerpos protegidos, prioridad/rotación, tiempos con reloj controlado, cambio de objetivo, pausa por scan, métricas y parada fallida. Los mensajes del sink de host se identifican como simulados y se contrastan con el codec v2 y correlación de ACK. Esto no acredita recepción ni convivencia Wi-Fi/BLE físicas.

## Recorrido físico pendiente

Mantener abiertas las tareas 2.1 y 2.2 hasta ejecutarlas en ESP32-S3 N16R8, con OLED/botón y cliente BLE v2 autorizado, según [cableado y carga](../firmware.md).

1. Identificar puerto real, compilar, grabar y observar arranque. Emparejar por el mecanismo de la base; leer metadata y entrar en Escudo.
2. Escanear una LAN propia, identificar objetivo y otra red propia en distinto canal permitido y seleccionar el objetivo.
3. Iniciar captura desde teléfono. Comprobar ACK, `capture_status`, generación de observaciones internas y crecimiento de Beacon/Deauth/Disassoc según tráfico observado; la ausencia de ataques no requiere fabricar alertas.
4. Recibir por BLE métricas y cobertura. Registrar canales/tiempos; correlacionar las observaciones de los BSSID propios con los cambios de canal y contadores. Los resúmenes BLE no publican una trama por BSSID: no atribuir un incremento a una red ajena solo por el canal.
5. Detener/reanudar, cambiar objetivo y salir a Idle durante captura; verificar ausencia de observaciones de generaciones cerradas, recursos residuales y éxito prematuro ante un fallo.
6. Comprobar en la LAN autorizada que el nodo no se asocia ni transmite probes y que el control BLE responde durante captura. Registrar pérdidas/pausas reales sin prometer cobertura continua ni considerar una compilación como prueba de tráfico.
7. Conservar evidencia en [verification.md](../../openspec/changes/sentinel-escudo-captura/verification.md). Siguen vigentes las pruebas físicas archivadas de API y descubrimiento; no se completan por suposición.
