# Verificación de integración Arduino Windows

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha: **2026-11-13**, America/Guatemala. Repositorio `C:\Users\Jefry\Documents\Arduino\ArtefactosProyecto`; rama `feat/sentinel-arduino-migration`, baseline `af7200ab1047f2d0dfc09b64047dfec6a3ce3309`. Tarea Orca `task_4491e2f691d6`. Este informe cubre documentación y revisión final separadas de los autores del SDK y del port; no modifica `tasks.md` ni declara aceptación física.

## Resultado para el coordinador

| Tarea del port | Evidencia y estado verificable |
| --- | --- |
| 2.1 | Regresión núcleo/Escudo e integridad PASS aportadas por su autor, revisadas sin repetirlas. |
| 2.2 | **Parcial:** firmware completo compilado/enlazado por el coordinador con CLI nativo Windows del IDE, exit 0. Esta integración revisó ELF, dependencias y placa instalada. Captura real prueba apertura del sketch en IDE 2.3.10. La elección de placa y Verificar mediante GUI siguen sin ejecutar por foco/accesibilidad; no presentarlas como hechas. |
| 2.3 | **Pendiente físico:** solo COM3/COM4 Bluetooth; ningún COM identificado como ESP32 y ningún teléfono en ADB. No hubo upload, monitor físico, boot/memoria/OLED/botón/BLE/radio ni prueba NVS en placa. |
| 2.4 | Documentación actualizada de todos los módulos, R-01/H-02/stack/roadmap, instalación/desarrollo y guías de Persona 1/2/3; auditoría de enlaces y propuestas activas completada. El coordinador contrasta esta evidencia y marca sus casillas. |

No falló el build real. Los bloqueos pendientes son el control visual del IDE y la disponibilidad de hardware; un build adicional no los sustituye. Los servicios Sanitizador/Cebo y las pantallas específicas/alertas OLED conservan sus encargos y pendientes.

## Fuentes de evidencia revisadas

- [SDK: distribución y pruebas reales](../sentinel-arduino-sdk/verification.md).
- [Port: movimiento, inicialización y regresión crítica](verification.md).
- [Compilación Windows del coordinador](coordinator-verification.md), `/tmp/sentinel-arduino-full-compile.log` y `/tmp/sentinel-arduino-full-build-receipt.json`.
- Baseline anterior ASan/UBSan: `/tmp/sentinel-arduino-baseline-core.log`, `/tmp/sentinel-arduino-baseline-escudo.log`; después del port: `/tmp/sentinel-arduino-port-core.log`, `/tmp/sentinel-arduino-port-escudo.log`, `/tmp/sentinel-arduino-port-hook.log`, `/tmp/sentinel-arduino-port-integrity.json`.

Se aplicó la skill local `openspec-apply-change`: esquema `spec-driven`, instrucciones `ready`, 5/8 tareas verificadas al empezar; se leyeron proposal, design, ambas specs y tasks devueltos por `contextFiles`. El usuario reserva las casillas al coordinador, así que este worker no las edita. Se conservaron el snapshot previo `/tmp/arduino-execution-before-dirty.json` y el mapa `/tmp/arduino-execution-baseline.json` sin recrearlos ni sobrescribirlos.

## Paquete, build y dependencias Windows

Plataforma real instalada en `C:\Users\Jefry\Documents\Arduino\hardware\sentinel\esp32`, separada del core oficial en Arduino15. Paquete `3.3.12-sentinel.1`, FQBN `sentinel:esp32:sentinel_s3_n16r8`, nombre **Sentinel ESP32-S3 N16R8 (secure SDK)**. El CLI nativo identifica `manually_installed:true` y `compatible:true` con `board listall Sentinel --format json`.

