# Cebo: clasificación, métricas, configuración e historial

Responsable: Persona 3. Cambio: [sentinel-cebo-eventos-historial](../../openspec/changes/sentinel-cebo-eventos-historial/proposal.md) ([ficha](../encargos/sentinel-cebo-eventos-historial.md), [diseño](../../openspec/changes/sentinel-cebo-eventos-historial/design.md), [spec](../../openspec/changes/sentinel-cebo-eventos-historial/specs/cebo-events/spec.md), [tareas](../../openspec/changes/sentinel-cebo-eventos-historial/tasks.md)). Rama: `feat/sentinel-cebo-eventos-historial`, apilada sobre [Telnet](cebo-telnet.md) y [HTTP](cebo-http.md). Se integra con la [API de módulos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3), el [contrato v2](../../contracts/module-protocol.md) y la [política de persistencia](../persistence-policy.md).

Este cambio da a Cebo cuatro capacidades sobre las interacciones que ya producen HTTP y Telnet: clasificarlas por origen, contarlas y exponer métricas y `cebo.stats`, aceptar y persistir `cebo.configure`, y entregar los eventos al historial del núcleo. No añade servicios ni abre puertos nuevos. Los indicios de este documento son **conducta observada, no atribución**: una conexión o un acceso aislados no prueban una intrusión (R-10).

## Clasificación

La tabla tiene una entrada por **(IPv4 de origen, protocolo)**, con 32 entradas y desalojo del origen visto hace más tiempo (LRU). El reloj es la marca de tiempo de cada interacción, así que la lógica es determinista y se prueba con reloj controlado. Los valores son propuestas del diseño, ajustables si fallan en la práctica sin retirar la función.

| Parámetro | Valor | Efecto |
| --- | --- | --- |
| Ventana de conteo | 60 s (`WindowMs`) | Ventana fija que empieza con la primera interacción del origen y se reinicia pasados 60 s |
| Expiración | 5 min sin actividad (`ExpiryMs`) | El origen se olvida y vuelve a ser «aislado» |
| Medio | ≥ 5 intentos de login en la ventana (`MediumAuthAttempts`) | `suspicion: medium`, severidad `warning`, evidencia `repeated_auth` |
| Alto | ≥ 20 interacciones en la ventana (`HighInteractions`) | `suspicion: high`, severidad `critical`, evidencia `rate_threshold` |
| Enfriamiento | 30 s por origen y protocolo (`CooldownMs`) | Entre dos eventos del mismo origen |

«Interacciones» son conexiones, accesos, entradas e intentos de login. El cierre (`disconnect`) no suma actividad y nunca genera evento. Un nivel alto prevalece sobre uno medio.

### Qué se publica y qué se suprime

- Un acceso, una entrada o un intento de login publica un evento si el origen no está en enfriamiento. Sin repetición suficiente es `informational` (severidad `info`), con `count: 1` y `window_ms: 1000`.
- Una conexión **sola** no genera evento (no es evidencia). Sí lo genera si con ella el origen alcanza el nivel medio o alto: un barrido de 20 conexiones sin peticiones produce un evento `connect` con `rate_threshold`.
- **Una escalada de nivel se publica aunque el origen esté en enfriamiento**, porque es información nueva. Un origen publica, como máximo, un evento cada 30 s más una escalada por nivel y ventana.
- Un evento que el núcleo no acepta se reintenta durante 2 s y después se descarta y se cuenta (`publish_failed`). Solo un evento **aceptado** inicia el enfriamiento y marca el nivel como publicado: si se pierde, la siguiente interacción del origen vuelve a publicarse.
- **Los contadores no dependen de lo que se publique.** Cada interacción se cuenta antes de decidir si se suprime, de modo que `auth_attempts` puede ser mucho mayor que el número de alertas.

### Campos del evento

`count` y `window_ms` dependen del nivel, y se documentan aquí porque el contrato solo exige enteros positivos:

| Nivel | `count` | `window_ms` | `evidence` |
| --- | --- | --- | --- |
| informational | 1 | 1000 | código de la interacción |
| medium | intentos de login en la ventana | 60000 | código de la interacción, `repeated_auth` |
| high | interacciones en la ventana | 60000 | código de la interacción, `rate_threshold` |

