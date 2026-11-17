# Verificación de sentinel-firmware-cleanup

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha de ejecución: 2026-11-13, Linux/WSL, Node 24.13.1 ya instalado. Rama compartida `feat/sentinel-arduino-migration`; sin commits, push, merge ni archivo de cambios. Este informe acredita herramientas/pruebas host y recoge la evidencia del build oficial entregada por dependencias; no acredita radio, BLE, memorias físicas ni subida de firmware. Estado: limpieza, pruebas y relevo de dependencias completados; comprobaciones físicas pendientes explícitas.

## Validador distribuible

`firmware/SentinelNode/src/sentinel/v2_validator.h` es la única salida distribuible de los schemas existentes en `contracts/schemas/v2`. `protocol_v2.cpp` lo incluye localmente. No se requiere generación durante el build Arduino.

El primer build completo oficial detectó que Arduino CLI no copiaba la extensión `.inc` del árbol `src/`. Tras liberación explícita del freeze, el coordinador autorizó renombrar la fuente a `.h` y cambiar únicamente su include en `protocol_v2.cpp`. Se mantuvieron exactamente los bytes y SHA256; no se regeneró la fuente distribuida ni se modificaron los contratos. Generador de mantenimiento, pruebas y whitelist específica de `.gitignore` apuntan ahora al `.h`.

Se ejecutó una última generación con la herramienta Python anterior hacia `/tmp/sentinel-validator-python.inc`, se generó con Node y se comparó mediante `cmp`: igualdad byte a byte, 193245 bytes, SHA256 **9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68** en ambas salidas. La referencia queda fuera del repositorio; no es un segundo contrato ni una fuente editable adicional.

Mantenimiento, fuera del build Arduino:

```sh
node firmware/tools/generate_validator.mjs --check
node firmware/test/validator_test.mjs
# Solo cuando cambien los contratos: regenerar, revisar el diff y ejecutar suites.
node firmware/tools/generate_validator.mjs
```

La prueba de mantenimiento conserva equivalencia con la fuente distribuida, regeneración con rutas que contienen espacios y cambios de schemas, detección de fuente desactualizada, rechazo de keywords/patrones no soportados y entradas ausentes. Una generación fallida no sobreescribe la fuente revisada. `.gitignore` permite específicamente esa fuente y sigue excluyendo otras salidas generadas, cachés y binarios.

## Pruebas preservadas

Los nueve archivos C++/hpp de pruebas originales (2386 líneas) y `SentinelNode/partitions.csv` conservan sus bytes según `/tmp/sentinel-simplification-before.json`. `check_migration.mjs` comprueba ese inventario y la única entrada `SentinelNode.ino`; los refactors funcionales autorizados se comprueban por suites, no se declaran idénticos por hash.

El coordinador añadió posteriormente `firmware/test/network_stop_test.cpp` como regresión independiente de la parada MQTT pendiente. El runner lo registra en `buildHost('core')` y lo enlaza con las fuentes/flags/cJSON del núcleo, con su propio `main` y sin `core_test.cpp`, `module_consumer.cpp` ni `module_test.cpp`. El archivo y sus aserciones son del coordinador y no se editaron en limpieza.

