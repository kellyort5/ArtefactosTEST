# Firmware Sentinel — Arduino IDE Windows

**Política vigente:** [persistencia operativa y carga USB](persistence-policy.md). El estado del build con tabla nueva/configuración persistente se registra en la [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); los builds anteriores no acreditan estos cambios. Primera carga limpia desde el IDE; reinicios/cortes posteriores deben conservar datos confirmados.

Firmware modular C/C++ para ESP32-S3 N16R8, con FreeRTOS y APIs ESP-IDF disponibles en Arduino. El [sketch](../firmware/SentinelNode/SentinelNode.ino) integra supervisor, BLE, MQTT, periféricos, descubrimiento, captura, cuatro detectores y recuperación del Escudo. Cebo y Sanitizador conservan API y puntos de extensión; sus servicios siguen en sus propios encargos. Ver [contratos v2](../contracts/module-protocol.md) y [conectividad segura](connectivity.md).

**Evidencia anterior a la nueva política:** firmware completo compilado y enlazado con Arduino IDE Windows/core oficial ESP32 **3.3.12**, wolfMQTT2.2.0 y ESP_SSLClient3.1.3. El CLI nativo terminó **exit0**, con **1376726 bytes de imagen** y **160228 bytes de variables globales**; son cifras de enlace, no memoria física medida. Se comprobaron 428 entradas inmutables y 730 depfiles sin SDK Sentinel. Ver [evidencia de dependencias](../openspec/changes/sentinel-arduino-dependencies/verification.md). La compilación del SDK propio queda como antecedente. Subida, arranque, memoria, OLED/botón, BLE, radio e historial físicos siguen pendientes.

**Compilación integrada actual:** core oficial3.3.12, tabla compatible y configuración/objetivo persistentes: CLI nativo Windows **PASS exit0**, programa **1381054/4194304 bytes** y variables globales **160228/327680 bytes**. Son cifras del enlazador. Ver [evidencia actual](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); la comprobación física de placa, BLE y cortes sigue pendiente.

## Preparar Arduino IDE

El recorrido del equipo es abrir el sketch, seleccionar placa/opciones, **Verificar**, **Subir** y **Monitor serie** a 115200 en Arduino IDE Windows. Node, npm, WSL y las pruebas de mantenimiento son opcionales para este recorrido de firmware. Las siguientes dependencias y la configuración ya están preparadas en este equipo; la evidencia del build completo se registra separadamente del procedimiento y de la prueba física.

### Instalación inicial

