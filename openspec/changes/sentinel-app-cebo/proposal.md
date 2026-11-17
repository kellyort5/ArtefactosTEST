# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

Los servicios Cebo requieren visualización MQTT en tiempo real, historial y visibilidad del envío de alertas.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar app: cebo, historial y alertas conforme a R-04, R-06, R-10 y H-08.
- Cierre observable: Cebo real visible por MQTT, control confirmado, historial y alertas internas/estado externo.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-cebo-eventos-historial`, `sentinel-alertas-webhook`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-cebo`: App: Cebo, historial y alertas para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/cebo/`
- `app/src/ (registro de ruta y store Cebo)`
- `app/test/cebo*`
- `docs/modules/app-cebo.md`

No editar carpetas de otros integrantes. Riesgos: Historial retenido limitado → gap visible, no historial infinito. Estado backend/nodo diferente → presentación separada sin falsos eventos del ESP32.
