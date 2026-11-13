# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../../docs/technical-decisions.md), [arquitectura](../../../../docs/architecture.md) y [encargos](../../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-escudo-detectores`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Evento recuperable tras desconexión y reinicio, huecos visibles y funciones conservadas.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-06, H-03 y H-07 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Reutilizar journal NVS circular de 64 eventos y cursor persistente de la base; no crear otro almacén. Eventos pasan por emit_event central, recuperables mediante read_events por BLE. Duplicado se identifica por message_id/cursores, no por texto. Si se sobrescribe el anillo, exponer gap, rango disponible y contador de pérdida.

Primero ejecutar captura/BLE concurrentes. Si durante la comprobación breve aparece fallo que impide control o sincronización, aplicar una pausa de captura acotada para transferir página y reanudar con indicador de cobertura. No retirar detectores, selección ni recorrido. No diseñar benchmark comparativo. Alternativa de guardar solo en RAM pierde continuidad de R-06 y se descarta. Nodo no necesita Internet para capturar, almacenar, OLED o botón.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../../contracts/module-protocol.md) y [spec principal](../../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/escudo/ (coverage, reconnect y backlog)`, `firmware/test/escudo/`, `docs/modules/escudo-recovery.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Capacidad finita → sobrescritura visible y no prometer historial ilimitado.
- Contención BLE/captura → pausa solo si falla funcionalmente y registrada en cobertura.

## Migration Plan

Trabajar en rama `feat/sentinel-escudo-recuperacion`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.

## Concreción aprobada para la implementación

El usuario autorizó ampliar este cambio a `firmware/components/sentinel/include/module_runtime.h`, `module_runtime.cpp` y `runtime.cpp` para extraer la rutina existente de serialización/persistencia de eventos a un helper interno compartido por firmware y pruebas. Se conserva el journal NVS, contratos, supervisor, transportes y orden persistir-antes-de-entregar. No se crea un segundo almacenamiento ni se cambia la API pública de módulos. La ampliación permite probar el camino de producción del núcleo, propiedad de Persona 1, dentro de las tareas 1.1/1.2; sustituye únicamente la restricción de carpetas correspondiente de esta planificación.

El usuario autorizó continuar con software conservando pruebas físicas pendientes. Las tareas 1.3, 2.1 y 2.2 no se completan con fixtures. La concurrencia se mantiene como primera opción; no se añade pausa automática sin un fallo físico bloqueante observado. Ver [guía de recuperación](../../../../docs/modules/escudo-recovery.md).
