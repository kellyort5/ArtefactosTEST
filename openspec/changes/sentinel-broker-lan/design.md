# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-module-contracts`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** TLS/WSS LAN real y ACL v1/v2 por nodo, con rol mínimo de notificaciones.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-04 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Conservar Mosquitto2.0.22/digest, Compose, TLS8883/WSS9001 y gestión privada .secrets de la base. Extender herramienta existente de certificados con SAN configurables del host/IP LAN; no desactivar hostname verification ni usar certificados reales versionados. App y firmware confían CA local por configuración autorizada.

ACL v2: nodo publica state/availability/metrics/events/acks y lee commands solo propio; operator lee propios y publica commands; viewer solo lee propios. Rol notify-<node> solo suscribe events/availability v2 y events históricos v1 si corresponde, sin publicar commands ni otros nodos. Mantener publicación/lectura de v1/events para replay de journal de base. Configurar IP/puertos/firewall del host para LAN autorizada sin abrir Internet; no modificar Compose de notifications ni poner HTTP/Telnet del ESP32 detrás del broker. Pruebas breves positivas/negativas con cuentas temporales propias, secretos por stdin/config local, nunca en ejemplos o logs.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `broker/ (config, scripts, pruebas y README)`, `docs/modules/broker-lan.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- SAN/CA y red host → comprobar teléfono/ESP en LAN; localhost de emulador no lo sustituye.
- ACL demasiado amplia → test cruzado por nodo y rol.

## Migration Plan

Trabajar en rama `feat/sentinel-broker-lan`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
