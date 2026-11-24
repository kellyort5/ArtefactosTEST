# Verificación de sentinel-firmware-readability

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha: 2026-11-13, America/Guatemala. Rama compartida: `feat/sentinel-arduino-migration`.

## Resultado y alcance

Refactorización implementada sin cambiar las API públicas, factories, contratos, detectores, control BLE ni formato del journal. Se conservan los módulos futuros y la llamada interna `sentinel_arduino_nvs_status()`, cuya adaptación pertenece al responsable de dependencias. No se modificaron pruebas, aserciones, fixtures, generador ni `v2_validator.inc` para conseguir los resultados.

El supervisor pasó de 251 a 23 líneas de coordinación. Las instrucciones extraídas siguen en funciones concretas, con el mismo orden de ejecución, límites por vuelta, cadencias y tratamiento de sesiones caducadas. El archivo runtime completo pasa de 916 a 885 líneas; los nuevos headers privados contienen 48 y 33 líneas. La mejora consiste en separar responsabilidades, no en eliminar funcionalidad.

## Mapa de responsabilidades

| Archivo o función | Responsabilidad y límite |
| --- | --- |
| `runtime.cpp`: `submit`, `busy`, `CoreContext` | Copiar solicitudes y salidas en estructuras acotadas; callbacks sin acceso al gate ni escritura del journal. Se conservan los locks y contadores existentes. |
| `runtime.cpp`: `process_busy_result`, `process_request`, `handle`, `handle_signal` | Serializar admisión, sesiones, deduplicación, comandos, botón y señales de transporte. Una sesión BLE obsoleta sigue omitiendo el resto de esa vuelta. |
| `runtime.cpp`: `complete_transitions` | Confirmar la transición de botón u orden después del supervisor de módulos; conservar ACK, revisión y evento originales. |
| `runtime.cpp`: `deliver_module_output`, `synchronize_network` | Persistir antes de entregar eventos; reintentar leyendo el JSON original, un registro por vuelta, con identidad y QoS existentes. |
| `runtime.cpp`: `current_state_payload`, `publish_state`, `publish_system_metrics`, `refresh_screen` | Construir estado una sola vez por ruta de envío y separar métricas/pantalla de la coordinación. Estado MQTT conserva QoS 1 y retained; métricas QoS 0 sin retained. |
| `runtime_storage.h` — nuevo, privado | Adaptador `NvsStore` extraído: misma partición `event_log`, namespace `ring`, claves `slot%02u`, errores y commit. Sin borrado automático de NVS. |
| `runtime_time.h` — nuevo, privado | Conversión UTC validado a `timeval`; conserva calendario, tratamiento de años bisiestos y truncamiento de fracciones a microsegundos. No valida ni aplica por sí solo una orden. |
| `core.cpp`, `digest.cpp`, `wire.cpp`, `protocol_v2.cpp` | Nombres privados más claros y comentarios sobre validación del journal, orden del gate, borrado de secretos, propiedad JSON y validación de salida. |
| `module_runtime.cpp` y header | Documentar propiedad de radio durante fallos de arranque/parada y entrega de salidas sin espera; conservar algoritmo y límites. |
| `escudo/detectors.cpp` y header | Compartir la expiración de ventanas mediante `Window::prune`, sin muestras ficticias ni cambios de umbral, cooldown, correlación o evidencia. |
| Escudo: captura y módulo | Explicar generaciones de callback, SSID como bytes, insuficiencia de evidencia pasiva y motivos de aislamiento al parar. El transporte no modifica la lógica del Escudo. |
| Fuentes y headers portables afectados | Includes relativos para resolver código del proyecto sin los flags `-I` del paquete SDK propio. |

## Regresión ejecutada

Comandos desde la raíz del repositorio con Node 24.13.1 en `PATH`:

```sh
SENTINEL_SANITIZE=1 node firmware/test/check_contracts.mjs
SENTINEL_SANITIZE=1 node firmware/test/escudo/check_contracts.mjs
```

Ambos terminaron con código 0, ASan/UBSan y sin diagnósticos de sanitizadores.

| Comprobación | Resultado y evidencia local |
| --- | --- |
| Núcleo: gate/despacho v2, leases exclusivos, timeout, colas, framing, botón, recuperación/sobrescritura/fallo del journal | PASS; `/tmp/sentinel-readability-core.log`. |
| Serializador C++ v1/v2 frente al codec AJV; comandos compartidos; fixtures v2 | PASS, 69 fixtures v2; mismo log. |
| Escudo: catálogo, selección, scan, presión de cola, reinicio, generaciones, parser, scheduler, comandos BLE y cobertura | PASS; `/tmp/sentinel-readability-escudo.log`. |
| Cuatro detectores: umbrales, expiración, cooldown, retransmisiones, generaciones, duplicados benignos, seguridad/canal/RSSI y Karma pasivo inconcluso/correlacionado | PASS; mismo log. |
| Recuperación entre procesos/arranques, páginas completas, gap, codec y ACK correlacionados | PASS, seis eventos únicos y seis reentregas; mismo log. |
| Conversión UTC del header real, comparada con `timegm`: época, fechas anteriores a 1970, siglos bisiestos y fracciones | PASS, 45 casos con ASan/UBSan; `/tmp/sentinel-readability-time.log`, fuente `/tmp/sentinel-readability-time-test.cpp`. |

