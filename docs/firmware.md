# Firmware de la base Sentinel

Este firmware implementa la base `sentinel-core-foundation` en C++/ESP-IDF y FreeRTOS para ESP32-S3 N16R8. Los perfiles preparan recursos reales; captura, detectores, HTTP/Telnet, notificaciones y Sanitizador siguen sin implementar. Los contratos exactos están en [contracts/README.md](../contracts/README.md) y el flujo seguro en [connectivity.md](connectivity.md).

## Preparación y compilación

Desde la raíz del repositorio, usar Python 3, PlatformIO Core **6.2.0**, compilador C/C++ host y Node **24.13.1** para comprobar contratos. `firmware/requirements.txt` fija Core; se puede instalar en un virtualenv externo al repositorio. `platformio.ini` fija `espressif32@6.13.0`, ESP-IDF **5.5.3** y los paquetes resueltos de herramientas. No usa framework Arduino ni componentes externos administrados; los componentes LCD/NimBLE/MQTT/cJSON vienen del paquete ESP-IDF fijado.

```sh
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:/home/jefry/.platformio/penv/bin:$PATH pio run -d firmware -e sentinel_s3
python3 firmware/test/run_host.py
python3 firmware/test/check_config.py
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH python3 firmware/test/check_contracts.py
```

El runner host usa el cJSON de ESP-IDF instalado; si aún falta, obtiene cJSON v1.7.19 de su fuente oficial en `/tmp/sentinel-core-host`, fuera del repositorio. Necesita el runtime OpenSSL del sistema para SHA256; en placa se utiliza mbedTLS. No descarga una implementación alternativa de radio o almacenamiento. Los tests host inyectan fallos de recursos/almacenamiento para verificar la lógica real, sin sustituir los adaptadores de producción.

`firmware/sdkconfig.defaults` y la configuración efectiva `sdkconfig.sentinel_s3` conservan flash de 16 MiB, PSRAM octal a 80 MHz y memoria externa requerida. El arranque comprueba `esp_flash_get_size` y `esp_psram_get_size`: una discrepancia impide activar perfiles de red y deja error. El arranque del propio IDF también puede detenerse ante PSRAM ausente. El nombre genérico N8 que imprime PlatformIO describe su perfil de catálogo; los overrides, las particiones y el diagnóstico real determinan la configuración N16R8. La aplicación factory ocupa hasta 4 MiB; ese límite de imagen no es la flash física.

## Cableado y puerto

