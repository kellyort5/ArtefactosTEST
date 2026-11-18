# Build nativo final — sentinel-mvp-standard-persistence

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## Resultado y entradas actuales

**PASS: compilación completa nativa Windows, exit 0**, el 2026-11-16 entre 16:09:26 y 16:19:18 UTC. Este build integra tabla nueva, cuatro claves locales, configuración Wi-Fi/MQTT persistente y objetivo Escudo persistente; no reutiliza un build histórico. Rama `feat/sentinel-arduino-migration`. No se programó hardware ni se hicieron commits, push, merge o archive.

Se utilizó el `arduino-cli.exe` incluido en Arduino IDE **2.3.10**, CLI **1.5.1**, configuración existente `%USERPROFILE%\.arduinoIDE\arduino-cli.yaml`, core oficial **esp32:esp32 3.3.12**, SDK oficial **ESP-IDF 5.5.5**. El encabezado oficial `esp_idf_version.h` declara 5/5/5 y el ELF contiene `v5.5.5`. Bibliotecas efectivas del log: **wolfMQTT 2.2.0**, **ESP_SSLClient 3.1.3**, NetworkClientSecure/Networking 3.3.12. No se instaló ni modificó core, SDK, compilador o bibliotecas.

FQBN exacto: `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`.

Directorio temporal de esta ejecución, fuera de Git: `%TEMP%\sentinel-mvp-standard-firmware\20261116T160906Z` (en adelante `$runRoot`). Se creó una copia íntegra del sketch en `$runRoot\input\SentinelNode`, idéntica por SHA256 a las fuentes actuales. Esto permite ejecutar todas las recetas estándar de exportación, que también escriben en `<sketch>\build`, sin generar artefactos en el repositorio. No se editaron fuentes durante el build.

## Inmutabilidad comprobada

Se compararon por contenido y conjunto completo de rutas los manifiestos antes/después:

| Manifiestos en `$runRoot` | Entradas | SHA256 idéntico antes y después |
| --- | ---: | --- |
| `sources-before.json` / `sources-after.json` | 53 | `34dc56a4d01407b2a7b3f409924e07ee9fb4d8fa7221158aff8f7a750483da26` |
| `staged-sources-before.json` / `staged-sources-after.json` | 51 | `36541978bc77a33d43c64acae1b5342c8754877a38ed8649517a72a580ebd1d8` |
| `dependencies-before.json` / `dependencies-after.json` | 379 | `8719d88ca9c6fd7a0e6425e77af71b295a2d564fb33e90fcdbc4fc8b32abc987` |

`sources` cubre todos los `.ino/.cpp/.h/.hpp/.c` del sketch, tabla, cuatro claves y manifiesto de dependencias; coincide además con el freeze independiente del coordinador `/tmp/sentinel-mvp-root-freeze.json`. `staged-sources` cubre el sketch temporal. `dependencies` cubre los dos árboles instalados de bibliotecas y los archivos del core de configuración/recetas/generador/boot_app0 inspeccionados. No hubo entradas nuevas, eliminadas o modificadas en esos conjuntos.

Las 49 copias C/C++/headers dentro de `build/sketch` conservan exactamente el contenido de entrada después de retirar únicamente la directiva `#line 1` añadida por el builder oficial; el `.ino.cpp` lo genera Arduino normalmente. Se produjeron 25 objetos del sketch, 312 de bibliotecas y 60 del core. El log acredita compilación real con `--clean`, sin reutilización de core/objetos compilados previamente.

## Comandos y evidencia

Comando PowerShell reproducible; las variables nombran únicamente rutas estándar del entorno y destinos temporales:

```powershell
$nativeCli = Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
$ideConfig = Join-Path $env:USERPROFILE '.arduinoIDE\arduino-cli.yaml'
$runRoot = Join-Path $env:TEMP 'sentinel-mvp-standard-firmware\20261116T160906Z'
$fqbn = 'esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel'
& $nativeCli compile --config-file $ideConfig --fqbn $fqbn `
  --build-path "$runRoot\build" --output-dir "$runRoot\export" `
  --clean --verbose --no-color "$runRoot\input\SentinelNode"
```

Se ejecutó también el mismo comando con `--show-properties=expanded` para inspección; no sustituyó la compilación. `build/build.options.json` acredita FQBN exacto, hardware oficial y `customBuildProperties` vacío. No se añadieron flags para simular características del SDK.

Archivos de evidencia conservados fuera de Git, bajo `$runRoot`:

