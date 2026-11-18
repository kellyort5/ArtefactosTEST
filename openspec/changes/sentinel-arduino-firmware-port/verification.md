# Verificación del port de fuentes

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha: 2026-11-13, America/Guatemala. Rama compartida:
`feat/sentinel-arduino-migration`; referencia anterior `af7200a`. Esta evidencia
cubre el trabajo de fuentes de 1.1–1.4 y la regresión 2.1. El coordinador marca
las tareas; este worker no modificó `tasks.md`. Sin commits, push, merges,
archivo de specs, borrado de flash ni cambios en contratos/app/broker/
notifications/simulator. El trabajo previo sin commit se conservó.

## Movimiento y adaptación

Se movieron completos `firmware/components/{sentinel,sentinel_api,escudo}` a
`firmware/SentinelNode/src/{sentinel,sentinel_api,escudo}`. Headers conservan
sus directorios `include/`; no hay copias mantenidas ni `.cpp` en el árbol
anterior. `firmware/src/main.cpp` se convirtió en `SentinelNode.ino`: Arduino
aporta el único `app_main`, setup llama una vez a start y loop cede CPU.
Se agregó también una guarda atómica a start contra llamadas repetidas.

`SentinelConfig.h` separa pines/módulos de sdkconfig. Conserva SDA8, SCL9,
GPIO4 y SSD1306 0x3c. Escudo está habilitado; Sanitizador/Cebo permanecen a
0 y conservan las declaraciones de factories y la API completa. Las
condiciones son `#if` por valor, y habilitar un modo sin factory falla al
enlazar. No se agregaron servicios ficticios ni se contabilizaron como prueba.

El runtime consulta `sentinel_arduino_nvs_status()` exportado por el core
parcheado: no repite nvs_flash_init ni borra identidad/bonds/eventos. NVS
event_log conserva su inicialización de partición separada. Una identidad no
recuperable detiene el arranque antes de perfiles/BLE. El sketch incluye
`esp32-hal-alloc-ble-mem.h` para reservar memoria antes de initArduino; se
conserva solo NimBLE C. Network conserva driver/netifs exclusivos y
WIFI_STORAGE_RAM, sin WiFi.begin. Se aceptó únicamente el resultado documentado
ESP_ERR_INVALID_STATE del event loop ya creado, sin ignorar otros errores.

La tabla `firmware/partitions.csv` se movió sin cambiar bytes a la raíz del
sketch: NVS 0x9000/0x6000, phy 0xf000/0x1000, factory 0x10000/0x400000,
event_log 0x410000/0x40000. Journal64, 2048 bytes, namespaces/slots, formatos
NVS, identidad, UUID, framing, gate/ACK/boot/revisión y políticas de transporte
no se reescribieron. Las recetas CMake/Kconfig trasladadas son referencias
inactivas; el CMake raíz rechaza explícitamente el flujo antiguo. PlatformIO
queda solo como referencia histórica y no mantiene otro runtime.

## Contrato SDK / Arduino Windows

FQBN acordado: `sentinel:esp32:sentinel_s3_n16r8`. SDK y plataforma pertenecen
al cambio separado `sentinel-arduino-sdk`; no los editó este worker.

El paquete ejecuta su verificador separado `.099`:

```text
<python-windows> <runtime.platform.path>/tools/verify_arduino_package.py --platform <runtime.platform.path>
```

Luego invoca el generador `.100`, conservando los hooks originales de
particionado. Rutas siempre entre comillas en la receta:

```text
<python-windows> <runtime.platform.path>/tools/sentinel_prepare.py --sketch <build.source.path> --build-path <build.path>
```

La receta definitiva usa el wrapper del paquete, que localiza y llama al
generador del repositorio con los mismos argumentos. El verificador `.099`
se ordena antes de los hooks originales y de `.100` por orden lexicográfico.
El empaquetador agrega los includes a `compiler.cpreprocessor.flags`
conservando el valor existente, sin edición manual del IDE.

Los includes C++ requeridos son:

```text
-I"{build.path}/generated"
-I"{build.source.path}/src/sentinel/include"
-I"{build.source.path}/src/sentinel_api/include"
-I"{build.source.path}/src/escudo/include"
```

El hook usa el Python que lo invoca y schemas reales de
`contracts/schemas/v2`, genera solo `generated/v2_validator.inc` en el build
temporal y deja recibo SHA-256 de inputs/output. Regenera en cada build y
falla ante entradas ausentes/keywords no soportados, incluso Python -O. No
genera `.cpp` ni escribe fuentes del sketch. El validador generado tiene el
mismo SHA-256 que antes: `9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68`.
El include y salidas de firmware/cachés Python están excluidos de Git.

`arduino_requirements.cpp` rechaza headers sin MQTT5, TLS con fechas, SC-only,
bonds persistentes, NAPT, 16 MB y OPI80. Los checks sobre headers no sustituyen
la comprobación de archivos/símbolos del SDK real. El símbolo NVS del core
parcheado es obligatorio al enlazar; no existe fallback al core estándar.