ZIP real `C:\Users\Jefry\AppData\Local\Temp\sentinel-arduino-sdk\release\sentinel-esp32s3-n16r8-3.3.12-sentinel.1-windows.zip`, 576.518.681 bytes; SHA256 `6c3bdd904f62b01d7c64342c47d81f38aad22c018cc6d671578e46c82e2d24b0`. El manifiesto distribuido SHA256 `bfeb51cbbd2d287ed51cfc377eb526b25e1dd1931309cf4736a2d3924dfa1b8a` conserva inventario de 8.174 archivos. Fabricación Linux/WSL y herramientas de trabajo Windows están descritas en la guía del paquete (`firmware/arduino-package/README.md`, artefacto SDK local retirado); los binarios permanecen fuera de Git.

El coordinador ejecutó el sketch completo y este worker reutilizó la evidencia conforme a su instrucción, sin repetir el build:

```powershell
$sentinelCli = Join-Path $env:LOCALAPPDATA 'Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe'
Set-Location 'C:\Users\Jefry\Documents\Arduino\ArtefactosProyecto'
& $sentinelCli compile --fqbn sentinel:esp32:sentinel_s3_n16r8 `
  --build-path "$env:TEMP/sentinel-arduino-firmware" firmware/SentinelNode
```

Arduino IDE **2.3.10**, CLI incluido **1.5.1**, commit `01f3d4f2b`; **exit 0**, imagen **1.358.113/4.194.304** bytes y RAM estática **161.556/327.680** bytes. Son cifras del enlazador, no medición física.

ELF real `%TEMP%/sentinel-arduino-firmware/SentinelNode.ino.elf`: este worker recalculó SHA256 **`7ec3342610ea3191f4bb39a4c579df13dfa18fbdbbfc6f62249341b8b376f044`**, coincidente con el recibo. El recibo del coordinador confirma un solo `app_main`, `sentinel_escudo_module` y `sentinel_arduino_nvs_status`; factories de P2/P3 ausentes honestamente. Se enlazan los seis objetos reales de Escudo: module, discovery, capture, channel_scheduler, frame_parser y detectors.

Hay bibliotecas globales de otros sketches en el equipo (Adafruit, ArduinoJson, PubSubClient y otras); **no se afirma que estén desinstaladas**. Se comprobó que no ocultan dependencias de Sentinel: `compile_commands.json` contiene **78 unidades**, ninguna unidad de biblioteca externa; **96 depfiles** sin rutas de `<Sketchbook>/libraries` ni `Arduino15/libraries`. `libraries.cache` contiene únicamente core y variante de la plataforma Sentinel. Informe `/tmp/sentinel-integration-dependencies.json`; no se alteró la instalación global para lograr este resultado.

El paquete instalado ejecuta verificador `.099`, después wrapper `tools/sentinel_prepare.py` en `.100`, que llama al generador del checkout. Los includes se añaden con `compiler.cpreprocessor.flags`. El include generado fuera del sketch tiene SHA256 `9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68` y recibo de 15 inputs reales. No se mezclan SDK/header estándar ni se sustituyen contratos.

Se cerró un hueco de integración: el checker del autor se había ejecutado con el header legado; aquí se ejecutó contra el **SDK Arduino instalado**:

```sh
python3 firmware/test/check_config.py --sdkconfig /mnt/c/Users/Jefry/Documents/Arduino/hardware/sentinel/esp32/tools/esp32-arduino-libs/esp32s3/qio_opi/include/sdkconfig.h
```

**PASS**, exit 0: seguridad/memoria configuradas, pines y flags de aplicación, particiones dentro de 16 MiB. Log `/tmp/sentinel-integration-effective-config.log`. No acredita memoria física. La prueba completa de configuración/bibliotecas, símbolos y parche NVS sigue documentada por el SDK; no se deduce de headers aislados.

## IDE visible: alcance y bloqueo auténtico

Se leyó la skill `computer-use` y su guía de Orca antes de operar. `orca-ide computer list-apps/list-windows` identifica **PID 34988**, ventana **3083754**, título **SentinelNode | Arduino IDE 2.3.10**, visible/no minimizada. Se comprobó el contenido real por una captura nativa Windows `PrintWindow` del handle, guardada en `C:\Users\Jefry\AppData\Local\Temp\sentinel-ide-observe.png`; muestra `SentinelNode.ino`, `SentinelConfig.h`, entrypoint correcto y **Select Board** todavía sin elección.

La observación de accesibilidad de Orca con `--max-elements 10 --no-screenshot` agotó 30 s (`action_timeout`), igual que los intentos previos del coordinador. Un único click al selector con `--restore-window` fue rechazado **`window_not_focused`**, indicando expresamente que no se envió input. No se repitió el foco ni se reportó un click exitoso. Recibos `/tmp/arduino-integration-gui-state.json` y `/tmp/arduino-integration-gui-click.json`; script de captura Windows `%TEMP%/sentinel-ide-observe.ps1`.

El backend del IDE demuestra que la placa está instalada y puede compilar el sketch con ese FQBN; la captura demuestra apertura en la GUI. Ninguno prueba que se haya elegido la placa en el selector o pulsado Verificar. El coordinador fue informado y pidió al usuario poner el IDE al frente/seleccionar placa. Hasta contar con esa evidencia, conservar 2.2 parcial.

## Detección de dispositivos y procedimiento pendiente

Inventario real ejecutado con PowerShell Windows y herramientas WSL; log `/tmp/sentinel-integration-inventory.log`, script nativo `%TEMP%/sentinel-integration-inventory.ps1`:

```powershell
& $sentinelCli version
& $sentinelCli --config-file "$env:USERPROFILE/.arduinoIDE/arduino-cli.yaml" board listall Sentinel --format json
& $sentinelCli board list --format json
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Name, PNPDeviceID
Get-PnpDevice -PresentOnly | Where-Object { $_.Class -in @('Ports','AndroidUsbDeviceClass','WPD') -or $_.FriendlyName -match 'ESP32|CP210|CH34|CH910|Android|ADB' }
& "$env:LOCALAPPDATA/Android/Sdk/platform-tools/adb.exe" devices -l
```

Solo **COM3/COM4, Standard Serial over Bluetooth link**, identificadores `BTHENUM`; ningún puente ESP32/USB-UART presente. ADB Windows y `/home/jefry/Android/Sdk/platform-tools/adb devices -l` devolvieron lista vacía. En WSL no aparecen `ttyUSB*`/`ttyACM*`. ADB vacío significa ningún teléfono disponible por depuración en este momento; no demuestra ausencia de un teléfono cercano capaz de usar BLE.

No se eligió un COM al azar ni se abrió/grabó un puerto Bluetooth. No se ejecutó upload/monitor/erase-all ni hubo pruebas activas en ninguna red. El coordinador ya tenía pendiente solicitar USB al usuario; no se duplicó esa pregunta.

Con placa identificada, comprobar COM único o pedir selección si hay varios y ejecutar desde la raíz en PowerShell:

```powershell
# Asignar $sentinelCom al COM real identificado; no usar COM3/COM4 Bluetooth.
$sentinelBuild = Join-Path $env:TEMP 'sentinel-arduino-firmware'
& $sentinelCli upload --fqbn sentinel:esp32:sentinel_s3_n16r8 --port $sentinelCom `
  --input-dir $sentinelBuild firmware/SentinelNode
& $sentinelCli monitor --port $sentinelCom --config baudrate=115200
```

