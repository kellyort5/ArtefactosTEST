# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-sanitizador-dns`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Auditoría, NAT, DNS y DoH reales integrados; TLS obligatorio y fallo sin fallback plano.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08, H-04 y H-05 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Cliente DoH RFC8484 con POST application/dns-message usando esp_http_client/mbedTLS y endpoint HTTPS configurado por BLE. No fijar proveedor: el operador aporta endpoint autorizado y CA local; validar hostname/cadena/vigencia con hora válida. Timeout inicial 5 s y máximo 2 trabajos TLS simultáneos; respuesta binaria DNS hasta 4096 con ID/pregunta coherentes, tipo de contenido y status HTTP validados. Sin URL con usuario/token ni log de consultas completas de usuarios.

Integrar con DNS local/caché y métricas de latencia/errores; resolver destino DoH es arranque explícito del propio cliente, nunca reenviar consultas de clientes por DNS plano como fallback. Al fallar DoH, clientes reciben SERVFAIL; estado protección degradada, reintentos 1–30 s. Portal cautivo mantiene modo configuración sin etiqueta protegido. Probar un cliente con NAT+DNS+DoH y BLE en la placa, no asumir viabilidad por existir componentes. Alternativa de resolver desde un backend de PC altera el artefacto y se descarta como implementación de R-08.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sanitizador/ (doh_client y estado de protección)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-doh.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Carga TLS/NAT conjunta → comprobar flujo básico y limitar trabajos, no benchmarks previos.
- Arranque de confianza/hora → estado explícito; prohibido desactivar verificación.

## Migration Plan

Trabajar en rama `feat/sentinel-sanitizador-doh`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
