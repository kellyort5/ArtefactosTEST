# Verificación: dependencias y adaptadores Arduino oficiales

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

## Estado

El usuario aprobó expresamente **core oficial, tres teléfonos y configuración mínima**. Se integraron los adaptadores y se instalaron únicamente las dos bibliotecas nuevas y las nueve claves autorizadas del menú Sentinel. No se reconstruyó un SDK, no se modificaron código/binarios del core y no se borraron instalaciones anteriores ni datos. No se hicieron commits, publicaciones ni operaciones de ramas; `tasks.md` es del coordinador.

**Fuente final: compilación/enlace Windows oficial PASS, exit0**. La corrección de CA configurada vencida pasó17 casos nativos ASan/UBSan y se incluyó en ese ELF final. Se retiró `firmware/arduino-package`; se notificó al coordinador y al autor de limpieza que sus seis herramientas SDK/Python pueden retirarse. La instalación global anterior permanece intacta.

Se leyeron instrucciones, brief, arquitectura, decisiones, roadmap, encargos, specs funcionales y todos los `contextFiles` de `openspec-apply-change`. El cambio estaba `ready`. Se mantuvieron Escudo, contratos, API de módulos/factories, esquema de datos, particiones y propiedad de archivos.

## Matriz final de componentes

| Componente | Versión/ruta | Resultado y adaptación |
| --- | --- | --- |
| Arduino IDE Windows / CLI incluido | IDE2.3.10, CLI1.5.1 `01f3d4f2b` | Ejecutable instalado del IDE; ningún framework adicional. |
| ESP32S3 Dev Module oficial | `esp32:esp32:esp32s3`, core3.3.12, IDF5.5.5 | Flash16M, QIO80MHz, PSRAM OPI y `PartitionScheme=sentinel`. Se conservan core/SDK/compilador oficiales. |
| Wi-Fi, captura, NAPT, OLED, FreeRTOS | componentes del core/SDK oficial | Adaptadores existentes; no otro driver, host de radio ni journal. NAPT/forwarding presentes. |
| NimBLE C del SDK | SC-capable, persistencia y capacidad3 | SC-only/nivel4/MITM se fijan por runtime. Rechazo explícito de segunda conexión y acceso solo del handle activo. Preflight de bonds de solo lectura antes de iniciar el host. |
| wolfMQTT | 2.2.0, fuentes del proveedor en formato Arduino | `WOLFMQTT_V5`, transporte externo cifrado obligatorio. Una tarea realiza I/O y ACK; outbox14 elementos de tamaño acotado, menor de32KiB. No cliente MQTT propio ni wolfSSL. |
| TLS nativo | NetworkClientSecure3.3.12, Mbed TLS3.6.6 | CA y nombre DNS/IP con verificación requerida, peer chain retenida. TCP/TLS con timeout1s; DNS usa API asíncrona lwIP con espera1s y hasta2 peticiones pendientes. |
| Validador adicional mantenido | motor BearSSL de ESP_SSLClient3.1.3 | Antes de cualquier byte MQTT, comprueba CA configurada y cadena DER contra una ruta válida y tiempo obligatorio. Sin comparación criptográfica/fechas propia, fingerprints o `setInsecure`. |
| Datos y preparación de carga | nueve claves de opción local + wrapper NVS | Sin `boot_app0`, `erase-all` ni exportación de flash completa. Particiones sin cambios. Error NVS real expuesto al runtime, sin borrado ni reintento automático. |

El MQTT nativo del SDK no tiene MQTT5 y sus TLS no comprueban fechas. No se borraron esas guardas para seguir usando capacidades ausentes: se sustituyó el cliente por fuentes mantenidas y se exige el segundo motor de validación en cada conexión. El wrapper `esp-mqtt-arduino`1.0.1 se descartó porque su propia documentación exige reconstruir el SDK.

## Fuentes primarias

