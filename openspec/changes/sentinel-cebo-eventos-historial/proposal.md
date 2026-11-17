# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

Las interacciones HTTP/Telnet necesitan clasificación, métricas, persistencia offline y recuperación del historial.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar cebo: clasificación, persistencia y mqtt conforme a R-06, R-10, H-06 y H-07.
- Cierre observable: Cebo clasifica ambos servicios y conserva/sincroniza eventos reales de manera acotada.
- Dependencias de integración: `sentinel-cebo-telnet`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `cebo-events`: Cebo: clasificación, persistencia y MQTT para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/cebo/ (classifier, counters y event_adapter)`
- `firmware/test/cebo/`
- `docs/modules/cebo-events.md`

No editar carpetas de otros integrantes. Riesgos: Ráfagas/desgaste → agregación y límites sin perder contadores; no campaña de resistencia previa. QoS1 duplicados → IDs/cursor estables y dedup del consumidor.
