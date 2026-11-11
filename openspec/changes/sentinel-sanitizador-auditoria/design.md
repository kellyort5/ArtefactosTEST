# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-firmware-module-api`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08, R-09 y H-05 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Registrar el módulo Sanitizador usando sentinel_api. Configuración upstream por BLE autenticado en RAM; pedir STA al gestor sin SoftAP todavía. Auditoría asíncrona con timeout inicial 5 s por prueba, resultado por job_id y cancelación al salir. Usar resolución DNS de ESP-IDF y esp_http_client con mbedTLS: cadena/hostname/vigencia, sin deshabilitar verificación para conseguir un ok. Hora desconocida da inconcluso/error de tiempo.

Prueba de portal usa URL HTTP controlada por el equipo y respuesta esperada; redirección/respuesta alterada es indicio de portal, no MITM confirmado. No introducir login automático ni seguir formularios. URLs HTTPS/portal y nombre DNS se configuran como destinos propios/autorizados; no guardar cuerpo/cabeceras. Comparaciones DNS pueden ser diferencias legítimas: conservar evidencia y explicación. Alternativa de diagnosticar solo ping no satisface DNS/certificados/portal.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sanitizador/ (module, audit y CMake)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-audit.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Portal y DNS divididos → resultado inconcluso y límites explícitos.
- Credenciales y TLS → solo BLE, RAM, sin logs; no bypass de certificados.

## Migration Plan

Trabajar en rama `feat/sentinel-sanitizador-auditoria`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