- [Core Arduino3.3.12: initArduino/NVS](https://github.com/espressif/arduino-esp32/blob/3.3.12/cores/esp32/esp32-hal-misc.c), [recetas oficiales](https://github.com/espressif/arduino-esp32/blob/3.3.12/platform.txt) y [cliente TLS nativo](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/NetworkClientSecure/src/ssl_client.cpp).
- [NimBLE del IDF5.5.5: comprobaciones SC por runtime](https://github.com/espressif/esp-nimble/blob/685675c0128deafdd201c9eb82e61d227364646c/nimble/host/src/ble_sm.c) y [store NVS](https://github.com/espressif/esp-nimble/blob/685675c0128deafdd201c9eb82e61d227364646c/nimble/host/store/config/src/ble_store_nvs.c).
- [Mbed TLS del SDK: SAN IP](https://github.com/espressif/mbedtls/blob/9d669eadb1955d348986b9280156710aaaadf79f/library/x509_crt.c), [validación X.509 BearSSL](https://bearssl.org/x509.html) y [release ESP_SSLClient3.1.3](https://github.com/mobizt/ESP_SSLClient/releases/tag/v3.1.3).
- [wolfMQTT2.2.0: distribución Arduino del proveedor](https://github.com/wolfSSL/wolfMQTT/tree/v2.2.0/IDE/ARDUINO) y [limitación del wrapper esp-mqtt-arduino](https://github.com/junzzhu/esp-mqtt-arduino#enabling-mqtt-v5-in-arduino-core).
- [Especificación Arduino: opciones y boards.local.txt](https://docs.arduino.cc/arduino-cli/platform-specification/).

## Implementación y límites

- `network.cpp` conserva API, radio y netifs, y delega MQTT en `mqtt_transport.cpp`; sus headers usan includes relativos normales.
- El worker preserva CONNECT limpio/expiry0, suscripción QoS1/retain-as-published/retain-handling0, LWT offline, disconnectreason4, payload/identificadores y reentrega. Mensajes parciales se borran al cerrar; solo se envían órdenes por el validador central con RETAIN conservado.
- `request_stop()` no espera I/O. `Network::network_stopping()` señala únicamente cierre real del worker. El núcleo, modificado y probado por el coordinador, mantiene lease y transición hasta ACK o el deadline existente5000ms; otros fallos siguen siendo errores.
- `configure()` responde `busy` **antes de cambiar red/propietario/credenciales** si hay worker activo. El usuario puede pasar a Idle, esperar confirmación y reconfigurar; no queda un cierre parcial tras una confirmación falsa.
- DNS mantiene contextos hasta que lwIP devuelve el callback y descarta resultados de generaciones anteriores. No implementa otro resolvedor ni usa un `WiFi.begin()` paralelo.
- BLE conserva ventana física, passkey/OLED, persistencia, producción SC/MITM y clave16bytes. Los defaults SDK SM_LVL0/SC_ONLY0 no impiden configurar esos campos por runtime: fuente y código máquina del `libbt.a` oficial contienen su comprobación.

### Certificados e IP SAN

La primera prueba encontró que BearSSL directo rechaza un certificado válido con SAN iPAddress al comparar ese IP (error56). Se conservó el soporte IPv4/IPv6 usando el verificador nativo de nombre/CA. `NetworkClientSecure::connect(ip,port,host,ca,...)` recibe la identidad original sin otro DNS bloqueante; la versión instalada usa `VERIFY_REQUIRED` y `mbedtls_ssl_set_hostname`. La fuente Mbed TLS3.6.6 vinculada a IDF5.5.5 contiene la comprobación de SAN IP. La segunda verificación solo omite comparar nombre porque ya lo exigió el verificador nativo.

Una regresión adicional mostró que un trust-anchor BearSSL no conserva fechas de su certificado CA. El caso **CA configurada vencida** fallaba antes: había bytes MQTT. La corrección comprueba con el mismo motor mantenido el certificado CA antes de usar su anchor, y luego la cadena peer por esa ruta; itera las CA del bundle para conservar una ruta válida aunque haya otra CA vencida ajena. El caso pasó después con **cero bytes MQTT/autenticación**, junto con CA futura y CA válida con keyCertSign. No se envían credenciales antes de ambas verificaciones ni se reutiliza una conexión sin comprobarla.

### Preservación de NVS y bonds

`partitions.csv` mantiene SHA256 **9cc33b3b11c342f15a9b0f92862212769619a779fe4578b1e65717082c36d3af**, idéntico al snapshot protegido. NVS0x9000/0x6000, factory0x10000/4MiB y event_log0x410000/256KiB. No se abrió un COM ni se ejecutó borrado/carga.

`arduino_environment.cpp` guarda la primera respuesta real de `nvs_flash_init`. Solo traduce NO_FREE_PAGES/NEW_VERSION_FOUND a ESP_FAIL para impedir que `initArduino()` entre en su rama de borrado; `sentinel_arduino_nvs_status()` entrega el error original y el runtime aborta identidad antes de BLE/modos. No informa NVS sana ante un error.

El store oficial restaura índices1..3 y no detecta por sí solo un cuarto. `ble_storage_preflight()` enumera todo `nimble_bond` en READONLY antes de `nimble_port_init`; capacidad excedida, claves/formato desconocidos, tamaños distintos, material sin SC/MITM/16bytes o errores de lectura detienen BLE sin escritura/evicción. La consola emite diagnóstico constante y el estado contractual sigue `stopped`. Con cuatro vínculos previos se preservan todos y se exige recuperación explícita, sin truncar ni volver a emparejar sobre esos datos.

Se compararon los tokens de campos de `ble_store_value_sec/cccd/csfc/local_irk/rpa_rec` entre ambos SDK instalados: idénticos; el preflight también exige sus tamaños reales del target. Evidencia `/tmp/sentinel-official-storage-compatibility.json`. Esto no acredita recuperación física de flash.

## Instalación y recetas reales

Versiones, proveedores y SHA256: `firmware/arduino/dependencies.json`. Configuración distribuible pequeña: `firmware/arduino/boards.local.txt` y los tres archivos de formato/opciones Arduino para wolfMQTT. Instrucciones Windows y recuperación: `firmware/SentinelNode/README.md`.

Se añadieron solo las nueve claves `esp32s3.menu.PartitionScheme.sentinel*` junto al boards.txt oficial3.3.12. Antes no existía boards.local.txt; estado previo y contenido registrado en `/tmp/sentinel-official-boards-local-before.json`. Se preservan otras opciones y archivos del core. La selección exige `ARDUINO_PARTITION_sentinel` y añade el wrapper de enlace. El costo por equipo es instalar esas dependencias/opción una vez y revisar/reaplicar al actualizar core; cada Verify/Upload usa el IDE normalmente, **sin generadores/Python propios por build**. Sí existen las recetas mínimas de datos aprobadas y las herramientas internas oficiales del IDE.

La ejecución real del build2 crea flash_args con solo0x0/0x8000/0x10000, omite boot_app0 y suprime merged.bin. El build stock del primer prototipo generó una imagen de16MiB que rellenaría NVS/journal: nunca se cargó y no es una exportación admitida. No se alteró la tabla para adaptarla a esa receta peligrosa.

## Pruebas ejecutadas

1. Prototipo mínimo Windows oficial: **PASS exit0**,443687bytes flash/26892globalRAM;684depfiles,682 con SDK oficial y cero `hardware/sentinel`. Log `/tmp/sentinel-official-prototype-build.log`, resumen `/tmp/sentinel-official-prototype-dependencies.json`. No es el firmware completo.
2. Preflight BLE: **11 casos ASan/UBSan PASS**, incluidos tres bonds seguros, cuarto previo, layout, legacy, autenticación, clave corta, formato desconocido y errores. Todos los bytes simulados permanecen intactos; los mocks no ofrecen API de escritura. Log `/tmp/sentinel-transport-tests/ble-storage-results.log`; el autor de limpieza incorporó esta prueba al flujo del repositorio.
3. Worker actual y `tls_verification.cpp`: **17 casos ASan/UBSan PASS** con fuentes wolfMQTT y motor BearSSL originales. Válido DNS/IPv4/IPv6 con cadena EC completa; leaf/intermedio/CA vencidos y futuros; nombre/CA erróneos; cadena ausente; CA keyCertSign y bundle con ruta válida; reloj obligatorio; cancelación pendiente/ACK; RETAIN; QoS1 pendiente reentregado con el mismo payload/identificador. Log `/tmp/sentinel-transport-tests/worker-final-results.log`. La regresión antes de corregir CA está en `worker-ca-gap.log`; handoff reproducible a limpieza en `HANDOFF.md` del mismo directorio.
4. Decodificador independiente `mqtt-packet`9.0.2 ya instalado, Node24.13.1: paquetes CONNECT/SUBSCRIBE/PUBLISH/DISCONNECT y flujos del worker verifican versión5, expiry0, QoS1, LWT retenido, RAP/rh0, reason4 y payload idéntico tras reconexión. Artefactos `.hex` en `/tmp/sentinel-transport-tests`.
5. Wrapper NVS: OK, NO_FREE_PAGES, NEW_VERSION_FOUND y error genérico preservan resultado exacto, una sola llamada y ningún reintento: **PASS**, `/tmp/sentinel-official-prototype/nvs-results.log`.
6. Suite núcleo/69fixtures/Escudo y nueva espera de cierre: el coordinador/autor de limpieza reportaron **PASS ASan/UBSan** y ningún caso anterior eliminado; evidencia de ellos, no reetiquetada como prueba física de transportes.

Los tests de worker usan dobles declarados de NativeTLS, DNS y FreeRTOS: el doble TLS usa OpenSSL para CA+SAN **sin fechas**; solo el motor BearSSL y la política de producción pueden rechazar fechas. Son pruebas de política/colas/protocolo, no de TCP/TLS sobre ESP32, scheduler real ni radio. Ningún resultado simulado se presenta como hardware real.

## Builds completos Windows

FQBN: `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`. Ejecutable nativo del IDE en `%LOCALAPPDATA%/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe`. Directorio de build/caché: `%TEMP%/sentinel-official-firmware`.

- Intento1: error concreto de extensión `.inc` omitida por la copia Arduino. Su autor renombró el validador a `.h` con bytes idénticos y corrigió el include; sin hooks/generación adicional. Log `/tmp/sentinel-official-firmware-build.log`.
- Build2: **PASS exit0**, ELF completo `%TEMP%/sentinel-official-firmware/SentinelNode.ino.elf`,1376490bytes flash/160228globalRAM. Log `/tmp/sentinel-official-firmware-build-2.log`; entradas congeladas `/tmp/sentinel-official-firmware-build-2-inputs.json`. Conserva dependencia/receta oficiales, pero precede la corrección CA y no acredita la fuente final.
- **Build final con corrección CA: PASS exit0**, incremental en el mismo directorio/caché y FQBN. Flash1376726bytes de4194304, globalRAM160228bytes de327680; no es medición de memoria libre bajo carga. Log `/tmp/sentinel-official-firmware-final-build.log`. ELF SHA256 **60d6e73cc23ac17520f0652d9995ea8dc20e63bd89ab2d4bd9c4f3dc5c2eb6ff**, BIN SHA256 **b0b1d8c355a219a36c5b5f533420d97a94406779d9bc39ed25dd4d9ee6591517**. La fuente TLS final SHA256 es **315cc7bab2572abd074984766655ec50d4b889d978c92b6c904441323f20db8e**.
- **428 entradas** (sketch, soporte, dos bibliotecas y boards.local) conservaron su SHA antes/después. Registro `/tmp/sentinel-official-firmware-final-inputs.json`; resumen `/tmp/sentinel-official-firmware-final-evidence.json`.
- **730 depfiles**,726 con rutas Arduino15/ESP32 oficiales y **cero** `hardware/sentinel`. El log identifica NetworkClientSecure/Networking3.3.12, ESP_SSLClient3.1.3 y wolfMQTT2.2.0, con FQBN oficial. No se usó el SDK Sentinel instalado.
- Se decodificó la tabla BIN resultante y coincidieron todos los tipos/offsets/tamaños con la tabla original. `flash_args` solo contiene0x0/0x8000/0x10000; no existe merged.bin. Propiedades efectivas en `/tmp/sentinel-official-final-board-properties.log` y nueve claves instaladas idénticas a la fuente de soporte.
- El objdump del ELF final muestra las llamadas de `initArduino` a `__wrap_nvs_flash_init`, en `/tmp/sentinel-official-initArduino-disassembly.log`; las pruebas de fallo inyectado verifican la traducción no destructiva y el error real del runtime.

Cada build respondió a un fallo o cambio real; no hubo campañas de benchmarks ni compilaciones repetidas por rutina. Después del snapshot final solo se corrigió el prerrequisito documental del README: los runners usan cc/c++/OpenSSL/tar/unzip/Node, sin CMake. Ese cambio textual no modifica fuentes compiladas ni requiere otro build.

## Retirada y pendientes físicos

Retirado exclusivamente el árbol fuente obsoleto `firmware/arduino-package`, tras comprobar la independencia de la ruta oficial. El coordinador y limpieza recibieron la señal; limpieza informó su retirada y cierre bajo su propia evidencia. Se conservó `Documents/Arduino/hardware/sentinel/esp32` y las instalaciones ajenas. Ninguna fuente/artefacto del firmware cambió por esa retirada: el paquete no era una entrada del build final.

Físicos no realizados: identificación/selección de puerto y carga, arranque/memorias/pines, pairing Android/SC, radio, MQTT/TLS en ESP32, OLED, botón y persistencia/reinicio. Solo constan COM Bluetooth3/4; no se inventó una placa. Esos pendientes no bloquean evidenciar software y no se declaran aprobados por un build.
