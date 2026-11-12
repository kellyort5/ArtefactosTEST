# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-escudo-descubrimiento`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Captura real integrada con parser robusto, control BLE y cobertura por canal.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, H-03 y H-06 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Solicitar modo promiscuo al gestor y filtrar frames relevantes. Callback copia cabecera y campos limitados a cola fija; parser y contador fuera del callback. Verificar longitudes, tipo/subtipo, elementos IE/RSN, BSSID/canal/RSSI antes de leer. Analizar Beacon, Deauth y Disassoc. Para la heurística Karma añadir metadatos de Probe Request/Response observados pasivamente, sin transmitir ni guardar payloads completos.

Scheduler inicial propuesto: priorizar 800 ms el canal objetivo y visitar otro canal permitido 200 ms; límites configurables internos, no promesa de cobertura continua. Canal de scan solo del país configurado. Registrar canal, tiempo de cobertura, pausas y descartes. Inicio/detención/cambio objetivo aplican parada/reinicio ordenados; no mezclar callbacks de una generación anterior. Alternativa de capturar solo el objetivo elimina comparaciones necesarias, por eso se mantiene recorrido. BLE concurrente primero; el ajuste de pausas está en recuperación.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/escudo/ (capture, frame_parser, channel_scheduler)`, `firmware/test/escudo/`, `docs/modules/escudo-capture.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Tramas truncadas → parser con límites y fixtures malformados.
- Una sola radio → scheduler y cobertura visibles; no garantía de observar todos los canales.

## Migration Plan

Trabajar en rama `feat/sentinel-escudo-captura`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