Los puertos y la ruta son los de la interacción que desencadena el evento. Códigos propios de cada interacción: `connection_observed`, `canonical_access`, `auth_attempt`, `input_observed`. `repeated_connections` y `repeated_access` existen en el contrato pero este cambio no los emite: no hay umbral acordado para ellos y un barrido de conexiones o accesos se refleja como `rate_threshold`.

```json
{"source_ipv4":"192.0.2.3","source_port":45000,"destination_port":80,"protocol":"http","interaction":"auth_attempt","path":"/login","count":5,"window_ms":60000,"suspicion":"medium","evidence":["auth_attempt","repeated_auth"]}
```

### Límites conocidos

- La ventana es fija, no deslizante: cinco intentos repartidos entre el final de una ventana y el principio de otra pueden no sumar. El nivel alto (20 interacciones) también lo sufre.
- Un atacante que rota más de 32 IPv4 desaloja entradas antiguas y reinicia su estado. La LAN es de confianza y la tabla es finita por diseño.
- Una ráfaga distribuida entre muchos orígenes puede sobrescribir el anillo de 64 eventos del núcleo; el consumidor lo ve como un hueco explícito (ver [historial](#historial-y-sincronización)).
- Los umbrales son heurísticas iniciales (H-06). No hay medición estadística ni ajuste fino; no se afirma ningún falso positivo/negativo.

## Métricas y `cebo.stats`

`ModuleContext::publish_metrics` envía cada 5 s (`MetricsIntervalMs`) los valores v2 del ámbito `cebo`, con el tiempo real transcurrido desde la publicación anterior. `cebo.stats` devuelve los mismos valores a demanda (`{"kind":"cebo.stats","interval_ms":…,"values":…}`) sin alterar la cadencia periódica. Las métricas son efímeras y no se persisten (R-06).

| Campo | Significado |
| --- | --- |
| `http`/`telnet`.`connections`, `accesses`, `auth_attempts`, `inputs` | Acumulados desde que se entró en el modo Cebo, de **todas** las interacciones |
| `active_connections` | Conexiones abiertas de ese servicio, como máximo 6 |
| `interactions_per_minute` | Interacciones de los últimos 60 s (cubos de 1 s) |
| `alerts` (por servicio y total) | Eventos `medium`/`high` aceptados por el núcleo, no los suprimidos |
| `rejected_connections` | Conexiones rechazadas por el límite de conexiones |

`rejected_connections` cuenta lo que el módulo ve: todas las de Telnet y las de HTTP que llegan hasta el módulo. `esp_http_server` se configura con un socket más que el máximo admitido justamente para que el excedente llegue al módulo y se cuente en lugar de descartarse sin rastro; una ráfaga simultánea puede aun así perder alguna cuenta.

## `cebo.configure`

Argumentos del contrato: `http_enabled` y `telnet_enabled` (solo `true`: no se puede quitar un servicio), `max_connections` (1–6) y `idle_timeout_ms` (1000–60000). El núcleo valida el mensaje con el esquema v2 y el módulo lo vuelve a comprobar: tipos, enteros exactos y rangos; un valor inválido devuelve `invalid_args` sin tocar nada.

- `max_connections` es el total **entre los dos servicios**, que además conservan su tope propio (HTTP 4, Telnet 2). Una `Limiter` compartida admite o rechaza cada conexión nueva en ambos servicios. Un valor menor no corta las conexiones abiertas; limita las siguientes.
- `idle_timeout_ms` lo leen los dos servicios en cada barrido, de modo que se aplica sin reiniciar listeners.
- El ACK `applied` solo se devuelve **después del commit** en NVS. Si el commit falla se devuelve `resource_error` y se conserva la configuración anterior, también en RAM.
- Repetir la configuración vigente y confirmada no escribe en flash.

## Persistencia de la configuración

Sigue el patrón de la selección de Escudo y la [política de persistencia](../persistence-policy.md). Espacio de nombres `cebo_config`, clave `settings`, en la partición NVS estándar, con un registro de 13 bytes: cabecera `SCEB` + versión 1, `max_connections` y `idle_timeout_ms` en little-endian. No contiene credenciales, texto del cliente ni marcas de tiempo, y no se promete cifrado físico.

- **Restauración:** al entrar en el modo Cebo (`start`), el boot sigue en Idle y no se confía en ningún reloj. Ausencia significa valores por defecto (6 y 5000 ms). La restauración **no abre sockets**; los servicios arrancan como ya describen [HTTP](cebo-http.md#ciclo-de-vida-y-estado) y [Telnet](cebo-telnet.md).
- **Registro dañado o fuera de rango:** no se aplica, se usan los valores por defecto y se publica un evento `system.status` (`resource_error`, recurso `system`) con reintento hasta que el núcleo lo acepta. No se borra ni formatea nada. Una `cebo.configure` válida lo sobrescribe, aunque coincida con los valores por defecto.
- Reinstalar o actualizar por USB puede reinicializarlo; no hay migración.

## Historial y sincronización

El journal circular, el cursor, `read_events` y la retransmisión tras una caída del broker son del **núcleo (P1)**; Cebo no tiene journal ni almacén de eventos propios (R-06, H-07). Su contribución es llegar a él por el único camino permitido, `emit_event`.

1. `emit_event` copia y valida el evento sin escribir en flash ni en el transporte. El núcleo lo **persiste antes de publicarlo** (`persist_module_event`): asigna `journal_seq`, el identificador estable del mensaje y el cursor, y solo entonces lo envía por BLE y por MQTT con QoS 1.
2. Al reconectar el broker, el núcleo relee el journal desde el primer evento retenido y lo reenvía **con el mismo identificador**. QoS 1 puede duplicar: el consumidor deduplica por `(device_id, message_id)`.
3. `read_events` pagina por cursor. Si el anillo (64 eventos de hasta 2048 bytes) sobrescribió el cursor pedido, la respuesta lo indica con un hueco explícito (`gap`, `retained_from`).
4. Tras un reinicio, el journal persistido conserva los eventos y sus IDs. Las métricas, la tabla del clasificador y los contadores **no** sobreviven: son estado de un modo y se reinician al entrar en él.

El anillo y el hueco tienen pruebas en el núcleo (`firmware/test/core_test.cpp`, anillo pequeño y recuperación), que son del dueño del núcleo y dependen de una infraestructura de host solo para Linux; no se han ejecutado en este equipo. La coincidencia de `device_id` y topic en un broker anónimo no autentica al emisor.

## Pruebas

```sh
node firmware/test/cebo/run_host.mjs
```

Comprueban, con reloj controlado y un NVS simulado en memoria: aislado, repetido, escalada, enfriamiento, ventanas, expiración, desalojo LRU, confirmación tardía o no confirmada; contadores aun con eventos suprimidos; ritmo de 60 s; límite compartido; el almacén real (`config_store.cpp`) con commit antes del ACK, fallos de apertura, escritura y commit, registros dañados y ausencia de credenciales; `cebo.configure`, `cebo.stats` y restauración a través de los hooks del módulo. Se verificó además que 21 defectos introducidos a propósito (umbrales, enfriamiento, commit, límites, conteo de alertas, etc.) hacen fallar las pruebas.

```sh
npm run build -w contracts
node firmware/test/cebo/check_contracts.mjs
```

Pasa los eventos (las tres sospechas), las métricas, el ACK de `cebo.stats` (con su correlación con la orden), el diagnóstico `system.status` y el estado por el decodificador v2 compartido y comprueba que rechaza un campo extra, una ruta con consulta, `active_connections` mayor de 6 y cualquier par sospecha/severidad que no corresponda.

## Puntos para coordinar

- **P1 (núcleo):** no se pide ningún cambio en `core`, `runtime` ni contratos. `cebo.stats` no incrementa `revision` y `cebo.configure` sí (comportamiento ya existente del núcleo). `AutoStartServices` sigue en `true` hasta que la app tenga el control de Cebo; con él en `false` se arrancan con `cebo.start`. `SentinelConfig.h` es de P1 y el flag `SENTINEL_MODULE_CEBO` sigue sin activarse en este cambio.
- **P4 (app, broker):** el historial y las métricas de Cebo llegan por los topics `events` y `metrics` del contrato; `cebo.configure` y `cebo.stats` están disponibles por BLE o MQTT de Cebo. La interfaz de configuración y el historial en la app son de P4.

## Comprobación física pendiente (tareas 2.1 y 2.2)

Requiere ESP32, LAN propia o autorizada, broker anónimo del proyecto y una copia con `SENTINEL_MODULE_CEBO` en 1 solo en el equipo local. Datos ficticios; registra PASS/FAIL/PENDIENTE y el commit grabado.

1. **2.1, clasificación y eventos reales.** Con Cebo activo, desde otra máquina de la LAN: `curl.exe -i http://IP/` y esperar el evento `informational`. Repetir seis veces `curl.exe -i -d "user=a&pass=b" http://IP/login` en menos de 60 s: se esperan un evento informativo y, al quinto, uno `medium` con `count: 5`; el resto se suprime pero `auth_attempts` sigue subiendo.
2. Telnet: `telnet IP` y teclear tres intentos ficticios; repetir hasta superar 20 interacciones en 60 s para ver `high`. Comprobar `device_id` y topic en el broker (`sentinel/v2/nodes/<device_id>/events` y `…/metrics`) y que los valores de `cebo.stats` coinciden.
3. `cebo.configure` con `max_connections: 2` y comprobar que la tercera conexión se rechaza y aumenta `rejected_connections`.
4. **2.2, persistencia.** Con `max_connections: 3`, reiniciar la placa sin cargar firmware, volver al modo Cebo y comprobar en `get_state` que el valor se restauró y que no se abrió ningún puerto antes de entrar en el modo. Generar un evento, cortar el broker, generar otro, reiniciar la placa, restaurar el broker y comprobar que ambos eventos llegan con el **mismo `message_id`** y sin secretos; con un anillo lleno, que `read_events` informa el hueco.
5. No abrir puertos del router ni exponer la placa a Internet.

## Estado de verificación

| Comprobación | Resultado |
| --- | --- |
| Pruebas de host de Cebo (clasificación, métricas, límites, almacén, configuración, eventos) | PASS (Windows, MinGW; sin radio ni NVS reales) |
| 21 defectos introducidos a propósito en la lógica | Todos detectados por las pruebas |
| Eventos, métricas, `cebo.stats`, `system.status` y estado frente al decodificador v2 compartido | PASS (paquete de contratos instalado en una carpeta temporal; host) |
| Compilación del módulo con core oficial ESP32 3.3.12, ESP32S3 (sketch aislado con `esp_http_server`, lwIP y NVS) | PASS |
| Verificar del sketch completo con el flag activado | PENDIENTE (faltan wolfMQTT y la configuración de placa en este equipo) |
| 2.1 Clasificación e interacciones reales por MQTT sobre el broker anónimo | PENDIENTE |
| 2.2 Corte del broker, reinicio, mismo ID y hueco visible | PENDIENTE |
| Pruebas del anillo pequeño y recuperación del núcleo | Existentes en `core_test.cpp` (P1); no ejecutadas aquí |

## Cotejo con R-06, R-10 y H-06/H-07

| Decisión | Cómo se cumple |
| --- | --- |
| R-10 punto 3: metadatos mínimos | Solo IPv4, puertos, protocolo, tipo, ruta canónica, cantidad, ventana y evidencia; `Interaction` no tiene campo para credenciales o cuerpos |
| R-10 punto 4: actividad aislada, intentos y repetitiva | `informational`, `medium`, `high`; ninguna conexión por sí sola es alerta |
| R-10 punto 7 y R-06: offline y cursor | Journal del núcleo, persistido antes de publicar, reenvío con el mismo ID y hueco explícito |
| H-06: heurísticas iniciales, pruebas de clasificación y límites | Umbrales del diseño; pruebas focalizadas; sin ajuste estadístico |
| H-07: persistencia simple y acotada, dedup/gap, sin otro almacén | Un registro de 13 bytes y el journal compartido; capacidad finita y pérdidas visibles |
| Política de persistencia | Commit antes del ACK, restauración validada sin abrir sockets, error informado sin borrar, sin credenciales |
