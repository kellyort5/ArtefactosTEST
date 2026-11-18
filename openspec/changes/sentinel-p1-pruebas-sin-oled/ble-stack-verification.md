# Correcciones de arranque y pila BLE P1

> Evidencia de la entrega P1 anterior a la integración MQTT anónima. Conservar resultados/pendientes; para firmware/APK actual consultar [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md).

Fecha: 2026-11-17. Rama `feat/sentinel-p1-serial`; integración con las ramas compartidas mediante PR y revisión pendiente. Se conserva el trabajo MQTT LAN del checkout original.

## Evidencia física y causa

La primera imagen serie arrancaba sin OLED pero informó `BLE_INIT: Malloc failed`. Su BSS interna contenía la caché de32 comandos (`CommandGate`,58632bytes). Se trasladó esa misma caché a PSRAM con `heap_caps_malloc` oficial y placement new; se comprueba el fallo de reserva y de creación de tareas BLE/supervisor. No se cambió SDK/core, particiones, NVS, contratos, capacidad de caché ni seguridad BLE. [Asignación oficial por capacidades](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/api-reference/system/mem_alloc.html).

La imagen intermedia `export-memory` se cargó en COM5 con hash verificado y se comprobó su arranque. RAM interna libre/mayor bloque: antes de red222224/163828; antes de BLE170792/126964; después de BLE67016/31732; después de supervisor40344/24564. Se observaron estado Idle y contador de captura0. El usuario confirmó después que el botón abrió ventana y recibió el código temporal; nunca se registra el código en la evidencia.

Al continuar desde Android apareció un reinicio. El backtrace coincidió con el ELF intermedio SHA-256 `2c0ad90734ebdaa61cac0e2471cff8f51f7e79e8e7a18d4c3242b21eb6e480f2` y se decodificó con `xtensa-esp-elf-addr2line` oficial: `digest_command` (`digest.cpp:50`) <- `submit` (`runtime.cpp:269`) <- acceso GATT (`ble.cpp:131`) <- NimBLE. Era la recepción de un comando protegido, después de introducir el código. Los errores posteriores de core dump son del intento fallido de guardar el diagnóstico; no justifican añadir particiones ni borrar NVS.

Los marcos observados del camino de aplicación consumían4752bytes (digest2096 + submit2096 + access560), además de las llamadas SDK, en una pila host NimBLE de5120bytes. El código EXCCAUSE1 y la cadena se corresponden con agotamiento de esa pila.

## Corrección final

El buffer canónico de2049bytes se reserva mediante `cJSON_malloc`, siempre se limpia antes de `cJSON_free` y se rechaza el cálculo si falta memoria, liberando la copia JSON. Las respuestas grandes de callbacks de admisión/cola llena se reservan con `new(std::nothrow)` y dueño RAII, copiándose a la cola acotada antes de liberarse. No se cambia el digest, autenticación, gate o deduplicación; no se amplía/recompila la pila del SDK.

## Verificación de software final

- `npm run test:firmware`: PASS. Nueva regresión: fallo de asignación del buffer, ausencia de fugas cJSON, digest idéntico tras recuperar memoria, comando original intacto y buffer completamente limpio antes de liberar. También gate, botón, supervisor, journal, bonds, NVS/conectividad y69 fixtures v2 con AJV.
- `SENTINEL_SANITIZE=1 npm run test:firmware`: PASS con AddressSanitizer/UBSan.
- Build Windows Arduino CLI/core3.3.12 con opción serie1: exit0, imagen1395566/4194304bytes, globales101828/327680bytes. Reducción estática frente al perfil serie inicial:58624bytes. El heap real de la versión final requiere nueva lectura física.
- Auditoría de los tres archivos reales con el compilador oficial y `-fstack-usage`: access560, digest32, submit272, busy464bytes, todos estáticos. El camino normal de aplicación baja de4752 a864bytes; incluyendo busy suma1328bytes. Las cifras no incluyen los marcos internos del SDK ni constituyen una prueba de estrés de radio.
- `node firmware/test/check_ble_stack.mjs <directorio .su>` comprueba los límites por función y un máximo conjunto de2048bytes para estos marcos de aplicación, dejando margen en los5120 del host. Informes nativos locales en `%TEMP%\sentinel-p1-serial-check\stack-audit`.
- Imagen final SHA-256 `8bf1d9b028ab8a893405922890435673f37c6dc0809823ce834233b18bbcc781`. ELF final SHA-256 `058ea7653570bdce7b6df2e1c68487b659808a52f09de241ae51d8566adf6150`.

## Entrega local y pendiente físico

El usuario retiró la ESP32. La imagen final NO se ha cargado ni se acredita pairing completo. Queda comprobar carga, ventana/código, estado/ACK aplicado y ausencia de reinicios en Android; OLED, radio y persistencia conservan sus pruebas pendientes.

Paquete local ignorado por Git: `C:\Users\Jefry\Documents\Arduino\ArtefactosProyecto\.cache\sentinel-p1-ble-fix`. Incluye sketch completo con opción1, imagen/bootloader/particiones y ELF finales, manifiesto SHA-256, `LEEME.txt` y `cargar.ps1 -Port COM5` (puerto explícito, hashes previos y receta de carga oficial, sin recompilar). Su creación no programa una placa. El APK diagnóstico P1 anterior sigue siendo compatible.

La fuente mantenida conserva el default0; la copia temporal Windows y el paquete usan1. `export-serial`, `export-default` y `export-memory` son versiones anteriores y no contienen la corrección de pila. La guía inicial y su build no acreditaban el funcionamiento físico; esta nota conserva esa distinción.
