# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-firmware-module-api`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Catálogo y objetivo reales por BLE; selección confirmada; ausencia de asociación/MQTT.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03 y H-03 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Usar el scan pasivo oficial ESP-IDF de 2.4 GHz, solicitado al gestor, sin conectar STA. Copiar al catálogo un máximo inicial de 32 BSSID, deduplicar por BSSID y conservar SSID/canal/RSSI/auth/cipher/PMF observado. Catálogo en RAM con scan_id y páginas v2; SSID oculto se muestra vacío, no inventado. La app puede volver a escanear. Selección exige BSSID/canal observados y SSID consistente; cambiar de objetivo ordena pausa/reconfiguración/reanudación por los hooks, cuya captura real se añade en el siguiente cambio. No editar App.tsx ni el codec. Alternativa de scan activo se descarta por el alcance pasivo. Botones remotos llaman a operaciones v2; inicio de captura se rechaza si su componente aún no existe.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/escudo/ (module, discovery, target y CMake)`, `firmware/test/escudo/`, `docs/modules/escudo-discovery.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Scan y BLE comparten radio → atender fin de scan por evento y mantener control BLE; no escanear en callback.
- Catálogo incompleto → límite y paginación visibles, nuevo scan explícito.

## Migration Plan

Trabajar en rama `feat/sentinel-escudo-descubrimiento`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
