# Verificación de ejecución — sentinel-arduino-sdk

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha: 2026-11-13. Repositorio: `C:\Users\Jefry\Documents\Arduino\ArtefactosProyecto`, rama `feat/sentinel-arduino-migration`, HEAD `af7200ab1047f2d0dfc09b64047dfec6a3ce3309`. Se aplicó `openspec-apply-change`; instrucciones actuales `ready`, esquema `spec-driven`. Las casillas de `tasks.md` quedan bajo control del coordinador. No se realizaron commits, push, merges ni operaciones sobre componentes externos.

## Resultado y artefactos

SDK ESP32-S3 real reconstruido y paquete Windows disponible, con placa `sentinel:esp32:sentinel_s3_n16r8`, versión `3.3.12-sentinel.1`. Instalación activa: `C:\Users\Jefry\Documents\Arduino\hardware\sentinel\esp32`. Boards Manager conserva su core estándar separado. La instalación activa tiene el mismo manifiesto, recetas y hashes de la distribución definitiva utilizada en el build nativo satisfactorio.

- ZIP definitivo: `C:\Users\Jefry\AppData\Local\Temp\sentinel-arduino-sdk\release\sentinel-esp32s3-n16r8-3.3.12-sentinel.1-windows.zip`.
- Copia Linux: `/home/jefry/.cache/sentinel-arduino/package-v2/sentinel-esp32s3-n16r8-3.3.12-sentinel.1-windows.zip`.
- SHA256 ZIP: `6c3bdd904f62b01d7c64342c47d81f38aad22c018cc6d671578e46c82e2d24b0`.
- Tamaño ZIP definitivo: 576.518.681 bytes.
- SHA256 `tools/sentinel-manifest.json`: `bfeb51cbbd2d287ed51cfc377eb526b25e1dd1931309cf4736a2d3924dfa1b8a`. Contiene inventario de 8.174 archivos y evidencia de construcción; no se versiona la distribución binaria.
- SDK exportado: `/home/jefry/.cache/sentinel-arduino/builder/out/tools/esp32-arduino-libs/esp32s3`, 101 archivos `.a` en `lib/`, además de bibliotecas específicas QIO/OPI y bootloader propio.

## Tareas verificadas y evidencia

| Tarea | Evidencia concreta |
| --- | --- |
| 1.1 | [Manifiesto fuente](../../../firmware/arduino-package/manifest.json), overrides sin asignaciones duplicadas y SHA de distribución. Core `94afccf35fb1e401facddbcf9e13bcf7c76a31d8` (3.3.12), IDF `b774170ff46c393eeb5e495ea37936038d3f4f4f` (5.5.5), builder `6671d0bd65cdb9d4cc1001b759e8610de945a8d5`. |
| 1.2 | Primer SDK `sdk-build.log` exit 0; reproducción mediante script definitivo `sdk-repro-build.log` exit 0, termina `SDK built: .../esp32s3`. Config efectiva idéntica al primer build, después de normalizar la asignación redundante de selección Arduino. |
| 1.3 | [Parche NVS](../../../firmware/arduino-package/patches/0001-preserve-nvs-before-setup.patch), ejecutado sobre el `initArduino()` real con seis resultados inyectados y ASan/UBSan: una inicialización, cero borrados/reintentos y accessor conserva cada resultado. La implementación se enlaza en ELF Windows como `sentinel_arduino_nvs_status`. |
| 1.4 | Instalador PowerShell nativo: extracción/verificación completa antes de activar; salida `Installed sentinel:esp32:sentinel_s3_n16r8 at C:\Users\Jefry\Documents\Arduino\hardware\sentinel\esp32`, exit 0. `board listall Sentinel --format json` identifica `manually_installed: true`, plataforma compatible y versión correcta. Herramientas propias, sin referencia a los binarios globales de Boards Manager en compilación/upload. |
| 2.1 | `arduino-cli.exe` Windows 1.5.1, commit `01f3d4f2b`, compila/enlaza `Capabilities.ino`, exit 0; `Sketch uses 337018 bytes (8%)`, `Global variables use 23300 bytes (7%)`. Log `capabilities-windows-v2.log`. |
| 2.2 | [Guía reproducible](../../../firmware/arduino-package/README.md), builder/empaquetador/instalador/verificador fuente, hashes, logs y esta evidencia. Builds/ZIP/SDK/caches fuera del repositorio; ninguna prueba física se declara realizada. |