- `native-build.ps1`, `result.json`, `compile.log` (original PowerShell UTF-16), `compile.utf8.log`, `properties.txt` y `properties.utf8.txt`.
- `sources-{before,after}.json`, `staged-sources-{before,after}.json`, `dependencies-{before,after}.json`, `toolchain-evidence.json` y `artifacts-sha256.json`.
- `integration-checks.json`, `elf-integration-checks.json`, `stock-recipes-check.json`, `global-board-check.json`, `library-provenance.json` y `partition-decoded.csv`.
- `config-check.log`, `guards-review.diff`, `security-guards-review.txt`, `inspect-elf.ps1`, `symbols.txt`, `initArduino-disassembly.txt`, `wrapper-disassembly.txt`, `elf-sections.txt`, `sentinel-native-verify.py` y `sentinel-native-elf-check.py`.

SHA256 del log nativo original: `537f65531a2b7961ce2dfbc71f44da344f46094eccdb993756e4abcef89ccc6d`; copia UTF-8: `8a270935d66310b507e2589eed735bd1eb51c651104f6e01f30c977ef9c8776c`.

Las comprobaciones suplementarias de configuración/tabla se ejecutaron desde WSL con el encabezado y generador **del core oficial instalado**; no son el build acreditado. El build, compilador, enlazador, esptool y las inspecciones ELF usan ejecutables nativos Windows del IDE/core.

## Tamaños y tabla final

El CLI informa **1,381,054 bytes (32%)** de programa frente a **4,194,304** permitidos. El archivo de imagen app mide **1,381,200 bytes**, también dentro de la partición. RAM estática: **160,228 bytes (48%)** sobre **327,680**, quedando **167,452** para variables locales. No se infiere disponibilidad dinámica de heap/PSRAM ni memoria física real a partir de estas cifras.

Flash configurada: 16 MiB / 16,777,216 bytes. El generador oficial `tools/gen_esp32part.py` lee el binario **producido por este build**, ejecuta `verify()` y `verify_size_fits(16 MiB)`, comprueba roundtrip CSV/binario y produce exactamente los mismos bytes desde el CSV actual:

| Partición | Inicio | Tamaño | Final exclusivo |
| --- | --- | --- | --- |
| otadata | `0xe000` | `0x2000` / 8 KiB | `0x10000` |
| factory | `0x10000` | `0x400000` / 4 MiB | `0x410000` |
| nvs | `0x410000` | `0x6000` / 24 KiB | `0x416000` |
| event_log | `0x416000` | `0x40000` / 256 KiB | `0x456000` |

No hay solapamientos y todo queda dentro de 16 MiB. `boot_app0.bin` oficial mide 8,192 bytes y ocupa exclusivamente `otadata` en `[0xe000,0x10000)`; no intersecta app/NVS/event_log. No hay apps OTA ni partición PHY independiente. El límite de journal de 64 eventos no se modifica.

## Recetas estándar y exportación real

La configuración global ya contenía exactamente las cuatro claves aprobadas y un respaldo del instalador comprobado por la tarea de plataforma; no fue necesario reinstalarla. `global-board-check.json` verifica etiqueta, `build.partitions=sentinel`, `upload.maximum_size=4194304` y `compiler.c.elf.extra_flags=-Wl,--wrap=nvs_flash_init`, sin las cinco recetas obsoletas.

Se expandieron los valores del `platform.txt` oficial usando las propiedades actuales y se compararon exactamente **diez recetas**: upload, merge/postobjcopy3, flash_args/postobjcopy4, copia boot_app0/postobjcopy5 y las seis postsavehex. Todas coinciden con stock; las cuatro claves no sustituyen esas recetas. La opción QIO se refleja en `build.boot=qio` y `build.memory_type=qio_opi`, OPI PSRAM, flash80MHz/16MB; el core utiliza `build.flash_mode=dio` en `flash_args` y en la imagen como parte de su receta estándar.

`build/flash_args` real y su copia exportada por postsavehex contienen:

```text
--flash-mode dio --flash-freq 80m --flash-size 16MB
0x0 SentinelNode.ino.bootloader.bin
0x8000 SentinelNode.ino.partitions.bin
0xe000 boot_app0.bin
0x10000 SentinelNode.ino.bin
```

`export/` contiene seis artefactos principales del CLI. La exportación estándar adicional en `input/SentinelNode/build/esp32.esp32.esp32s3/` contiene esos mismos binarios/ELF/map y también `boot_app0.bin`, `flash_args`, CSV, sdkconfig y build.options. Se verificó igualdad con los originales de build/core; todos los destinos están fuera de Git.