Los resultados conservan el baseline inmediato descrito por el coordinador: núcleo con 69 fixtures v2 y Escudo con cuatro detectores y recuperación 6/6. No se retiró ningún caso ni se debilitaron expectativas.

## Compilación real del runtime y revisión de diferencias

Se compiló **la fuente actual** `runtime.cpp`, incluyendo ambos headers privados, mediante el g++ Xtensa Windows instalado. `/tmp/compile-sentinel-runtime.cjs` reutiliza los argumentos reales del `compile_commands.json` de `%TEMP%/sentinel-arduino-firmware`, sustituye la fuente por la del repositorio y escribe únicamente `%TEMP%/sentinel-readability-runtime.o` y su dependencia. Se eliminaron los includes adicionales de `generated`, `sentinel/include`, `sentinel_api/include` y `escudo/include`; la unidad resuelve sus headers por rutas relativas. Resultado: código 0 en `/tmp/sentinel-readability-runtime-compile.log`.

Esta comprobación usa los headers del **SDK previo instalado**, sin mocks ESP ni enlace. Acredita la compilación de la unidad afectada; no acredita enlace, ejecución de FreeRTOS/NVS ni compatibilidad final con el core oficial. El build completo Windows con plataforma oficial corresponde al responsable de dependencias y no se ejecutó desde este cambio.

Se cotejaron once bloques de instrucciones extraídas contra la copia anterior, ignorando únicamente comentarios/formato y aplicando las sustituciones explícitas de extracción. Incluye admisión/ACK de saturación, señales, liberación de slots, transiciones, salida de módulos, replay, métricas, pantalla, NVS, manejo de órdenes y UTC: PASS en `/tmp/sentinel-readability-equivalence.log`; script `/tmp/check-sentinel-readability.cjs`. La condición que usaba `continue` en el supervisor ahora retorna `false` desde su helper y conserva el `continue` en el bucle.

Las diferencias se revisaron contra `/tmp/sentinel-simplification-before.zip`; manifiesto de 19 archivos modificados y dos nuevos, con SHA256 anterior/actual: `/tmp/sentinel-readability-files.json`. Copias de revisión limitadas a estos archivos: `/tmp/sentinel-readability-diff/`. El archivo protegido y su JSON originales permanecen intactos. `git diff --check` terminó correctamente; no se realizaron commits, push, merge, cambios de rama ni edición de `tasks.md`.

El validador local conserva SHA256 `9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68`. Se mantiene su include local; Arduino no requiere ejecutar un generador para compilarlo.

## Límites y entrega

Las fuentes están estables y se comunicaron los dos headers nuevos al coordinador y al responsable de dependencias. Quedan bajo esa integración el build/enlace completo con Arduino oficial y la sustitución segura del hook NVS. Este cambio no declara esas comprobaciones terminadas.

No hubo placa identificada para grabar. Captura Wi-Fi, BLE, OLED, MQTT y persistencia NVS físicos no se ejecutaron; los tests host no acreditan esos comportamientos físicos. No se borraron flash, NVS ni instalaciones globales y no se realizaron pruebas activas de red.

## Integración de parada asíncrona — coordinador

El seguimiento al autor no pudo arrancar por `agent-update-prompt`; se liberó su terminal fallido antes de editar. El coordinador implementó el ajuste acotado autorizado de task1.4: `Resources::network_stopping()` devuelve false por defecto; `ModuleSupervisor::finish()` espera únicamente cuando la parada devuelve false con pendiente explícito. Conserva perfil/lease y deadline5000ms; los fallos reales y una liberación inconsistente siguen generando resource_error. No cambia Module API ni contratos externos.

Se agregó `firmware/test/network_stop_test.cpp` como ejecutable independiente. Limpieza lo registró en el runner común sin alterar los nueve casos C++ previos. Cubre exclusión antes del ACK, espera y cierre, deadline exacto, ACK tardío sin aplicación automática, fallo real durante espera, resultado inconsistente, recuperación Idle y adaptador previo con defaultfalse.

`SENTINEL_SANITIZE=1 npm run test:firmware` terminó con salida0, incluida esta prueba y las anteriores, con69fixtures v2 y cross-codec v1/v2 PASS. Evidencia `/tmp/sentinel-cleanup-network-stop-core.log`. Esto prueba la lógica real del supervisor con recursos controlados en host; el worker MQTT y el firmware completo oficial se verifican por dependencias. Sin pruebas físicas ni borrado NVS.