Conservar la tabla única, receta dispersa en 0x0/0x8000/0x10000 y ausencia de `boot_app0.bin` en 0xe000. Nada de borrado total ni imágenes fusionadas sobre NVS. Las particiones y formatos conservados reducen riesgos, pero su continuidad física sigue pendiente de probar. Registrar device estable/boot nuevo, flash 16777216, PSRAM 8388608, Idle/OLED/botón, pairing SC/MITM, permisos/framing y recuperación con la app existente conforme a T01/T02/T03/T14/T15 de [Persona 1](../../../docs/guia-pruebas-persona-1.md). No registrar passkeys/credenciales en evidencia compartida.

## Escudo y extensiones de todos los módulos

La revisión del port conserva sin reinterpretar sus pruebas: fuentes de Escudo, API, lógica portable, tests y particiones idénticos por SHA256 en **36 archivos**; no hay aserciones debilitadas. Núcleo/cross-codec con ASan/UBSan PASS, **69 fixtures v2**. Escudo PASS para catálogo/selección, parser/callbacks/generaciones, scheduler/cobertura, cuatro detectores positivos/benignos/inconclusos, backpressure/stop/restart y journal/paginación/gap; recuperación cross-boot **6 únicos/6 repetidos**. Se leyeron los logs correspondientes, sin hueco o contradicción que justificara repetirlos.