| Suite | Evidencia ejecutada |
| --- | --- |
| Núcleo y módulos | PASS: comandos, framing, botón, supervisor, gates/dispatch, leases exclusivos, parada fallida, outputs, journal/overwrite/fallos. |
| Parada de red pendiente | PASS con ASan/UBSan: espera hasta ACK/liberación, órdenes concurrentes busy, timeout de 5000 ms sin ampliación, ACK tardío sin aplicación, errores y liberación inconsistente, reintento a Idle y compatibilidad del adaptador anterior. Caso independiente ejecutado dentro de `test:firmware`. |
| Preflight NVS/bonds | PASS con ASan/UBSan: 11 casos del fixture entregado (vacío, tres bonds seguros, cuarto previo, layout erróneo, no autenticado, legacy, clave corta, nombre malformado, formato desconocido, namespace nuevo y error de lectura). Todos mantienen bytes almacenados y balance apertura/cierre. `ble_storage_test.cpp` y sus tres stubs se copiaron sin cambios del fixture del responsable; se compila el helper de producción real con stubs host, sin API de escritura/borrado. |
| Contratos núcleo | PASS: serialización v1/v2 → codec compartido AJV, comandos v1 → parser nativo, 69 fixtures v2 → AJV y validación nativa con iguales resultados. |
| Descubrimiento | PASS: catálogo/SSID/dedup/overflow/cursors, hooks reales de módulo, gate/ACK, selección, errores de escaneo, backpressure e Idle/reinicio. |
| Captura | PASS: epochs/límites de callback, parser management/IE/RSN, scheduler, órdenes BLE, métricas/cobertura, reescaneo y aislamiento tras parada fallida. |
| Detectores | PASS: los cuatro detectores, umbrales, expiración, cooldown, reintentos/epochs, duplicados benignos, seguridad/canal/RSSI, correlación Karma pasiva y casos inconclusos. |
| Recuperación | PASS: fases write/recover/checks en procesos separados; identidad entre arranques, 6 únicos/6 repetidos, páginas completas y gap visible. Simulación de Store host; no evidencia NVS/BLE física. |
| Codec Escudo | PASS: 70/72/30/18/7 mensajes por escenario, correlación ACK/órdenes, páginas y 19 solicitudes de documentación válidas por escenario. |
| Fronteras de módulos | PASS: cinco combinaciones de flags, símbolos de factories desactivadas ausentes, link fallido si falta una factory activada, rechazo de flag=2. |
| Configuración oficial | PASS sobre `esp32s3-libs/3.3.12/qio_opi/include/sdkconfig.h` instalado: memoria/SC/persistencia de 3 bonds/TLS nativo con cadena retenida/NAPT; defaults GPIO/módulos; límites y partición event_log de 256 KiB dentro de 16 MiB. No equivale a detección física. |
| Guardas oficiales | PASS del preprocesador sobre `arduino_requirements.cpp`: rechazo por ausencia de cada requisito, core distinto de 3.3.12, menú de particiones ausente, capacidad de bonds no verificada, claves debug y TLS inseguro. Defaults compilados SC-only=0/nivel=0/legacy admitidos como exige el core oficial; la seguridad efectiva sigue siendo responsabilidad del adaptador runtime. |
| Guardas de transporte | PASS del preprocesador sobre `transport_requirements.h`: MQTT5 obligatorio, TLS externo obligatorio, ESP_SSLClient 3.1.3 exacto y X509 completo; rechazo de versiones distintas, macros ausentes, ENABLE_MQTT_TLS, SSLCLIENT_INSECURE_ONLY y BEARSSL_SSL_BASIC, incluso cuando las opciones prohibidas se definen a cero. No acredita conexión MQTT/TLS física. |

Comandos estables desde la raíz; Node 24 en PATH, compilador C/C++, `nm` y runtime OpenSSL del host. cJSON 1.7.19 se reutiliza de caché local, o se recuperan solo sus dos fuentes con SHA256 fijados; no se instala PlatformIO, ESP-IDF ni otro framework para estas pruebas.

```sh
SENTINEL_SANITIZE=1 npm run test:firmware
SENTINEL_SANITIZE=1 npm run test:escudo
node firmware/test/arduino_build_test.mjs
node firmware/test/check_migration.mjs --baseline /tmp/sentinel-simplification-before.json
node firmware/test/check_config.mjs --sdkconfig <header-efectivo>
```

Los runners `run_host.mjs` de núcleo y Escudo permiten ejecutar solo C++; los `check_contracts.mjs` compilan y ejecutan la suite pertinente más su contraste con los codecs compartidos. Cada ejecución usa una carpeta temporal única y la elimina al salir, evitando que agentes simultáneos sobreescriban objetos/binarios; se comprobó además esta variante con el núcleo y ASan/UBSan. `SENTINEL_SANITIZE=1` habilita ASan/UBSan y detiene errores UBSan; los fixtures inválidos deben devolver el código de rechazo explícito, no un crash. Los tests host son de mantenimiento y no son requisito para compilar/subir desde Arduino IDE.

Logs locales: `/tmp/sentinel-cleanup-core.log`, `/tmp/sentinel-cleanup-escudo.log`, `/tmp/sentinel-cleanup-build-boundaries.log`, `/tmp/sentinel-cleanup-migration.log`, `/tmp/sentinel-cleanup-isolated-host.log`; ampliación del núcleo con parada pendiente en `/tmp/sentinel-cleanup-network-stop-core.log`. El snapshot `/tmp/sentinel-simplification-before.zip` y su mapa permanecen fuera del repo e intactos.