Los logs de esta tabla están en `/home/jefry/.cache/sentinel-arduino/`. La preservación de trabajo previo corresponde al snapshot del coordinador `/tmp/arduino-execution-before-dirty.json`; no se editaron archivos ajenos a la propiedad asignada ni `tasks.md`.

## Capacidades compiladas, seguridad y coherencia

`sdkconfig` y `qio_opi/include/sdkconfig.h` efectivos coinciden y el verificador los contrasta con hashes de las bibliotecas compiladas, del core y de las recetas. No se reconfiguró el SDK editando un header.

- MQTT5 `CONFIG_MQTT_PROTOCOL_5=y` y símbolos reales `esp_mqtt5_client_set_connect_property`, `set_publish_property`, `set_user_property`, `delete_user_property` en `libmqtt.a`.
- TLS con `CONFIG_MBEDTLS_HAVE_TIME=y`, `CONFIG_MBEDTLS_HAVE_TIME_DATE=y`, TLS1.2 y opciones inseguras deshabilitadas. `libmbedx509.a` contiene verificación X509 y las funciones de tiempo pasado/futuro; no hay `setInsecure` ni un fallback de fechas.
- NimBLE habilitado, SC-only=1, nivel 4, legacy/debug keys deshabilitados, un enlace y cuatro bonds persistentes. La inicialización del host y las autorizaciones GATT siguen siendo propiedad del firmware.
- Forwarding/NAPT reales y `esp_netif_napt_enable` presente. Wi-Fi NVS deshabilitado para que el driver no persista credenciales de conectividad.
- Flash 16 MiB, PSRAM OPI/80MHz, error ante PSRAM ausente. Estos son parámetros compilados; no son evidencia de capacidad física de una placa conectada.
- Core y SDK comparten las revisiones fijadas. Compilador Linux y Windows: `esp-14.2.0_20260121`, GCC 14.2.0. Python embebido Windows 3.12.10; esptool 5.3.1. Los orígenes y checksums oficiales de herramientas constan en el manifiesto fuente, y cada ejecutable distribuido en el inventario final.

SHA256 de bibliotecas y core:

| Archivo | SHA256 |
| --- | --- |
| `libmqtt.a` | `00bcb41cc49a64c2a730f09d353751e4c1b5469452ab884443433cd2826bdd93` |
| `libmbedx509.a` | `ba31dd9e00a196b34b6609c92d9ed0ef6e85c1629dfaa6ad0fe38a568eb7a575` |
| `libbt.a` | `068ddff3bc848f0cfc061bd91b471f056dc4275fdb23dd528fbcccd025ef769b` |
| `libesp_netif.a` | `10668c31f2f1ceaa6f7e1d714664df34e2c7ad7b5eccf148d94dbbe9c0cbdd10` |
| Core `esp32-hal-misc.c` parcheado | `904960b1cf589d9d90a3e2c1f5551a0bdab2c1ff267405ecf124798431711d93` |

Los SDKs ajenos Matter/RainMaker/cámara/voz/Zigbee no se resuelven en los manifiestos del builder; no se retira ninguna capacidad de Sentinel. `CONFIG_ARDUINO_SELECTIVE_COMPILATION=n` es la única selección explícita y coincide con lo compilado.

## NVS, particiones y upload

El parche conserva una única `nvs_flash_init()` antes de setup y expone `extern "C" esp_err_t sentinel_arduino_nvs_status(void)`. El port consulta el resultado guardado para impedir perfiles cuando el almacenamiento no está disponible. El core no formatea ni reintenta NVS automáticamente.

Se detectó y corrigió otro riesgo real: la receta estándar escribe `boot_app0.bin` en `0xe000`, dentro de la partición NVS original `0x9000..0xefff`. Sentinel conserva su tabla factory y omite esa inicialización OTA. El upload es disperso: `0x0` bootloader, `0x8000` tabla y `0x10000` aplicación. No genera imagen merged/padded de toda la flash. El verificador rechaza una receta incompatible y el hook rechaza tablas del sketch distintas de la conservada.

La tabla binaria del build Windows se decodificó y conserva exactamente `nvs=0x9000/0x6000`, `phy_init=0xf000/0x1000`, `factory=0x10000/0x400000`, `event_log=0x410000/0x40000`. `flash_args` no contiene `0xe000`/`boot_app0`, y no hay `.merged.bin`.