1. Abrir Arduino IDE 2. En **Archivo → Preferencias → URLs adicionales de gestor de placas**, agregar `https://espressif.github.io/arduino-esp32/package_esp32_index.json` conservando las URLs existentes. En el gestor de placas, buscar **esp32 de Espressif Systems** e instalar **3.3.12**.
2. Descargar el ZIP de [ESP_SSLClient 3.1.3](https://github.com/mobizt/ESP_SSLClient/releases/download/v3.1.3/ESP_SSLClient.zip). En **Programa/Sketch → Incluir biblioteca → Añadir biblioteca .ZIP**, seleccionar ese archivo.
3. Cerrar Arduino IDE y abrir PowerShell en la raiz del repositorio. Ejecutar:

   ```powershell
   .\firmware\arduino\install-wolfmqtt.ps1
   .\firmware\arduino\install-board-config.ps1
   ```

4. Abrir nuevamente Arduino IDE y seleccionar las opciones de la siguiente tabla.

Los instaladores detectan las rutas configuradas por Arduino. El primero descarga wolfMQTT2.2.0 original, verifica SHA256, prepara la estructura y aplica los tres archivos Sentinel, sin instalar wolfSSL. El segundo combina cuatro claves locales y retira las cinco recetas Sentinel obsoletas de carga/exportación para el core3.3.12, conservando opciones/comentarios ajenos. No instalan otro SDK ni cambian la flash. Se ejecutan al preparar el equipo o actualizar estas dependencias, nunca por compilación.

Si ya están correctos, terminan sin descargar ni escribir. Para reparar una biblioteca distinta preparan una copia completa verificada y respaldan la anterior en `<Sketchbook>\.sentinel-backups`, fuera de `libraries`. Los cambios de configuración conservan el original en `boards.local.txt.sentinel-<id>.bak` junto al archivo. Los respaldos permanecen fuera del repositorio.

Puedes usar tu ZIP original o rutas personalizadas:

```powershell
.\firmware\arduino\install-wolfmqtt.ps1 -ArchivePath "$env:USERPROFILE\Downloads\wolfMQTT-2.2.0.zip"
.\firmware\arduino\install-wolfmqtt.ps1 -SketchbookPath 'D:\Arduino'
.\firmware\arduino\install-board-config.ps1 -ArduinoDataPath 'D:\Arduino15'
```

Para ver la acción prevista sin modificar nada, añadir `-WhatIf`. Si PowerShell bloquea los scripts por su política, se puede permitirlos **solo durante esa terminal**, sin administrador ni cambios globales, con `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`; después ejecutar de nuevo los dos comandos. No ejecutar la instalación mientras Arduino está compilando. Al cambiar de versión del core, revisar los metadatos aprobados en vez de aplicar opciones a otra versión por su nombre.

[dependencies.json](../firmware/arduino/dependencies.json) conserva versiones, URLs y hashes de archivos originales; el código del proveedor queda fuera de Git. [boards.local.txt](../firmware/arduino/boards.local.txt) es la fuente de cuatro claves: menú, `build.partitions`, máximo4MiB y `--wrap=nvs_flash_init`. `partitions.csv` y el wrapper siguen dentro del sketch; carga y exportación usan las recetas estándar de Arduino. El instalador elimina solo los cinco overrides Sentinel antiguos, conserva claves ajenas/respaldo y es idempotente.

### Seleccionar placa y verificar

Abrir `firmware/SentinelNode/SentinelNode.ino` desde el checkout completo. En **Herramientas** seleccionar:

| Opción | Valor |
| --- | --- |
| Placa | **esp32 → ESP32S3 Dev Module** del paquete oficial |
| Flash Size | **16MB** |
| Flash Mode | **QIO 80MHz** |
| PSRAM | **OPI PSRAM** |
| Partition Scheme | **Sentinel standard USB (4MiB factory, no OTA)** |
| USB CDC On Boot | **Disabled**, para la consola UART0/puente USB-UART de esta guía |
| Erase All Flash Before Sketch Upload | **Enabled** para la primera carga de la tabla nueva; después **Disabled** para carga normal sin solicitar borrado total. Ninguna actualización USB garantiza conservar datos. |

Si no aparece la partición Sentinel, repetir el instalador de configuración y revisar la versión del core antes de compilar. Pulsar **Verificar**; no ejecutar generadores previos. La primera compilación puede tardar más que las posteriores. Ante errores, conservar la salida completa para revisar dependencias/opciones; no desactivar las guardas para ocultarlos.

El usuario aprobó hasta **tres teléfonos recordados**. Si había cuatro vínculos anteriores o almacenamiento incompatible, el firmware debe detener BLE de forma segura sin borrar ni sobrescribir esos datos. Las instalaciones globales de la plataforma Sentinel anterior se conservan como antecedente y no se seleccionan para este sketch.


## Configuración y módulos

Editar [SentinelConfig.h](../firmware/SentinelNode/SentinelConfig.h) para pines y flags de aplicación. Escudo está habilitado. Mantener Sanitizador/Cebo en 0 hasta incorporar sus implementaciones reales; activarlos sin su factory debe fallar al enlazar.

| Responsable | Fuentes | Factory |
| --- | --- | --- |
| Persona 1 | `firmware/SentinelNode/src/escudo/` | `sentinel_escudo_module()` |
| Persona 2 | `firmware/SentinelNode/src/sanitizador/` | `sentinel_sanitizador_module()` |
| Persona 3 | `firmware/SentinelNode/src/cebo/` | `sentinel_cebo_module()` |

`src/sentinel_api/include/module_api.hpp` conserva la API compartida. Arduino compila recursivamente los `.cpp` existentes de `src/`; no incluir archivos `.cpp`, crear copias en `libraries/` ni editar CMake para añadir fuentes. Los módulos solicitan recursos mediante `ModuleContext`. Solo el núcleo inicializa Wi-Fi/BLE y administra NVS, MQTT y el supervisor. Ver [desarrollo e interfaces](development.md).

Persona 1 mantiene la integración común de Arduino y sus dependencias. Editar `sdkconfig.h` o definir macros en el sketch no recompila las bibliotecas del core. El objetivo sigue siendo flash 16 MiB y PSRAM octal 8 MiB/80 MHz, con comprobación física de tamaños y diagnóstico ante discrepancias. El límite factory de imagen de la partición aprobada es 4 MiB y no representa toda la flash.

## Cableado y puerto

Con la placa desconectada, comprobar el [pinout aportado](architecture.md#pinout-aportado-por-el-usuario) y los valores de `SentinelConfig.h` antes de conectar:

| Elemento | Conexión inicial |
| --- | --- |
| SSD1306 128×32 | I2C, dirección 0x3C, SDA GPIO8, SCL GPIO9, alimentación compatible de 3V3 y GND común |
| Botón externo | GPIO4 a GND, pull-up interno; no es BOOT |
| Consola | UART0/puente USB-UART a 115200; USB CDC On Boot desactivado |

No usar GPIO35–37 de memoria octal ni ocupar GPIO19/20/43/44 con estos periféricos. OLED ausente se diagnostica y deshabilita nuevos emparejamientos que requieren mostrar el código; no habilitar Just Works como sustituto.

Conectar un cable USB de datos e identificar el puerto en el Administrador de dispositivos y el selector de Arduino. COM3/COM4 de Bluetooth no identifican la ESP32. Desde PowerShell se pueden listar los puertos sin programar ningún dispositivo:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Name, PNPDeviceID
```

Seleccionar en Arduino el **COM real de la placa**, pulsar **Subir** y abrir **Monitor serie** a **115200**. Cerrar otros monitores que ocupen el puerto. Si no entra en descarga, seguir la secuencia BOOT/RESET correspondiente a la placa. Para la primera carga con tabla nueva, seleccionar **Erase All Flash Before Sketch Upload: Enabled** antes de Subir y aceptar pérdida de los datos anteriores; después volver a Disabled. No hace falta adjuntar el USB a WSL para trabajar desde Windows.

La opción Sentinel usa la tabla nueva compatible con las recetas estándar de Arduino, incluido `boot_app0` en `0xe000` reservado a `otadata`. La primera carga debe ser limpia; identidad, bonds, configuración e historial anteriores se pierden y hay que emparejar/reconfigurar. Una actualización USB normal puede reinicializarlos y no exige migración. La imagen `merged.bin` de exportación sirve para inicialización, no para preservar NVS. Con borrado total desactivado no se afirma que toda carga borre la flash. Ver [procedimiento y límites](persistence-policy.md#preparación-y-primera-carga). En arranque normal, un fallo de NVS/bonds se informa sin formato ni expulsión automática.

## Comandos Windows equivalentes

Desde la raíz del repositorio, con las bibliotecas y la opción local ya instaladas, este es el equivalente de **Verificar** mediante el CLI incluido en Arduino IDE:

```powershell
& "$env:LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe" compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel firmware/SentinelNode
```

Para programar y monitorizar, el recorrido recomendado es **Subir** y **Monitor serie** del IDE con el puerto de la placa identificado. La consulta de puertos anterior es de solo lectura y no programa ningún dispositivo.

Tras disponer de build y subida verificados, registrar la salida real de arranque: versión, device ID persistente, boot ID nuevo, flash **16777216**, PSRAM **8388608**, perfil idle y revisión 0. No confundir cifras del enlazador con memoria medida. Tras reiniciar, device ID debe conservarse y boot ID cambiar; no compartir contraseñas ni passkeys.

## Supervisor y extensión

[src/sentinel/runtime.cpp](../firmware/SentinelNode/src/sentinel/runtime.cpp) compone identidad, NVS, radio, transportes y periféricos. Arduino aporta un solo `app_main`; `setup()` arranca Sentinel y `loop()` cede CPU. Un wrapper mínimo de inicialización NVS conserva el resultado real de la primera llamada y evita que el core borre/reintente automáticamente ante falta de páginas o versión incompatible. El runtime informa el error antes de habilitar identidad/BLE/modos; no lo interpreta como almacenamiento sano. `event_log` se inicializa por separado sin formato automático.

Un único supervisor valida autorización, boot, vigencia, duplicados y revisión; confirma accepted, detiene el modo anterior, comprueba liberación y arranca el siguiente antes del resultado applied/rejected. Si falla la parada, no inicia otro modo. Los callbacks copian a colas limitadas y no hacen análisis pesado ni escrituras flash.

- Idle mantiene control BLE seguro sin servicios de módulos.
- Escudo utiliza scan pasivo y captura promiscua con BLE, sin asociarse a la red ni usar MQTT directo. Conserva selección, recorrido de canales, detectores y recuperación.
- Cebo obtiene STA/MQTT del gestor cuando su implementación se registre; HTTP/Telnet son responsabilidad de ese módulo.
- Sanitizador solicita STA o STA+AP y NAPT mediante la misma API cuando se implemente; no crea otro driver ni anuncia protección inexistente.

El botón corto rota únicamente modos registrados. Mantener 3 s abre 120 s de emparejamiento; al liberar no dispara además la acción corta. SSD1306 utiliza `esp_lcd`/I2C, sin LVGL, y muestra estado/error o passkey. Las alertas recientes en OLED mantienen su asignación en `sentinel-integracion-tres-modos`; la migración no las declara implementadas.

## Comprobaciones focalizadas

Mantenimiento opcional desde la raíz, con Node **24.13.1**, C/C++ y OpenSSL en Linux/WSL. No se necesita Python ni PlatformIO para estos runners; no son pasos para compilar o subir desde Arduino:

```sh
SENTINEL_SANITIZE=1 npm run test:firmware
SENTINEL_SANITIZE=1 npm run test:escudo
node firmware/test/arduino_build_test.mjs
```

Los runners usan cJSON 1.7.19 verificado por SHA-256 desde caché o descarga oficial. Cubren lógica real y contratos, con fallos inyectados de recursos/almacenamiento; no acreditan radio ni flash física. Ver [guía de pruebas de Persona 1](guia-pruebas-persona-1.md) para el recorrido del Escudo con ESP32/app reales.

El [mantenimiento de transportes](development.md) conserva además la regresión de CA vencida/futura y los casos de MQTT5/TLS/colas en `firmware/test/check_transports.mjs`, usando archivos originales de proveedor verificados por hash y certificados públicos sintéticos. Sus17 casos y siete transcripciones pasan por software; no agrega pasos a Verificar/Subir ni acredita MQTT/TLS en placa.

## Registro circular

`partitions.csv` reserva `otadata` en0xe000/0x2000, factory de4MiB en0x10000, NVS general de24KiB en0x410000 y `event_log` de256KiB en0x416000. No hay partición PHY separada: initdata va compilado en la app del SDK y calibración en NVS. `otadata` no habilita OTA funcional. El ring tiene **64 registros de hasta2048 bytes** cada uno. El espacio restante de flash no se anuncia como historial ni OTA.

Cada slot `slot00`…`slot63` guarda el sobre de un evento completo con ID estable y cursor persistente `journal_seq`. Los eventos de base son `system.booted`, `system.profile_changed` y `system.error` con código enumerado; no admiten cuerpos privados. Una escritura solo se considera persistida tras `nvs_set_blob` y `nvs_commit` correctos. Al arrancar se examinan los slots válidos y se reconstruye el cursor máximo sin depender de un índice auxiliar. NVS aporta verificación de integridad de sus blobs.

La sobrescritura conserva los últimos64. `read_events(after_cursor,limit)` admite1…8, emite eventos individuales y después el ack de página con `next_cursor`, rango retenido, `more` y `gap`; nunca agrega ocho mensajes grandes en uno. Un cursor antiguo o hueco recuperado produce gap y permite leer los registros válidos. Ante error de lectura/escritura se expone degradación/resource_error y se mantienen los controles; no se borra automáticamente ninguna partición.

Al reconectar MQTT se recorre el ring con los mismos IDs, sin eliminar por PUBACK. Un duplicado de consulta vigente puede reemitir la página conocida; si la sesión se interrumpe, consultar otra vez con un command ID nuevo y el cursor confirmado por la app. El cursor de la app avanza tras la página completa. Un ack MQTT no acredita recepción por la app.

La capacidad es acotada, hay desgaste y puede perderse la escritura interrumpida o un registro dañado. No se garantiza pérdida cero ante todo corte eléctrico ni retención ilimitada. Ante NVS incompatible o llena se informa error: cualquier recuperación destructiva necesita una decisión explícita del operador, fuera del arranque automático.

## Evidencia histórica y pendientes físicos

La evidencia de esta sección corresponde a la base original con PlatformIO Core6.2.0/ESP-IDF5.5.3, no al build Arduino. Conserva sus resultados y tareas físicas pendientes; no se usa como instrucción actual ni acredita la migración. El port Arduino aporta sus pruebas host en el [informe de verificación](../openspec/changes/sentinel-arduino-firmware-port/verification.md). Las comprobaciones IDE/Windows/COM tienen su propia [evidencia de integración](../openspec/changes/sentinel-arduino-firmware-port/integration-verification.md).

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
| 6.4 | Antecedente: configuración/UTC protegidos en RAM. Política actual: configuración autorizada en NVS oficial; UTC sigue transitorio. Falta prueba BLE real, restauración validada y reinicio/corte sin upload. |
| 6.5 | STA/esp-mqtt5/TLS real implementados; falta conexión desde placa y rechazo de CA/hostname/fecha incorrectos. |
| 6.6 | Topics/QoS/LWT/periódico/replay implementados; falta orden real MQTT/duplicado/retained desde placa. |
| 6.7 | Cierre STA/MQTT y reconciliación implementados; falta recorrido Cebo–Escudo físico sin asociación residual. |
| 6.8 | Flujo documentado en connectivity.md; falta ejecutar esos pasos en placa. |

Las casillas que requieren estas comprobaciones permanecen abiertas. La compilación y los tests host acreditan solo lo que ejecutan.


Compilación final del 2026-11-09 UTC (2026-11-08 en El Salvador): `pio run -d firmware -e sentinel_s3` **SUCCESS**, 116.29 s, sin errores ni warnings. RAM estática **77368/327680 bytes** (23.6 %); imagen **1183525/4194304 bytes** (28.2 % del límite factory). Son cifras de enlace, no heap libre observado, flash física detectada ni medición de PSRAM. Configuración efectiva verificada: SC-only=1, nivel4, bonds4, legacy/debug keys deshabilitados, validación de fecha TLS habilitada, flash16 MiB y PSRAM octal80 MHz. Informe de entrega: [firmware/EVIDENCE.md](../firmware/EVIDENCE.md).
