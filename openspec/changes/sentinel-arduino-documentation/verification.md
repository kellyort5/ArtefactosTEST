# Verificación de sentinel-arduino-documentation

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

## Estado final de software

Documentación actualizada con core oficial3.3.12, wolfMQTT2.2.0, ESP_SSLClient3.1.3 y configuración mínima aprobada. El firmware completo final compiló Windows exit0; SDK fuente y Python propios del firmware retirados. Guía Persona1 lista desde Windows con instalación, cableado, opciones, Verificar/Subir/Monitor, appv2 y pruebas T01–T15 con expectativas reales. Los pendientes físicos y de módulos/pantallas futuros se conservan. No hay archivo de cambios ni operaciones Git de publicación.

El cuarto agente entregó la preparación; el coordinador completó la fase final tras fallar su reinicio por agent-update-prompt, sin reintentos ni reemplazar otros trabajadores. El seguimiento Orca task_c1f53a488174 registra esa recuperación. Las secciones iniciales siguientes conservan su momento y se complementan con el cierre final.

Fecha: 2026-11-13, America/Guatemala. Alcance: documentación independiente y auditoría semántica; el flujo oficial completo todavía espera dependencias. Ver [preparation.md](preparation.md) para archivos preparados, deltas exactos de specs no aplicados y relevo mínimo.

## Comprobaciones ejecutadas

Desde la raíz del repositorio, sin cambiar de rama ni modificar casillas:

```sh
openspec list --json
openspec status --change sentinel-arduino-documentation --json
openspec instructions apply --change sentinel-arduino-documentation --json
openspec validate sentinel-arduino-documentation --strict --json
openspec validate --specs --strict --json
```

Resultados: raíz local confirmada; cambio `ready`, esquema `spec-driven`, **0/4 casillas** conservadas. Validación del cambio **PASS** con `skip_specs:true` y cero deltas funcionales declarados. Specs principales **14/14 PASS** sintácticos, sin sustituir la auditoría semántica de preparation.md. Resultado JSON local: `/tmp/sentinel-documentation-phase1-specs.json`.

Se cotejó el inventario de capacidades archivadas y principales: **14/14 presentes**, ninguna spec archivada faltante. Los deltas activos no se copian como entregados; SDK propio y firmware-port anteriores no se sincronizan automáticamente.