P2 entrega `firmware/SentinelNode/src/sanitizador/` y factory `sentinel_sanitizador_module()`; P3 `src/cebo/` y `sentinel_cebo_module()`. Usan el mismo header `src/sentinel_api/include/module_api.hpp`, siete hooks, contexto/leases, serializador, transportes y journal únicos. Flags actuales P2/P3 en **0**; habilitar uno sin factory falla al enlazar. No hay segundo core/supervisor ni servicios ficticios anunciados.

Se revisaron **32 Markdown de ocho cambios activos**: Sanitizador auditoría/pasarela/DNS/DoH, Cebo HTTP/Telnet/eventos-historial e integración de tres modos. Todos contienen Arduino y rutas `SentinelNode/`; ninguno conserva rutas `firmware/components/` ni comandos `pio run`. Resultado `/tmp/sentinel-integration-active-proposals.json`. Sus servicios y pruebas siguen pendientes; este worker no alteró esos artefactos ni sus casillas.

## Documentación, enlaces y preservación

Se actualizaron 28 documentos bajo `docs/`: R-01/H-02 y stack/arquitectura/roadmap, firmware/desarrollo/migración, guías de pruebas y Persona 1/2/3/equipo, índice y las trece fichas de firmware. El [plan de migración](../../../docs/firmware-arduino-migration.md) ahora documenta el paquete real, comandos, hashes, resultado y siguientes comprobaciones; ya no ordena proponer un cambio inexistente ni instalar PlatformIO como requisito.

Los comandos PlatformIO restantes en docs son resultados históricos identificados (`firmware.md` y `core-foundation-check.md`); la comparación con PDF conserva el stack original y enlaza R-01 vigente. El mapa de movimiento conserva rutas antiguas solo como columna histórica. Los módulos futuros conservan directorios aún inexistentes como código/procedimiento, sin enlaces que aparenten entregas reales. La auditoría puntual de integridad con baseline `/tmp` se diferencia de los tests host diarios reproducibles.

La auditoría de Markdown comprueba enlaces locales de archivos y anchors en `docs/`, `openspec/` y `firmware/`. Se detectó un enlace al header API antiguo en el cierre archivado `sentinel-firmware-module-api`; el coordinador corrigió **solo su ruta** al quedar fuera de mi propiedad. No se modificaron estados/notas/casillas del archivo. Informes y comprobadores externos: `/tmp/check-arduino-links.py`, `/tmp/sentinel-integration-links.json`, `/tmp/sentinel-integration-anchors.json`.

Se contrastaron **10 hashes protegidos** de app v2 (archivo, spec consolidada, README de app y ficha) con `/tmp/arduino-migration-doc-preserve.json`: **ninguna diferencia**. No se tocó app/broker/contracts/notifications/simulator, fuentes firmware, SDK/scripts de otros autores ni OpenSpec tasks. La lista exacta de documentos de este worker está en `/tmp/sentinel-integration-modified-docs.json`; el resto del diff compartido conserva trabajo previo/otros autores. Sin commits, push, merge, publicación, eliminación de ramas o operaciones destructivas.

Comprobaciones finales de documentación: **191 Markdown**, **0 enlaces locales rotos**, **0 anchors inválidos**; `/tmp/sentinel-integration-link-check.log`, exit 0. `git diff --check` también terminó con exit 0. No se repitieron builds/hosttests amplios sin hueco o cambio real.
