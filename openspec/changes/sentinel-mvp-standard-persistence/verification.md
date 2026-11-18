# Verificación — persistencia operativa y Arduino estándar

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## Estado

Implementación, regresiones host, documentación y build nativo Windows verificados. La inspección de artefactos y fuentes confirma el estado final del firmware. Los nueve trabajos software están implementados; la comprobación física permanece pendiente. No se ha programado una placa ni completado pendientes físicos por compilar. Cambio activo `sentinel-mvp-standard-persistence`, rama `feat/sentinel-arduino-migration`; sin commit, push, merge ni archivo.

Política autorizada: identidad, vínculos BLE, configuración aceptada e historial confirmado sobreviven al uso normal, reinicio y corte de alimentación. Reinstalación/actualización USB puede reinicializarlos sin migración entre versiones. La escritura que se interrumpe antes de terminar no se confirma como persistida.

## Matriz de cumplimiento

| Requisito | Evidencia actual | Resultado |
| --- | --- | --- |
| Tabla sin colisión boot_app0/NVS/BLE | CSV/binario validado por generador oficial, límites16MiB y roundtrip; boot_app0 de8192bytes contenido en otadata | PASS software y binario real del build |
| Capacidades conservadas | App4MiB, NVS24KiB, event_log256KiB; ring64eventos de hasta2048bytes | PASS configuración/código; memoria física pendiente |
| Carga/exportación estándar | Cuatro claves locales; propiedades efectivas oficiales incluyen upload, merged y boot_app0 en0xe000 | PASS recetas y exportación reales |
| Preparación repetible | Instalador9→4, cinco claves antiguas retiradas, claves/comentarios ajenos, respaldo exacto, WhatIf, no-op/timestamps y bloqueo Windows | PASS |
| Persistencia normal de identidad/BLE/historial | Wrapper NVS, identidad con commit, preflight real nimble_bond y journal con cursor tras confirmación; fuentes previas preservadas | PASS revisión y regresión; reinicio/corte físicos pendientes |
| Configuración Wi-Fi/MQTT | Helper NVS y Network reales; validación, restore, ausencia/corrupción/fallos, RAM anterior ante guardado fallido, Idle/Escudo/UTC y limpieza de copias | PASS host ASan/UBSan; NVS/radio físicos pendientes |
| Objetivo Escudo | Helper y módulo reales, selección/commit, retorno de modo/nueva instancia, fallos y captura segura, observaciones temporales nuevas | PASS host ASan/UBSan; placa pendiente |
| Privacidad y alcance | Sin cambios wire, contratos/app intactos, secretos fuera de mensajes/Git; NVS sin promesa de cifrado físico; módulos/roles conservados | PASS revisión |
| Documentación y equipo | Política, arquitectura/R06/H07/roadmap, cuatro guías y encargos, primera carga limpia, regla oficial primero y pruebas P1 | PASS; evidencia física no inventada |
| Especificaciones | Cuatro deltas sincronizados conservando escenarios existentes; OpenSpec38/38 estricto | PASS |
| Firmware completo Windows | Core oficial3.3.12/IDF5.5.5, fuentes finales congeladas, artefactos y recetas efectivos | PASS exit0, fuentes/artefactos/enlace verificados |

## Arduino y protección de arranque

Tabla aprobada: otadata0xe000/0x2000, factory0x10000/0x400000, nvs0x410000/0x6000 y event_log0x416000/0x40000. No hay partición PHY separada porque el SDK fijado incorpora initdata a la aplicación; calibración permanece en NVS. otadata reserva el destino de boot_app0, sin añadir OTA funcional. Primera carga de esta distribución limpia desde Arduino; los datos anteriores no se migran.

Las cuatro claves restantes sirven para etiqueta/selector, `build.partitions`, máximo de imagen4MiB y `--wrap=nvs_flash_init`. El wrapper conserva el primer error de NVS y evita que el core la borre antes de setup ante NO_FREE_PAGES/NEW_VERSION_FOUND; protege reinicios normales, no continuidad entre versiones. NVS propia de la aplicación no protegería automáticamente los bonds internos.

Verificaciones del worker `ctx_18f48ab17c76`, revisadas por el coordinador:

- PASS `node firmware/test/arduino_build_test.mjs`: factories reales y guardas de core/SDK/partición/MQTT/TLS conservadas.
- PASS `node firmware/test/check_config.mjs --sdkconfig <oficial qio_opi/include/sdkconfig.h> --partition-tool <oficial tools/gen_esp32part.py>`: tabla exacta, capacidades, parseo binario, verify_size_fits, roundtrip y contención boot_app0.
- PASS `firmware/test/installers_test.ps1` en Windows con ZIP original en caché: actualización9→4, claves ajenas/comentarios/duplicados, respaldo exacto, WhatIf, idempotencia con timestamps, bloqueo de archivo y cobertura wolfMQTT existente. Sin descargas nuevas.
- Instalador aplicado al boards.local.txt global del core3.3.12 con respaldo y repetición sin cambios. Propiedades expandidas oficiales confirman cuatro claves y recetas estándar; no se modificaron fuentes/binarios del core ni SDK.

