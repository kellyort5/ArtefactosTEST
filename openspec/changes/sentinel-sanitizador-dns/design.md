# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-sanitizador-pasarela`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** DNS local real, reglas/caché probadas y ningún fallback DNS plano; DoH aún explícitamente pendiente.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08 y H-05 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Servidor DNS local en UDP53 y manejo TCP53 cuando el cliente lo requiere, limitado a interfaz/clientes SoftAP. Parser DNS con validación de longitudes, compresión sin ciclos y una pregunta por solicitud; máximo inicial 1232 bytes de consulta y 4096 de respuesta, 4 solicitudes pendientes y caché 32. Reglas v2 individuales hasta 32, nombre normalizado ASCII/punycode, minúsculas y sin punto final; match exacto o sufijo con frontera de punto. Allow específico tiene precedencia sobre block más amplio; empate mismo dominio resuelto por acción allow explícita. Bloqueo devuelve NXDOMAIN y contador, no una página que capture credenciales.

Inicialmente dominios permitidos sin respuesta en caché devuelven SERVFAIL con DoH no configurado; no reenviar silenciosamente a DNS sin cifrar. DoH se conecta en el siguiente cambio, cuya ausencia se muestra como incompleto. Usar interfaz interna de resolución asíncrona que el cliente DoH implementará. Reglas en RAM durante sesión, sin persistir credenciales; política de reglas y datos visible en la app. Caché respeta TTL y límite, no mezcla clientes de upstream. Filtrar DNS local no garantiza bloquear IP directa/DoH de otras apps.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../sentinel-module-contracts/design.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sanitizador/ (dns_server, rules y dns_cache)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-dns.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Paquetes DNS malformados → tests de compresión/ciclos/límites.
- Evasión de DNS → documentar cobertura sin prometer filtrado de todo tráfico.

## Migration Plan

Trabajar en rama `feat/sentinel-sanitizador-dns`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
