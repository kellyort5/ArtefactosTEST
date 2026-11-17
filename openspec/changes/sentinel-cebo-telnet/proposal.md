# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

R-10 incluye puerto 23 y autenticación simulada además de HTTP; ambos son obligatorios.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar cebo: telnet simulado conforme a R-10 y H-06.
- Cierre observable: HTTP y Telnet reales simultáneos en Cebo con respuestas seguras y sin secretos.
- Dependencias de integración: `sentinel-cebo-http`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `cebo-telnet`: Cebo: Telnet simulado para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/cebo/ (telnet_service y límites compartidos)`
- `firmware/test/cebo/`
- `docs/modules/cebo-telnet.md`

No editar carpetas de otros integrantes. Riesgos: Entrada fragmentada/IAC → parser por estado con tests pequeños y límites. Secretos y sockets → cero registro de líneas y cierre en timeout/transición.
