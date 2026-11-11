# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-app-protocolo-v2`, `sentinel-cebo-eventos-historial`, `sentinel-alertas-webhook-telegram`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Cebo real visible por MQTT, control confirmado, historial y alertas internas/estado externo.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-04, R-06, R-10 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Usar transporte WSS con CA/identidad y ACL, UI de servicios HTTP/Telnet, start/stop con ACK y métricas de cantidades/frecuencias/sospecha. Tabla de incidentes con IP/puertos/protocolo/tipo/tiempo-calidad, gráficos e historial IndexedDB común. Sin campos para ver contraseñas capturadas. Recuperación por cursor del nodo y filtros locales, dedup por ID y gap de retención visibles. Alertas dentro de app para nuevos incidentes. Configuración local autorizada de URL HTTPS/token de componente de notificaciones, no Firebase/Telegram secret keys en frontend; consulta de status protegido muestra webhook/Telegram/push configurado/última entrega/error, sin afirmar lectura. La recepción push móvil se entrega en cambio separado del mismo autor.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `app/src/features/cebo/`, `app/src/ (registro de ruta y store Cebo)`, `app/test/cebo*`, `docs/modules/app-cebo.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Historial retenido limitado → gap visible, no historial infinito.
- Estado backend/nodo diferente → presentación separada sin falsos eventos del ESP32.

## Migration Plan

Trabajar en rama `feat/sentinel-app-cebo`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