`merged.bin` existe y mide exactamente 16 MiB. Sus regiones de bootloader, tabla, boot_app0 y app coinciden byte por byte con los cuatro archivos individuales en sus direcciones estándar. Las regiones NVS/event_log de esta imagen son relleno `0xff`: es una imagen de **inicialización**, sin garantía de conservar datos durante programación USB. Esto no promete conservación con una carga USB normal ni prueba escritura física. La primera carga de la tabla nueva requiere inicialización limpia, nuevo emparejamiento/configuración y no ofrece migración.

## SDK, enlace y guardas

Se inspeccionaron 734 archivos de dependencias `.d`, propiedades y comandos reales: SDK `esp32s3-libs/3.3.12/qio_opi`, core oficial y bibliotecas indicadas. No aparecen referencias a `Documents/Arduino/hardware/sentinel` ni SDK Sentinel. La tabla de símbolos del ELF contiene los helpers productivos `load/save_connectivity` y `load/save_escudo_target`, además del wrapper, NVS real y consulta del primer resultado. El desensamblado nativo prueba dos llamadas de `initArduino` a `__wrap_nvs_flash_init`, ninguna directa a `nvs_flash_init`, y la llamada del wrapper al NVS real. La protección se aplica antes de `setup()`.

PASS de integración:

```text
node firmware/test/check_config.mjs --sdkconfig <SDK oficial>/qio_opi/include/sdkconfig.h --partition-tool <core oficial>/tools/gen_esp32part.py
```

Revisión frente al snapshot inicial: wrapper NVS, guardas de transporte, verificador TLS y adaptador BLE idénticos. `arduino_requirements.cpp` conserva guardas de core/partición, RAM/flash, SC/bonds, TLS/tiempo/cadena y NAPT; solo cambia el diagnóstico del menú y añade rechazo de PHY init en partición. No se degradan controles. La compilación actual incluye estas unidades y acepta el SDK efectivo sin overrides de `CONFIG_*`.

No se repitieron suites host/instalador: plataforma/configuración ya entregaron sus PASS y el coordinador confirmó el PASS final de Escudo con ASan/UBSan, 197 mensajes codec/ACK y trece fuentes previas de pruebas críticas inmutables (`ctx_0cbf0773da35`), conservando el freeze productivo. Esos resultados corresponden a sus tareas, no a ejecuciones nuevas de este worker. Esta tarea cubre el build final y la evidencia de integración faltantes.

## Límites y pendientes físicos

Pendientes: primera carga limpia en placa identificada, emparejamiento BLE protegido con botón/OLED, configuración y conexión MQTT/TLS reales, selección Escudo, regreso a modo con captura detenida, reinicio y corte/restauración de alimentación después de escrituras confirmadas, conservación de identidad/bonds/configuración/objetivo/historial y reconciliación de cursores tras USB. Ver la [política de persistencia](../../../docs/persistence-policy.md) y la [verificación general](verification.md).

Ninguna prueba física se declara PASS por este build, por análisis estático o por dobles NVS. No se garantiza una escritura interrumpida ni cifrado físico de flash. La ejecución no abrió un puerto de carga, no subió firmware y no modificó fuentes del repositorio; este registro es el único archivo creado por este worker dentro de Git.

## SHA256 de artefactos actuales

Los nombres de los primeros seis archivos corresponden a `export/`; `boot_app0.bin` corresponde a la exportación estándar adicional. Manifiesto completo: `artifacts-sha256.json` fuera de Git.

| Archivo | Bytes | SHA256 |
| --- | ---: | --- |
| `SentinelNode.ino.elf` | 14,980,604 | `4128fdb47228b14875b4b9ee4b517bd9b158c123789e6dd70c610d6f046f2c92` |
| `SentinelNode.ino.bin` | 1,381,200 | `129294e22d0aef5b72a357fd6caba17101e09efc52bc41624316ce1b71e7b5b3` |
| `SentinelNode.ino.partitions.bin` | 3,072 | `e1908d6b41dfecdfa4df3818adcca7476f17d9a06566e44502422ea2f12f2bc9` |
| `SentinelNode.ino.bootloader.bin` | 19,968 | `84bb365666003b051f564d04380d0cf5d9d85c420ddafe9fc863071b49e49bfc` |
| `SentinelNode.ino.merged.bin` | 16,777,216 | `610370e26ce7e43853aff78a5c3176c25157dd9132e5d7b3666be091150c5614` |
| `SentinelNode.ino.map` | 18,252,395 | `9e427e9030815af256643218ba6669c89122249fe0c52e5a21c414d94ea1e07d` |
| `boot_app0.bin` | 8,192 | `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0` |