Reporte temporal: `/tmp/sentinel-mvp-platform-report.txt`; propiedades: `/tmp/sentinel-mvp-platform-properties.txt`.

## Configuración y recuperación

`connectivity_config.cpp` usa una entrada NVS completa y acotada; valida estructura, formato, UTF-8/límites, URI segura y CA. `Network::configure` confirma guardado/commit antes de sustituir configuración RAM. Fallo de guardado devuelve resource_error sin tocar la configuración/radio previos; `configuration_error` aparece en estado y OLED. Al iniciar restaura en Idle sin asociarse ni recuperar una hora UTC anterior. No se prometen rollback físico de todos los fallos eléctricos ni cifrado de flash. Se permiten credenciales propias autorizadas; nunca credenciales recibidas por Cebo, logs, eventos, estado público o Git.

`escudo_target.cpp` usa un registro51bytes en namespace propio del NVS central. Conserva SSID/BSSID/canal y contexto de seguridad de selección, sin RSSI, catálogo, uptime, generación, captura o evidencia temporal. Selección comprueba radio y commit antes del éxito. Si falla NVS conserva el objetivo anterior y detiene/invalida la captura del candidato rechazado; una parada fallida se expone y se reintenta bloqueando nuevos arranques hasta liberar.

Al volver a Escudo desde Idle o tras reinicio, recupera objetivo con captura detenida; no explora ni se conecta automáticamente. Evil Twin espera una observación nueva del objetivo para su comparación temporal. Deauth/Disconnect/Karma mantienen identidad de selección pero requieren evidencia nueva. No se modificaron detectores ni sus casos de prueba existentes.

La identidad continúa en NVS y se deriva de MAC al inicializar; borrar por USB no garantiza que cambie su valor. El boot_id sí es nuevo por arranque. BLE inspecciona el namespace real nimble_bond sin escribir. Journal confirma cursor después del Store, recupera registros validados y muestra degradación/gap sin formato automático. T03/T11 de Persona1 comprueban objetivo recuperado antes de volver a explorar o seleccionar.

## Pruebas y límites

Worker conectividad `ctx_4f56b5f64d2f`:

- PASS `npm run test:firmware` y `SENTINEL_SANITIZE=1 npm run test:firmware` con Node24.
- `connectivity_test.cpp` enlaza helper y Network de producción con dobles NVS/SDK: save/restore, ausencia, tamaños/versión/contenido inválidos, open/read/write/commit fallidos, configuración anterior, Idle/Escudo, UTC y borrado RAM.
- Parser CA host con OpenSSL ya existente; no sustituye mbedTLS real ni pruebas físicas.

Worker objetivo `ctx_0cbf0773da35`:

- PASS final `SENTINEL_SANITIZE=1 npm run test:escudo`: discovery/capture/detectors/recovery y target_persistence_test con helper/módulo/supervisor reales y dobles de frontera.
- PASS197 mensajes codec/ACK y19 ejemplos; recuperación6eventos únicos/6repeticiones, gap y paginación. PASS guards Arduino y trece fuentes C++/headers anteriores inmutables.
- Una aserción nueva confundía BSSID de catálogo con evento de objetivo; se corrigió la prueba nueva y la suite final pasó, sin debilitar pruebas previas.

Los dobles NVS verifican lógica y manejo de resultados, no resistencia física ni transacciones ante todos los cortes. ASan/UBSan sin hallazgos en suites finales. Sin benchmarks ni campañas prolongadas. Reporte temporal de Escudo: `/tmp/sentinel-escudo-persistence-report.txt`.

## Documentación, contratos y Git

Arquitectura/decisiones/roadmap, guías P1-P4 y planes activos mantienen alcance y autores de los tres módulos. P2/P3 incorporarán configuración persistente en sus entregas pendientes; P4 contempla reinicialización USB incluso si device_id sigue igual. Se preservan resultados, hashes y casillas físicas de los informes anteriores como históricos. Se aclaró la tarea futura de upstream persistente y la evidencia histórica de nueve claves sin marcar pendientes como terminados.

Cuatro deltas sincronizados: device-connectivity, event-journal, project-workspace y escudo-discovery. `openspec validate --all --strict --json`: PASS38/38,24 cambios y14 specs. Auditoría de111 documentos fuente modificados/nuevos y875 enlaces locales: cero enlaces nuevos rotos; cuatro referencias históricas preexistentes a destinos del paquete SDK retirado identificadas sin fabricar reemplazos. Registro `/tmp/sentinel-mvp-links-final.json`.