Valores propuestos, pendientes de comprobar en la unidad y su [pinout](architecture.md#pinout-aportado-por-el-usuario):

| Elemento | Conexión inicial |
| --- | --- |
| SSD1306 128×32 | I2C, dirección 0x3C, SDA GPIO8, SCL GPIO9, alimentación compatible de 3V3 y GND común |
| Botón externo | GPIO4 a GND, pull-up interno; no es BOOT |
| Consola | UART0 a 115200 mediante USB/UART de la placa |

Configurar `SENTINEL_SDA`, `SENTINEL_SCL`, `SENTINEL_OLED_ADDRESS` y `SENTINEL_BUTTON` mediante `pio run -d firmware -e sentinel_s3 -t menuconfig`. Se rechazan pines de memoria octal GPIO35–37 y los pines USB/UART19/20/43/44 para estos periféricos. Confirmar disponibilidad eléctrica antes de conectar.

Detectar el puerto real con `pio device list`; `/dev/ttyACM0` en los ejemplos siguientes es un marcador, no un puerto detectado. En WSL debe adjuntarse explícitamente el USB si no se ve; alternativamente usar PlatformIO desde Windows con el COM real. No se ejecutó grabación ni monitor sin placa.

```sh
PATH=/home/jefry/.platformio/penv/bin:$PATH pio run -d firmware -e sentinel_s3 -t upload --upload-port /dev/ttyACM0
PATH=/home/jefry/.platformio/penv/bin:$PATH pio device monitor -d firmware -e sentinel_s3 --port /dev/ttyACM0 --baud 115200
```

Los flags se verificaron con `pio run --help` y `pio device monitor --help`. En consola se imprimen versión, device ID persistente, boot ID nuevo, flash y PSRAM detectadas, perfil idle y revisión0. No se imprimen credenciales ni passkey. Es necesario registrar los tamaños **medidos** de la unidad al grabar; ninguna cifra de compilación reemplaza esa lectura.

## Supervisor y extensión

`components/sentinel/runtime.cpp` compone identidad, NVS, radio, transportes y periféricos. Un único supervisor ejecuta comandos y transiciones. Hay ocho slots privados y cola de ocho solicitudes, caché de 32 resultados sanitizados, cola separada de rechazos busy y salida BLE acotada. El pool se libera borrando el cuerpo privado tras procesarlo. Los callbacks depositan solicitudes; STA/MQTT se activan y detienen desde el supervisor.

Orden de transición: validar autorización/boot/vigencia/duplicado/revisión, accepted, detener red anterior, comprobar liberación, preparar perfil nuevo, incrementar revisión y emitir applied/rejected. Si falla la liberación no se inicia otro perfil y queda fase error; `set_mode idle` permite intentar liberar otra vez. `ready` indica recursos preparados, aunque STA/MQTT aún estén desconectados.

- `idle`: sin STA/MQTT; BLE seguro y controles locales disponibles.
- `escudo`: BLE de control, Wi-Fi detenido/en modo NULL, sin asociación, escaneo activo, captura ni MQTT.
- `cebo`: STA autorizada y esp-mqtt5 con reintentos asíncronos; BLE permanece disponible. Sin configurar todavía, el perfil puede estar preparado y desconectado.

`Resources` y `Runtime::transition` en `include/core.h` son el punto de integración para inicio/parada y exclusión de futuros módulos; los adaptadores reales son `Network`, GATT y periféricos. Las capacidades futuras se publican en false. Ningún módulo de seguridad genera incidentes ficticios.

El botón tiene debounce50 ms. Al liberar una pulsación corta alterna Escudo/Cebo (desde idle selecciona Escudo). Mantener3 s abre120 s de emparejamiento; al liberar no se ejecuta también la acción corta. SSD1306 utiliza `esp_lcd` e I2C de ESP-IDF, sin LVGL, y muestra perfil, fase, conectividad y error o passkey. OLED ausente/fallido se diagnostica y deshabilita nuevos pairings; los bonds ya autorizados y controles seguros se conservan.

## Registro circular

`partitions.csv` reserva NVS general de24 KiB en0x9000, PHY de4 KiB, factory de4 MiB en0x10000 y `event_log` NVS de256 KiB en0x410000. El ring tiene **64 registros de hasta2048 bytes** cada uno. El espacio restante de flash no se anuncia como historial ni OTA.

Cada slot `slot00`…`slot63` guarda el sobre de un evento completo con ID estable y cursor persistente `journal_seq`. Los eventos de base son `system.booted`, `system.profile_changed` y `system.error` con código enumerado; no admiten cuerpos privados. Una escritura solo se considera persistida tras `nvs_set_blob` y `nvs_commit` correctos. Al arrancar se examinan los slots válidos y se reconstruye el cursor máximo sin depender de un índice auxiliar. NVS aporta verificación de integridad de sus blobs.

La sobrescritura conserva los últimos64. `read_events(after_cursor,limit)` admite1…8, emite eventos individuales y después el ack de página con `next_cursor`, rango retenido, `more` y `gap`; nunca agrega ocho mensajes grandes en uno. Un cursor antiguo o hueco recuperado produce gap y permite leer los registros válidos. Ante error de lectura/escritura se expone degradación/resource_error y se mantienen los controles; no se borra automáticamente ninguna partición.

Al reconectar MQTT se recorre el ring con los mismos IDs, sin eliminar por PUBACK. Un duplicado de consulta vigente puede reemitir la página conocida; si la sesión se interrumpe, consultar otra vez con un command ID nuevo y el cursor confirmado por la app. El cursor de la app avanza tras la página completa. Un ack MQTT no acredita recepción por la app.

La capacidad es acotada, hay desgaste y puede perderse la escritura interrumpida o un registro dañado. No se garantiza pérdida cero ante todo corte eléctrico ni retención ilimitada. Ante NVS incompatible o llena se informa error: cualquier recuperación destructiva necesita una decisión explícita del operador, fuera del arranque automático.

## Evidencia y tareas

Entorno: WSL/Linux, PlatformIO Core6.2.0, Node24.13.1 y toolchain fijado. No se detectaron `/dev/ttyUSB*` ni `/dev/ttyACM*`; por indicación del usuario no se buscó ni utilizó Android. Por tanto no se han medido memoria de placa, OLED, bonding, radio, NVS física ni integración ESP32/broker.

Pruebas ejecutables: `run_host.py` cubre fixtures de comandos, versión, expiración, boot incorrecto, revisión, retained, duplicado/ID conflictivo, digest canónico, pool lleno, funciones reservadas, exclusión/transición fallida, botón corto/largo, framing MTU23/2048/timeout/desconexión, ring reducido, recuperación, sobrescritura/gap, fallo de escritura y repetición paginada. `check_contracts.py` valida salidas del serializador C++ con AJV compartido y los fixtures de comandos compartidos con cJSON. Resultado final: PASS en ambas comprobaciones, también PASS con `SENTINEL_SANITIZE=1` (AddressSanitizer/UBSan), y PASS en `check_config.py`.

Trazabilidad de pendientes físicos:

| Tarea | Implementación y comprobación restante |
| --- | --- |
| 3.2 | Overrides/particiones y detección implementados; falta lectura real de16 MiB/8 MiB al grabar. |
| 3.3 | Identidad/boot/idle seguro implementados; falta consola real y persistencia de identidad tras reboot. |
| 3.6 | Driver SSD1306 y botón integrados; faltan OLED, acción corta/larga y fallo de pantalla reales. |
| 3.7 | Cableado/comandos documentados y sintaxis comprobada; upload/monitor y medición física pendientes. |
| 4.1 | NVS real conectado al ring; falta evento persistido y lectura después de reiniciar placa. |
| 6.1 | Inicialización y leases Wi-Fi/NimBLE implementados; falta anuncio/arranque y parada en placa. |
| 6.2 | Ventana física, SC-only/MITM/passkey/bonds/CCCD protegidos implementados; falta pairing y nivel negociado con hardware. |
| 6.3 | GATT y framing conectados al gestor; falta intercambio de metadata/get_state/indications en BLE real. |
| 6.4 | Configuración/UTC protegidos y RAM implementados; falta aplicar datos de laboratorio por BLE real. |
| 6.5 | STA/esp-mqtt5/TLS real implementados; falta conexión desde placa y rechazo de CA/hostname/fecha incorrectos. |
| 6.6 | Topics/QoS/LWT/periódico/replay implementados; falta orden real MQTT/duplicado/retained desde placa. |
| 6.7 | Cierre STA/MQTT y reconciliación implementados; falta recorrido Cebo–Escudo físico sin asociación residual. |
| 6.8 | Flujo documentado en connectivity.md; falta ejecutar esos pasos en placa. |

Las casillas que requieren estas comprobaciones permanecen abiertas. La compilación y los tests host acreditan solo lo que ejecutan.


Compilación final del 2026-11-09 UTC (2026-11-08 en El Salvador): `pio run -d firmware -e sentinel_s3` **SUCCESS**, 116.29 s, sin errores ni warnings. RAM estática **77368/327680 bytes** (23.6 %); imagen **1183525/4194304 bytes** (28.2 % del límite factory). Son cifras de enlace, no heap libre observado, flash física detectada ni medición de PSRAM. Configuración efectiva verificada: SC-only=1, nivel4, bonds4, legacy/debug keys deshabilitados, validación de fecha TLS habilitada, flash16 MiB y PSRAM octal80 MHz. Informe de entrega: [firmware/EVIDENCE.md](../firmware/EVIDENCE.md).
