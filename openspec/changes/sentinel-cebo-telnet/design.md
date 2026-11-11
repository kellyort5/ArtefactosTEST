# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Dependencias para integrar: `sentinel-cebo-http`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** HTTP y Telnet reales simultáneos en Cebo con respuestas seguras y sin secretos.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-10 y H-06 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Servidor TCP puerto23 con sockets lwIP, 2 sesiones Telnet y total de 6 junto a HTTP. Máquina de estados mínima por sesión: saludo/login/password/respuesta fija/cierre, con tratamiento acotado de IAC sin negociar funciones peligrosas. Línea máxima512 y timeout5 s; input descartado tras contar interacción, no journal ni echo de secretos. El servicio simula respuesta, nunca llama shell, system, exec ni intérprete. No guardar nombres de usuario ni claves. Reutilizar bus de interacciones del HTTP para IP/puertos/tipo y luego clasificación. La parada de Cebo cierra ambos servicios; no modificar app ni broker.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../sentinel-module-contracts/design.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/cebo/ (telnet_service y límites compartidos)`, `firmware/test/cebo/`, `docs/modules/cebo-telnet.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Entrada fragmentada/IAC → parser por estado con tests pequeños y límites.
- Secretos y sockets → cero registro de líneas y cierre en timeout/transición.

## Migration Plan

Trabajar en rama `feat/sentinel-cebo-telnet`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