Se comprobaron los enlaces locales Markdown y anclas de README.md, CONTRIBUTING.md, docs/** y los Markdown de este cambio con `/tmp/sentinel-documentation-check.py`. Resultado: **52 archivos Markdown, 689 enlaces locales y 39 anclas comprobados, cero errores**. El informe final de la fase está en `/tmp/sentinel-documentation-phase1-links.json`; no se comprobaron enlaces externos por red. Se conservó el ancla histórica al mapa del movimiento para no editar el informe histórico.

```sh
git diff --check -- README.md CONTRIBUTING.md docs openspec/changes/sentinel-arduino-documentation
```

Resultado: **PASS**, sin errores de espacios. El cotejo adicional contra `/tmp/sentinel-documentation-phase1-before.json` identifica **33 archivos existentes modificados** y sus hashes, más preparation.md/verification.md nuevos; la diferencia propia se conserva en `/tmp/sentinel-documentation-phase1.diff` y no confunde el trabajo previo del workspace compartido con cambios propios.

La búsqueda de instrucciones vigentes confirma que ya no se ordena instalar PIO ni ejecutar runners Python de firmware retirados. Se conservan referencias Python en evidencia histórica y herramientas del broker fuera de esta migración. Las guías describen el include distribuido y los comandos opcionales Node24 `test:firmware`/`test:escudo` publicados por limpieza. No se presentan como nuevos resultados ejecutados por este agente.

## Evidencia reutilizada sin repetir pruebas

[Legibilidad](../sentinel-firmware-readability/verification.md): supervisor de 23 líneas de coordinación, API/contratos conservados, headers privados de almacenamiento/tiempo, regresión host PASS y compilación de runtime con SDK previo, sin atribuirle enlace oficial.

[Limpieza](../sentinel-firmware-cleanup/verification.md): runners Node24, validador distribuido con mismo SHA256, casos C++/particiones preservados, ASan/UBSan, 69 fixtures v2, cuatro detectores y recuperación 6 únicos/6 repetidos. Sus guards/configuración esperan el relevo oficial; no se ejecutaron de nuevo desde documentación.

[Integración anterior](../sentinel-arduino-firmware-port/integration-verification.md): build Windows completo con SDK propio y apertura del sketch; Verificar visual y hardware pendientes. No demuestra el core oficial con nuevas bibliotecas.

## Límites

Este agente no ejecutó compilación/enlace oficial, instalación de bibliotecas/receta, upload/monitor, BLE/radio/OLED/NVS ni pruebas Android físicas. El coordinador comunicó compilación/enlace aislados de wolfMQTT2.2.0/ESP_SSLClient3.1.3 y aprobación de core oficial, tres teléfonos recordados y configuración mínima de nueve claves/recetas para proteger datos; se documentó esa decisión sin atribuirle build completo o instalación final. Cuatro vínculos antiguos deben conservarse y provocar fallo seguro antes de sobrescritura incompatible. El README del sketch sigue en propiedad de dependencias.

No se modificaron specs normativas, código, pruebas, contratos ni componentes externos. Tampoco tasks.md, evidencias históricas OpenSpec, archivos protegidos de baseline, instalaciones globales ni ramas. No hubo commits, push, merges, archivo de cambios o nuevas rondas de investigación/benchmarks.

La fase independiente queda preparada; la verificación final de instrucciones de instalación/build/subida y los deltas semánticos espera el relevo y revisión del coordinador. Este informe no declara completo el cambio de cuatro tareas ni sus pendientes físicos.

## Reconciliación de specs — coordinador

El seguimiento del agente de documentación no pudo iniciar por `agent-update-prompt` (`ctx_9028da556db7`); se liberó su terminal residual y el coordinador ejecutó únicamente esta reconciliación tras revisar preparation.md y las cinco specs completas. El informe inicial de preparación conserva su estado histórico.

Se sustituyó skip_specs:true por cinco deltas MODIFIED en este cambio, con proposal/design coherentes, sin implementar funciones nuevas. Se sincronizaron device-runtime (2 requisitos), device-connectivity (2), message-contracts (2), project-workspace (1) y mobile-control (1). Se conservó el nombre de todos los escenarios existentes, incluido «Uso de la base inicial», para evitar perderlos al archivar. BLE incorpora la aprobación explícita de tres teléfonos y un escenario de preservación/fallo seguro ante cuatro vínculos antiguos. Se aclaró que no se inician servicios todavía ausentes, sin prohibir módulos ya entregados.

Los Purpose de module-dispatch, escudo-discovery y app-module-protocol se corrigieron directamente conforme a las reglas de specs; no cambian requisitos ni acreditan hardware. Snapshot de reglas consultado antes de escribir: /tmp/sentinel-doc-sync-instructions.json. Contexto/rutas exactas: /tmp/sentinel-doc-sync-status.json. Copias previas de cinco specs en /tmp/sentinel-doc-spec-before-*.md. No se copiaron los deltas de capacidades futuras ni arduino-sdk. Sin archivo de cambios ni cierre de pruebas físicas.

Verificación: `openspec validate sentinel-arduino-documentation --strict` PASS antes de sincronizar; `openspec validate --specs --strict` 14/14 PASS tras sincronización. La revisión final de instrucciones Arduino y enlaces sigue pendiente del build y la limpieza.

## Revisión final de documentación — coordinador, en curso

Se concretó instalación Windows de core3.3.12, ESP_SSLClient3.1.3, wolfMQTT2.2.0 conforme al proveedor y la configuración de nueve claves. La guía P1 incorpora acceso desde Explorador/PowerShell, opciones exactas, Verificar/Subir/Monitor y distingue mantenimiento WSL opcional. Las referencias vigentes del validador usan .h; su rename conserva bytes/SHA según limpieza. Se corrigió reprovisionamiento Cebo activo: busy antes de tocar recursos/credenciales, pasar Idle confirmado y reintentar. Se documenta composición TLS real y espera asíncrona de liberación.

Los planes de firmware-port se alinearon con core oficial y validador distribuido; SDK propio queda identificado como antecedente sustituido en sus artefactos, conservando evidencia y casillas. La tarea pendiente de pasarela comprueba forwarding/NAPT en el core oficial con P1, sin exigir otro paquete. No se sincronizó arduino-sdk ni capacidades futuras.

`openspec validate --all --strict --json` PASS, 36/36 (22 cambios,14 specs). Informe /tmp/sentinel-documentation-final-validation.json. Revisión ampliada README/CONTRIBUTING/AGENTS/docs y deltas de este cambio: 58 archivos,701 enlaces locales,41 anclas, cero errores, /tmp/sentinel-documentation-final-links.json. No se hicieron llamadas de red para comprobar enlaces externos. `git diff --check` del alcance documental PASS. Estos resultados corresponden a este punto; todavía falta incorporar resultado final de compilación oficial y retirada del SDK, y comprobar coherencia final tras ese relevo.

### Preservación del trabajo externo

Se compararon por SHA256 los 249 archivos del snapshot protegido pertenecientes a app/, broker/, contracts/, notifications/, simulator/, package-lock.json y AGENTS.md: todos permanecen idénticos a antes de esta simplificación, incluido el trabajo local previo de app v2. Informe /tmp/sentinel-simplification-external-boundaries.json. package.json solo incorpora comandos opcionales de mantenimiento de firmware, sin cambiar dependencias/lockfile. Esta comparación acredita límites por archivo, no sustituye pruebas de esos componentes.

## Cierre final del coordinador

| Requisito del objetivo | Evidencia final y límite |
| --- | --- |
| Código modular, legible y comentado | Cambio sentinel-firmware-readability: coordinación, almacenamiento y UTC separados; ventana de detectores compartida; API pública de módulos idéntica y regresiones críticas PASS. La integración mínima de cierre asíncrono está documentada y probada. |
| Dependencias existentes y Arduino sencillo | Core oficial3.3.12 + wolfMQTT2.2.0 + ESP_SSLClient3.1.3; nueve claves de protección de datos y tres teléfonos aprobados. Firmware completo Windows exit0,1376726bytes imagen/160228globalRAM;730depfiles sin SDK Sentinel. No reconstrucción de SDK ni generadores/Python propios por Verify/Upload. |
| Firmware anterior y Python retirados | Un solo SentinelNode.ino, sin components/src antiguos, plataforma SDK fuente ni scripts Python de firmware; mantenimiento Node opcional. Se retiraron además caché y recomendaciones obsoletas PlatformIO, con respaldo del editor. |
| Documentación y specs coherentes | Arquitectura/R-01/C-05/H-02/roadmap/encargos y guías actualizados. Cinco deltas normativos sincronizados; compilación autónoma de dependencias añadida a project-workspace. Las14 capacidades archivadas ya tenían principal; no se copian capacidades futuras o SDK sustituido como entregados. Purpose heredados corregidos sin nuevas funciones. |
| Guía de Persona1 | docs/guia-pruebas-persona-1.md: preparación Windows, bibliotecas/opción local, hardware, Verificar/Subir/Monitor, appv2, T01–T15 y resultados esperados. P4/UI y módulos futuros conservan sus dependencias; no se presenta la UI común como panel completo del Escudo. |
| Preservación y pruebas |249 archivos externos, API pública y10 archivos de tests/particiones conservados. Núcleo/Escudo ASan/UBSan,69fixtures,11 casos de vínculos y17 casos de transporte/7transcripciones PASS. Certificados públicos y runner reproducible sin claves/proveedores/binarios en Git. |

Evidencia local de auditoría: /tmp/sentinel-simplification-final-audit.json, /tmp/sentinel-simplification-final-immutables.json y /tmp/sentinel-official-firmware-final-evidence.json. ELF final SHA25660d6e73cc23ac17520f0652d9995ea8dc20e63bd89ab2d4bd9c4f3dc5c2eb6ff. Después del build solo se corrigió el prerrequisito textual CMake del README; fuentes compiladas, configuración y bibliotecas siguen intactas.

Validación documental final:36/36 elementos OpenSpec estrictos (22 cambios,14 specs);59 archivos Markdown,709 enlaces locales y41 anclas sin errores. Informes /tmp/sentinel-documentation-final-validation.json y /tmp/sentinel-documentation-final-links.json. git diff --check PASS. Se preservan los informes de fases anteriores como antecedentes, sin atribuir sus conteos/pendientes al cierre actual.

Las pruebas de upload/monitor GUI, placa real, memoria/pines, OLED/botón, BLE/SC Android, coexistencia, MQTT/TLS sobre ESP32 y NVS/reinicio siguen pendientes. Son el siguiente recorrido de la guía; esta tarea de simplificación de software/documentación no las exige como condición de cierre ni las marca hechas. Tampoco se archivan cambios, eliminan ramas, hacen commits/push/merge o se modifica el alcance de los módulos de otros autores. HEAD y rama se conservan.
