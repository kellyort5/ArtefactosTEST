# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Dependencias para integrar: `sentinel-module-contracts`, `sentinel-broker-lan`, `sentinel-cebo-eventos-historial`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Webhook y Telegram reales, con dedup/reintentos y estado autenticado; infraestructura mínima documentada.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-04 y R-10 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Propuesta a revisar: componente pequeño Node.js separado en notifications/, mismo runtime ya usado, paquete y lockfile propios fuera de workspaces para no competir con P4 por el lock raíz. Cliente MQTT de solo lectura events/availability del nodo, TLS validado con cuenta notify-<node> autorizada por P4. No editar broker/ ni firmware. Configuración local .env/secretos ignorados. Webhook HTTPS con lista fija de destino, timeout y autenticación propia opcional; Telegram Bot API por HTTPS con token y chat local, nunca en logs/URL de errores.

Enrutador común por event_id; solo alertas según severidad/configuración, sin multiplicar por retransmisión MQTT. Estado simple persistente de hasta512 IDs/24 h, outbox128 con vencimiento24 h y reintentos1–30 s; archivo JSON acotado escrito atómicamente en volumen privado. Control por canal/origen con ventana30 s, supresión/agrupación contabilizada. Evitar una base de datos o cola externa. Entrega webhook y Telegram independientes; HTTP2xx significa aceptado por destino, no leído. Advertir indisponibilidad y descartes al vencer/llenarse la cola. Publicar estado por API HTTPS protegida del contrato; no falsificar eventos del ESP32. Push se añade en cambio propio.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../sentinel-module-contracts/design.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `notifications/ (servicio Node, package y lockfile propios, Compose y README)`, `notifications/test/`, `docs/modules/notifications.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Nuevo proceso necesario por R-10 → usar Node existente, sin plataforma backend adicional.
- Secretos/rate limits → configuración privada, redacción y reintentos acotados.
- Caída de broker/servicio → lectura paginada por app no equivale entrega externa; documentar retención limitada del componente.

## Migration Plan

Trabajar en rama `feat/sentinel-alertas-webhook-telegram`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