El backup del instalador va a `Sketchbook/.sentinel-backups/<fecha-id>/esp32`, y el staging fuera de `hardware`, para evitar que Arduino descubra otra arquitectura. La extracción rechaza rutas fuera del destino y comprueba SHA256 e inventario antes de mover la instalación previa.

Se instaló también el ZIP **definitivo** con PowerShell en el Sketchbook temporal `C:\Users\Jefry\AppData\Local\Temp\sentinel-arduino-installer-test`, sin modificar la instalación compartida durante el build del coordinador. `install-release-windows.log`, exit 0, registra `Sentinel package: PASS`, preservación de la instalación fixture en `.sentinel-backups/20261113-201936-1f1a6ecd8be14c08a8e63d43e37e7fdd/esp32` y activación de la plataforma nueva. El archivo marcador previo se conserva intacto; no aparece otra arquitectura de backup dentro de `hardware/sentinel`.

## Comandos y evidencia nativa

```powershell
$cli = 'C:\Users\Jefry\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
& $cli --config-file 'C:\Users\Jefry\.arduinoIDE\arduino-cli.yaml' compile `
  --fqbn sentinel:esp32:sentinel_s3_n16r8 `
  --build-path 'C:\Users\Jefry\AppData\Local\Temp\sentinel-arduino-capabilities' `
  --jobs 4 --verbose `
  'C:\Users\Jefry\Documents\Arduino\ArtefactosProyecto\firmware\arduino-package\examples\Capabilities'
```

`capabilities-symbols.log` confirma símbolos definidos en el ELF final: MQTT5 connect/publish properties, NAPT, `ble_gap_security_initiate`, verificación X509 y accessor NVS. El ejemplo solo imprime sus direcciones; no usa una red ni una placa para generar esta evidencia.

- ELF: `C:\Users\Jefry\AppData\Local\Temp\sentinel-arduino-capabilities\Capabilities.ino.elf`, SHA256 `316f7c8e6a0853f533367641b708f93bfc0fef4f48e9366405e202c3840c1323`.
- BIN: SHA256 `572a01bf1c907760e9249d7885cbbf084001edd8705131cfc2e58b7d933d9fd7`.
- Bootloader: SHA256 `1225b26259441bf1227446bc156f4bed021da4d04c615dc39c53ff080501b981`.
- Tabla binaria: SHA256 `50b80a0d33954e432309b9e096b57c9068e6b46c6714a390579ed1da8c28c3f0`.

Rechazo nativo, `rejection-windows.log`, exit 0 del runner y exit 2 en cada verificación negativa:

```text
PASS rejection: Not a Sentinel distribution: missing tools/sentinel-manifest.json
PASS rejection: Incompatible SDK: CONFIG_SPIRAM_MODE_OCT=n, requires y
PASS rejection: Core does not preserve its single NVS initialization result
```

El segundo caso coloca el manifiesto Sentinel delante del SDK estándar real: no basta con cambiar el nombre del paquete o sus defines. La prueba del core destructivo también usa el arranque estándar real. El inventario validado protege además contra reemplazar bibliotecas manteniendo headers compatibles.

## Ajustes resueltos y límites

El primer compile Windows detectó rutas de herramientas incorrectas por la diferencia entre `platform.txt` de Git y el transformado de Boards Manager. Se corrigieron explícitamente en el empaquetador y el siguiente build nativo terminó con exit 0; el log fallido inicial se conserva como antecedente, no como aceptación.

Arduino 1.5.1 ordena hooks lexicográficamente. La receta definitiva usa `.099` para verificar antes de los hooks originales y de `.100`, que ejecuta `prepare_arduino_build.py --sketch ... --build-path ...`. Se preservan las recetas originales de particionado y los flags existentes; los includes agregados son `generated`, `src/sentinel/include`, `src/sentinel_api/include` y `src/escudo/include`.

La compilación de firmware completo/Escudo corresponde a la integración del coordinador; no se duplicó esa ejecución ni se modificaron sus fuentes/runners. Compilar el paquete no acredita radio, pairing SC/MITM, rechazo TLS remoto, OLED/botón, PSRAM física, persistencia en placa, upload ni monitor físico. Esas pruebas requieren evidencia de hardware y permanecen pendientes.

El coordinador confirmó posteriormente **PASS exit 0** del firmware completo Windows, log `/tmp/sentinel-arduino-full-compile.log`: 1.358.113/4.194.304 bytes de programa y 161.556/327.680 bytes de RAM estática. Es evidencia aportada por la integración, no una segunda ejecución de este worker.