## Pruebas ejecutadas y resultados

El coordinador había terminado las pruebas **anteriores** con ASan/UBSan:
`/tmp/sentinel-arduino-baseline-core.log` y
`/tmp/sentinel-arduino-baseline-escudo.log`, ambas PASS.

Comandos después del port desde la raíz, con Node 24.13.1 en PATH:

```sh
SENTINEL_SANITIZE=1 python3 firmware/test/check_contracts.py
SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
python3 firmware/test/arduino_build_test.py
python3 firmware/test/check_migration.py --baseline /tmp/arduino-execution-baseline.json
git diff --check
```

- Núcleo/cross-codec **PASS**: gate/dispatch/leases exclusivos/stop timeout,
  colas, comandos, framing, botón, supervisor, journal/recovery/overwrite/error;
  fixtures de comandos y **69 fixtures v2**, serialización C++→AJV v1/v2.
  Log: `/tmp/sentinel-arduino-port-core.log`.
- Escudo con ASan/UBSan **PASS**: catálogo/SSID/dedup/overflow/cursores,
  hooks reales, gate/ACK, selección, fallos scan/backpressure/Idle/restart;
  callback epochs/bounds, parser management/IE/RSN, canales, métricas/cobertura,
  rescan/failed-stop; cuatro detectores y sus casos benignos/inconclusos;
  recuperación write/recover/checks en procesos separados, paginación/gap,
  identidad entre boots y deduplicación (**6 únicos y 6 repetidos**).
  Cross-codec verifica tandas de 70/72/30/18/7 mensajes y 19 ejemplos
  documentados. Log: `/tmp/sentinel-arduino-port-escudo.log`.
- Frontera Arduino **PASS**, 3 tests: rutas con espacios y schemas reales,
  regeneración/cambio/rechazo/entradas ausentes/salida fuera del sketch;
  compilar registro real con todas las combinaciones relevantes de flags,
  inspeccionar símbolos y rechazar factories ausentes; rechazar headers SDK
  incompletos/inseguros. Los headers sintéticos prueban rechazo, nunca un SDK
  real ni los servicios ausentes. Log: `/tmp/sentinel-arduino-port-hook.log`.
- Integridad **PASS**, **36 archivos idénticos SHA-256** contra
  `/tmp/arduino-execution-baseline.json`: todas las fuentes/headers/recetas de
  Escudo, API, headers y lógica portable del núcleo, casos C++ y particiones.
  Log detallado: `/tmp/sentinel-arduino-port-integrity.json`. Las aserciones y
  fixtures originales no se modificaron ni se cambiaron resultados esperados.
- Sintaxis Python y `git diff --check`: **PASS**.

Los runners solo adaptan rutas y usan cJSON 1.7.19 idéntico por SHA-256. Pueden
usar caché/descarga oficial sin instalar otro framework. `check_config.py`
ahora recibe `--sdkconfig <sdkconfig.h efectivo>` y comprueba configuración de
aplicación por separado. Se ejecutó con el header **legacy anterior** solo
como comprobación del checker/particiones (PASS en
`/tmp/sentinel-arduino-port-config-legacy.log`); eso **no** prueba el SDK Arduino.

## Pendiente de integración y hardware

No se declara compilación/enlace completos con SDK Arduino Windows, apertura
del IDE, upload COM, radio/BLE físico ni NVS flash probados por esta evidencia.
La plataforma reconstruida no estaba entregada durante esta comprobación.
La integración 2.2–2.3 corresponde al coordinador tras recibir el SDK; la
documentación general 2.4 tiene otro propietario. P2/P3 siguen pendientes de
sus implementaciones y pruebas reales.

Comandos finales previstos en PowerShell (CLI nativo del IDE y Python Windows
previamente localizados; ningún comando de borrado):

```powershell
& $sentinelPython 'firmware/tools/verify_arduino_package.py' --platform "$env:USERPROFILE/Documents/Arduino/hardware/sentinel/esp32" --json
& $sentinelArduinoCli compile --fqbn sentinel:esp32:sentinel_s3_n16r8 --build-path "$env:TEMP/sentinel-arduino-final" 'firmware/SentinelNode'
# Solo tras identificar placa/COM y conservar política de flash:
& $sentinelArduinoCli upload --fqbn sentinel:esp32:sentinel_s3_n16r8 --port $sentinelCom --input-dir "$env:TEMP/sentinel-arduino-final" 'firmware/SentinelNode'
& $sentinelArduinoCli monitor --port $sentinelCom --config baudrate=115200
```

Los compañeros agregan fuentes existentes a `SentinelNode/src/sanitizador`
o `src/cebo`, exportan su factory e implementan la misma API/leases. El flujo
está documentado en `firmware/SentinelNode/README.md`; no requiere otro
framework, copias de bibliotecas ni editar el supervisor.