Tras renombrar el validador a `.h` e integrar las guardas y el fixture NVS, ejecución final `SENTINEL_SANITIZE=1 npm run test:firmware` y `SENTINEL_SANITIZE=1 npm run test:escudo`: **PASS, exit 0**, incluidos 69 fixtures v2 AJV/nativo, las cuatro detecciones y recuperación 6 únicos/6 repetidos. Logs finales de este relevo: `/tmp/sentinel-cleanup-official-core.log`, `/tmp/sentinel-cleanup-official-escudo.log`, `/tmp/sentinel-cleanup-official-guards.log`, `/tmp/sentinel-cleanup-official-config.log`. La comparación de los nueve archivos C++/hpp originales y las particiones contra el snapshot vuelve a pasar. Durante el freeze posterior solo finalizaron suites; no se editaron fuentes firmware.

## Prueba opcional reproducible de transportes y CA

Por ampliación expresa del coordinador, se preservó el harness focalizado entregado por dependencias como `firmware/test/transports/` y un runner Node pequeño `firmware/test/check_transports.mjs`. El primer intento del runner detectó una ruta de include temporal ausente, corregida en el runner; la ejecución completa posterior terminó **PASS, exit 0**. Log: `/tmp/sentinel-cleanup-transports.log`.

```sh
SENTINEL_SANITIZE=1 node firmware/test/check_transports.mjs --archives /ruta/a/archivos-upstream-en-cache
```

La carpeta indicada contiene `wolfMQTT.tar.gz` y `ESP_SSLClient.zip` de las versiones 2.2.0/3.1.3. Cada ejecución comprueba sus SHA256 contra `firmware/arduino/dependencies.json` antes de extraerlos fuera de Git; no descarga dependencias. En este entorno se reutilizó `/mnt/c/Users/Jefry/AppData/Local/Temp/sentinel-official-prototype`, sin modificar instalaciones ni configuración global. El comando es opcional para mantenimiento host, con Node24, compiladores C/C++, OpenSSL y sus headers, `tar`/`unzip`, y `mqtt-packet`9.0.2 ya instalado en el repo; no agrega dependencias npm ni es requisito de Verify/Upload en Arduino IDE.

En cada ejecución se copian **las fuentes activas** `mqtt_transport.cpp`, `tls_verification.cpp` y sus tres headers a un temporal; no hay copia de producción editable en tests. wolfMQTT original y el worker se instrumentan con ASan/UBSan cuando `SENTINEL_SANITIZE=1`; el motor original BearSSL se compila sin modificaciones ni instrumentación, siguiendo la receta del responsable. Las colas/tareas, DNS y socket TLS nativo son dobles declarados: OpenSSL valida CA y DNS/IP con fechas deliberadamente desactivadas, reproduciendo el límite SDK; la validación de fechas/cadena la realiza el BearSSL mantenido mediante el helper real de producción. Esto no acredita sockets, TLS, RTOS ni radio físicos.

Se preservan **17 casos**: leaf vencido/futuro, DNS incorrecto, CA ajena, cadena peer ausente; cadenas EC completas válidas por DNS/IPv4/IPv6; intermedio vencido/futuro; CA configurada vencida/futura; CA válida con keyCertSign; bundle con CA vencida no usada y ruta válida; reloj ausente; cancelación durante I/O con ACK pendiente y cero bytes MQTT; reentrega de payload QoS1 pendiente tras nueva sesión verificada. Cada fallo de certificados comprueba cero bytes de autenticación/MQTT. El contraste independiente con `mqtt-packet` valida **siete transcripciones**: MQTT5, clean session/expiry0, LWT QoS1 retenido, SUBSCRIBE RAP/rh0, PUBLISH QoS1, DISCONNECT reason4/expiry0 e igualdad del payload reentregado.

La regresión de CA configurada conserva el fallo real documentado por dependencias en `/tmp/sentinel-transport-tests/worker-ca-gap.log`: se enviaba autenticación usando un trust anchor caducado. El helper activo corregido valida la CA como certificado antes de usarla como anchor y pasa la misma expectativa de cero bytes. Tests conservan reloj fijo independiente de `millis()`, certificados públicos sintéticos en PEM y stubs; DER, paquetes binarios, hex, objetos, bibliotecas y ejecutables se crean solo en temporal y se eliminan al finalizar. No se versionan claves privadas, binarios ni fuentes del proveedor.

## Inventario de retiradas

