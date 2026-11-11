# Proposal

## Why

MQTT y una app cerrada no envían por sí mismos las alertas externas obligatorias de R-10.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar el componente de alertas y webhook conforme a R-04 y al alcance actualizado de R-10.
- Cierre observable: Webhook real, con dedup/reintentos y estado autenticado; infraestructura mínima documentada.
- Dependencias de integración: `sentinel-module-contracts`, `sentinel-broker-lan`, `sentinel-cebo-eventos-historial`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `notification-dispatch`: Componente de alertas y webhook para completar la entrega integrada de Sentinel Node; el envío push conserva su cambio independiente.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `notifications/ (servicio Node, package y lockfile propios, Compose y README)`
- `notifications/test/`
- `docs/modules/notifications.md`

No editar carpetas de otros integrantes. Riesgos: Nuevo proceso necesario por R-10 → usar Node existente, sin plataforma backend adicional. Secretos/rate limits → configuración privada, redacción y reintentos acotados. Caída de broker/servicio → lectura paginada por app no equivale entrega externa; documentar retención limitada del componente.
