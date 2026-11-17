# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../../docs/technical-decisions.md), [arquitectura](../../../../docs/architecture.md) y [encargos](../../../../docs/encargos/README.md).

Responsable único: **Persona 1 — Jefry: app común v2 por reasignación explícita del usuario**. Dependencias para integrar: `sentinel-module-contracts`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** App nueva mantiene base y acepta protocolo funcional, con navegación/control/historial comunes.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-04, R-06, R-07 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Reutilizar BLE/MQTT/controller/journal actuales y codec de contracts, sin reescribir transportes. Negotiación por Metadata/estado fresco, separar v1 base y v2, topics por versión y recepción de eventos históricos v1 intactos. No permitir ejecutar funciones ausentes. Secciones App de tres módulos en React y Zustand; páginas siguientes viven en app/src/features/<modulo>/ para aislar trabajo del mismo autor. El usuario reasignó exclusivamente esta spec común a Persona 1 el 2026-11-12. Persona 4 conserva los demás encargos de app/broker/laboratorio; no hay coautoría simultánea de esta spec.

Mantener journal IndexedDB256, dedup por ID y paginación con gap visible; no subirlo a nube ni crear base extra. Configuraciones secretas se envían por BLE autenticado y se borran del formulario/estado al terminar, sin localStorage. Comandos confirman accepted/applied/rejected; tras cambiar canal o modo reconciliar estado, no asumir éxito al hacer click. MQTT/WSS solo cuando modo/canal exista, sin intentar STA directa en Escudo. Propuesta v2 es dependencia revisada, no formato improvisado por la UI.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../../contracts/module-protocol.md) y [spec principal](../../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `app/src/ (controller, store, journal, transports y navegación)`, `app/test/`, `app/package.json`, `package.json`, `package-lock.json`, `app/README.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Retained y versiones mezcladas → leer estado vivo y pruebas de boot/revisión.
- Duplicados/reconexión → journal estable y fixtures comunes.

## Migration Plan

Trabajar en rama `feat/sentinel-app-protocolo-v2`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