Ya retiradas por ser recetas/entradas inactivas: `firmware/CMakeLists.txt`, `firmware/src/CMakeLists.txt`, `firmware/platformio.ini`, `firmware/requirements.txt`, `firmware/sdkconfig.defaults`, `firmware/sdkconfig.sentinel_s3`; `SentinelNode/src/{sentinel,escudo,sentinel_api}/CMakeLists.txt` y `SentinelNode/src/sentinel/Kconfig.projbuild`. Las fuentes funcionales ya trasladadas al sketch no se eliminan y `firmware/EVIDENCE.md` se conserva. La retirada previa de `firmware/src/main.cpp`, `firmware/components/` y `firmware/partitions.csv` estaba en el estado inicial; se conserva su reemplazo en el sketch.

Runners Python sustituidos y retirados: `firmware/test/{run_host,check_contracts,check_config,check_migration,arduino_build_test,host_support}.py` y `firmware/test/escudo/{run_host,check_contracts}.py`. Se mantienen los casos y aserciones C++, y las expectativas de codec/recuperación del runner previo; las pruebas del hook se trasladan a la generación de mantenimiento porque Arduino consume fuente local.

## Cierre coordinado y límites físicos

La retirada esperó la señal definitiva de dependencias y del coordinador, recibida después del build completo oficial final: **PASS, exit 0**, core 3.3.12, dos bibliotecas mantenidas y nueve claves aprobadas. La evidencia de dependencias está en `/tmp/sentinel-official-firmware-final-evidence.json` y `/tmp/sentinel-official-firmware-final-build.log`: 428 entradas comprobadas sin cambios, 730 depfiles, ninguno del SDK Sentinel; FQBN `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`. ELF SHA256 `60d6e73cc23ac17520f0652d9995ea8dc20e63bd89ab2d4bd9c4f3dc5c2eb6ff`. Este agente inspeccionó esa evidencia y no duplicó la compilación Windows.

Con esa señal se retiraron definitivamente las seis herramientas de `firmware/tools/`: `generate_validator.py`, `prepare_arduino_build.py`, `build_arduino_sdk.sh`, `package_arduino_sdk.py`, `install_arduino_package.ps1` y `verify_arduino_package.py`. El único generador conservado es el mantenimiento Node fuera del build diario. El responsable de dependencias retiró `firmware/arduino-package/**`; limpieza no editó esa zona ni instalaciones globales.

Comprobación posterior a la retirada: **PASS**, `/tmp/sentinel-cleanup-retirement.json`. Las seis herramientas y el paquete antiguo ya no existen, no hay archivos Python propios ni referencias ejecutables a esas herramientas/SDK en las zonas mantenidas (`SentinelNode`, `arduino`, `test`, `tools`, scripts raíz); `firmware/EVIDENCE.md` continúa presente. Validador distribuido idéntico a la referencia y schemas, hash del TLS activo igual al build final (`315cc7bab2572abd074984766655ec50d4b889d978c92b6c904441323f20db8e`), configuración oficial/particiones y comparación de los nueve tests originales pasan de nuevo. No se repitieron suites completas porque retirar herramientas ya inactivas no cambió comportamiento. Las referencias históricas a herramientas/configuración anteriores se conservan como evidencia, no como recetas ejecutables actuales.

Las guardas/configuración Node ya están adaptadas y ejecutadas contra los archivos oficiales actuales. El fixture focalizado NVS del responsable de dependencias se integra sin cambiar sus expectativas, con stubs de solo lectura que carecen de API de escritura/borrado; estos stubs no acreditan compatibilidad física ni ABI de NimBLE. La compilación completa Windows la ejecuta dependencias y este agente no la duplica. Subida, arranque, BLE/radio y NVS reales siguen pendientes sin placa identificada; no se formatea flash/NVS ni se inventa un puerto.

## Revisión final del coordinador

Después de worker_done se detectó una recomendación residual `platformio.platformio-ide` en firmware/.vscode/extensions.json, además de c_cpp_properties.json/launch.json generados y la caché .pio/build. Se respaldó la configuración de editor en /tmp/sentinel-legacy-vscode-before-removal y se retiraron únicamente esos restos obsoletos y los directorios components/src vacíos. El respaldo de fuentes inicial y la instalación SDK global se conservan. No cambia ninguna entrada Arduino; no requiere repetir build ni suites. Inventario puntual /tmp/sentinel-final-legacy-editor-removal.json. Los patrones de ignore históricos se conservan para impedir que vuelvan a versionarse cachés.
