# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-sanitizador-auditoria`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Cliente real con DHCP/NAT y métricas verificadas; cierre del modo sin recursos residuales.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08 y H-04 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Pedir STA+AP al gestor y usar esp_netif/DHCP/lwIP oficiales. El framework ESP-IDF instalado contiene Kconfig LWIP_IP_FORWARD y LWIP_IPV4_NAPT y la cabecera lwip/lwip_napt.h; reutilizarlos en lugar de añadir un framework router. P1 integra los flags globales del manifiesto de configuración solicitado por P2 en el núcleo; P2 no edita sdkconfig compartido. AP Sentinel_Secure con clave propia 8–63 por BLE, WPA2-PSK como mínimo, sin credencial predeterminada. Propuesta inicial de subred 192.168.50.1/24, validada para no solapar upstream; conflicto rechaza inicio hasta reconfigurar.

DHCP anuncia DNS local del AP, integrado en el siguiente incremento; antes de DNS/DoH el estado es pasarela sin protección DNS, nunca protegido. NAPT sobre interfaz autorizada IPv4, sin port forwarding entrante. Control BLE continúa. Obtener asociación/IP y RSSI mediante APIs disponibles; RSSI desconocido se declara null con motivo, no se inventa. Contar bytes IPv4 reenviados de subida/bajada por cliente con punto de observación acotado en netif, sin modificar fuentes del framework instalado; prueba focalizada debe comprobar ambos sentidos. Deshabilitar IPv6 forwarding y no anunciar protección IPv6. No construir un DPI.

Al caer upstream, declarar degradado y al salir cerrar NAPT/DHCP/AP y reiniciar contadores de sesión según contrato; eventos mantienen identidad. La radio STA+AP comparte canal físico de upstream, lo cual se informa y no se trata como dos radios independientes.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../sentinel-module-contracts/design.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sanitizador/ (gateway, clients y traffic)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-gateway.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- NAT y TLS compiten por memoria → límites y comprobación funcional con un cliente; optimizar solo si bloquea.
- Soporte RSSI/contadores → validar en placa al implementar, conservar métrica explícita desconocida si no observable.

## Migration Plan

Trabajar en rama `feat/sentinel-sanitizador-pasarela`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