Comparación contra `/tmp/sentinel-mvp-usb-before.json` y su ZIP: contratos, app y trece fuentes críticas de pruebas previas intactos; ningún archivo que existía al iniciar este cambio eliminado. `git diff --check` PASS. Rama feat/sentinel-arduino-migration, HEAD af7200ab1047f2d0dfc09b64047dfec6a3ce3309 sin cambios de historial/publicación.53 fuentes/configuración congeladas en `/tmp/sentinel-mvp-root-freeze.json` para cotejar el build.

## Comprobaciones físicas pendientes

Seguir [guía Persona1](../../../docs/guia-pruebas-persona-1.md) y [política de carga](../../../docs/persistence-policy.md):

1. Primera carga limpia de tabla nueva desde Arduino, con puerto real identificado; pérdida USB aceptada, nuevo emparejamiento/configuración.
2. Configurar conectividad y seleccionar objetivo por BLE protegido, confirmar guardado y producir eventos reales.
3. Reiniciar sin subir: misma identidad, bonds, configuración/historial confirmados, boot nuevo e Idle sin asociación ni UTC viejo.
4. Entrar Escudo y comprobar objetivo guardado/captura detenida/catálogo vacío antes de explorar/reseleccionar. Iniciar captura explícitamente.
5. Cortar/restaurar alimentación después de confirmar escritura y repetir recuperación; no exigir datos de escrituras que no terminaron.
6. Reconectar BLE/MQTT, recuperar eventos y deduplicar sin ejecutar comandos antiguos.
7. Actualización/reinstalación USB puede reinicializar; reconfigurar/emparejar y reconciliar cursores/caché si ocurre.

Estos pasos no se completan con stubs ni build. Tampoco se cierran los pendientes físicos previos ni se declara entregado el MVP completo de los tres módulos por esta adaptación de infraestructura.

## Build integrado Windows — compilación PASS

El CLI nativo del IDE terminó exit0 el2026-11-16T16:19:18Z, con compilación limpia iniciada a16:09:26Z, FQBN `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`. Core oficial3.3.12/IDF5.5.5, wolfMQTT2.2.0 y ESP_SSLClient3.1.3. Programa1381054/4194304bytes y RAM global160228/327680bytes; no son mediciones de memoria física.

Salida temporal `%TEMP%/sentinel-mvp-standard-firmware/20261116T160906Z`: bin1381200bytes, bootloader19968bytes, particiones3072bytes y merged16777216bytes, además de ELF/map. La compilación usó una copia congelada idéntica al sketch actual por contenido y conjunto de rutas.53 entradas de producción,51 de staging y379 de dependencias son idénticas antes/después; coinciden con el freeze independiente del coordinador. No se cargó ninguna imagen.

### Inspección final y reproducibilidad

El [informe nativo completo](native-build-verification.md) conserva comandos, entorno, hashes y límites. El coordinador inspeccionó además los archivos de evidencia temporales `integration-checks.json`, `elf-integration-checks.json`, `global-board-check.json`, `stock-recipes-check.json` y `library-provenance.json`.

- Tabla binaria real idéntica al CSV actual con parseo/verify/roundtrip oficial y límite16MiB; regiones merged idénticas a los cuatro binarios en0x0/0x8000/0xe000/0x10000. NVS/event_log son relleno0xff en merged, coherente con imagen de inicialización y pérdida USB permitida.
- Diez recetas efectivas coinciden con stock; cuatro claves globales correctas y cinco overrides ausentes. Flash_args y boot_app0 exportados por las recetas oficiales, sin hooks propios.
- ELF contiene load/save de conectividad y objetivo, NVS real y wrapper. Desensamblado de initArduino: dos llamadas al wrapper, cero llamadas directas no protegidas; wrapper llama NVS real.
- 734 depfiles revisados, cero referencias al SDK Sentinel; SDK/core/bibliotecas oficiales fijados y customBuildProperties vacío.25 objetos del sketch,312 de bibliotecas y60 de core compilados realmente.
- Manifest de artefactos en informe nativo: ELF SHA256 `4128fdb47228b14875b4b9ee4b517bd9b158c123789e6dd70c610d6f046f2c92`, app `129294e22d0aef5b72a357fd6caba17101e09efc52bc41624316ce1b71e7b5b3`, tabla `e1908d6b41dfecdfa4df3818adcca7476f17d9a06566e44502422ea2f12f2bc9`.

La opción QIO80MHz se refleja en build.boot=qio y memoria qio_opi; el core usa flash-mode dio en flash_args como parte de su receta oficial. No se alteró para forzar una etiqueta distinta.

Este resultado cierra la implementación de esta política y su integración de software; no cierra otras specs de módulos ni pendientes de placa/teléfono. Ninguna publicación o archivo de cambios está autorizado por este informe.
