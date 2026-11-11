# Evidencia de firmware: sentinel-core-foundation

Ámbito exclusivo: `firmware/`, `docs/firmware.md`, `docs/connectivity.md`, grupos3/4/6. Rama `feat/sentinel-core-foundation`; sin commits, push, merge, cambio de rama ni edición de contracts/specs. El wire se alineó con contracts/README.md y schemas v1 finales.

## Implementación

- `platformio.ini`, CMake, requirements, sdkconfig defaults/efectivo y particiones: PlatformIO6.2.0, espressif32@6.13.0, ESP-IDF5.5.3, flash16 MiB y PSRAM octal8 MiB; NVS independiente event_log256 KiB.
- `components/sentinel/core.cpp`, `digest.cpp`, `wire.cpp`: parser cJSON estricto/acotado con UTF8/NUL/claves/tipos/calendario/URI, gate de boot/vigencia/revisión, SHA256 canónico, resultados32 y admisión de duplicados pendientes/cola8; perfiles excluyentes y ring64×2048.
- `runtime.cpp`: identidad NVS, boot nuevo, supervisor FreeRTOS único, commits NVS sin erase automático, memoria detectada, comandos reales, eventos y replay/paginación; estado/capacidades honestos y reconciliación last_command_id/revisión.
- `peripherals.cpp` y Kconfig: driver esp_lcd SSD1306/I2C, GPIO8/9/4 por defecto, debounce50 ms, corto al liberar, largo3 s/ventana120 s sin doble acción; fallo OLED impide pairing nuevo.
- `ble.cpp`: NimBLE real, SC-only/MITM/cifrado128/bonding4, ventana física/passkey OLED, GATT/CCCD protegidos, metadata larga coherente, framing LE8/2048/timeout10 s, indications confirmadas y limpieza de sesión.
- `network.cpp`: esp_netif/esp_wifi STA solo Cebo, NVS Wi-Fi deshabilitado y almacenamiento RAM, credenciales RAM, esp-mqtt5 con CA/hostname/fecha obligatorios, Clean Start/expiry0, backoff1…30 s, topics/QoS/retain, LWT offline también al parar, estado inmediato/periódico5 s y replay del ring sin borrar por PUBACK.

No hay captura, detectores, honeypot, notificaciones externas, AP/NAT ni servicios ficticiamente terminados. Secretos solo en buffers RAM privados, borrados al procesar/reconfigurar; las salidas/cache/NVS de eventos no contienen argumentos de provisión.

## Comandos y resultados

Ejecutados desde la raíz del repositorio, con Node24 en PATH cuando corresponde:

| Comando | Resultado |
| --- | --- |
| `pio run -d firmware -e sentinel_s3` | SUCCESS, 116.29 s, sin errores/warnings; RAM77368/327680 bytes e imagen1183525/4194304 bytes. Configuración y fuente finales incluidos. |
| `python3 firmware/test/run_host.py` | PASS: comandos, gate/cache/digest, pendientes/cola llena, botón, framing, exclusión/fallos y ring reducido/recuperación/sobrescritura/gap. |
| `SENTINEL_SANITIZE=1 python3 firmware/test/run_host.py` | PASS con AddressSanitizer y UBSan. |
| `python3 firmware/test/check_contracts.py` | PASS: serializador C++ → codec AJV compartido; fixtures de command compartidos → parser cJSON. |
| `python3 firmware/test/check_config.py` | PASS: configuración generada efectiva, seguridad TLS/BLE, pines y particiones dentro de16 MiB; no es detección física. |
| `pio run --help`, `pio device monitor --help` | Sintaxis de build/upload/monitor documentada verificada. |
| Comprobación de enlaces locales de las dos guías | PASS. |
| Búsqueda de `/dev/ttyUSB*` y `/dev/ttyACM*` | Sin dispositivos. No se grabó ni monitorizó una placa. |

La primera instalación descargó herramientas oficiales en `/home/jefry/.platformio`, sin incorporarlas al repositorio. Se corrigieron diferencias reales de API IDF5.5.3 (lwIP IPv6, campo privado mbedTLS, inicialización store NimBLE y UTC sin timegm) y se activó explícitamente MBEDTLS_HAVE_TIME_DATE, ausente en el default. Los guardas de compilación rechazan SC/legacy/debug keys o TLS sin verificación de fecha/servidor.

## Tareas y límites

Comprobables sin placa: **3.1, 3.4, 3.5, 4.2, 4.3, 4.4**. Las casillas se actualizan individualmente bajo `/tmp/sentinel-core-tasks.lock` con fcntl.flock, verificando descripción exacta y reejecutando OpenSpec después de cada cambio.

Implementadas con verificación física pendiente: **3.2, 3.3, 3.6, 3.7, 4.1, 6.1, 6.2, 6.3, 6.4, 6.5, 6.6, 6.7, 6.8**. Los motivos y pasos concretos están por ID en [docs/firmware.md](../docs/firmware.md); el recorrido seguro está en [docs/connectivity.md](../docs/connectivity.md). No se buscaron teléfonos ni emuladores ni se ejecutó adb, por indicación del usuario; no se afirma pairing ni intercambio Android como probado.

NVS real, driver OLED, Wi-Fi, NimBLE y MQTT están conectados al supervisor y compilados. Los dobles de recursos/almacenamiento existen únicamente en tests de fallos host y no son implementaciones de producto. Falta probar memoria física, consola/controles, nivel BLE negociado, NVS después de reboot y conexión TLS/órdenes/replay/cambios de perfil desde el ESP32.

## API para integración

`include/sentinel.h`: composición `start`, envío acotado `submit(Channel,json,n,authorized,retained,session)`, solicitudes `post`, metadata protegida y adaptador BLE. Los callbacks de producción generan la autorización a partir del enlace/ACL, no de un parámetro recibido en JSON. `include/core.h`: tipos, CommandGate, PendingCommands, Runtime/Resources, Frame, Button y Journal/Store; `network.h`: gestor real de recursos. `wire.h` es el serializador público compartido por la ejecución y las comprobaciones host.

App/broker deben usar IDs/boot/revisión reales de metadata/state, UUIDs D-05, reloj y configuración solo por BLE protegido y una cuenta/CA/SAN LAN preparada para el device_id físico. Los endpoints localhost del laboratorio host no funcionan desde la placa. Reconciliar la salida de Cebo por BLE mediante revisión/last_command_id; no interpretar PUBACK o accepted como applied.
