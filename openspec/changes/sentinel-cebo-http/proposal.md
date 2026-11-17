# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

La base Cebo solo conecta MQTT; falta un servicio HTTP real seguro que genere interacciones normalizadas.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar cebo: panel http ficticio conforme a R-10 y H-06.
- Cierre observable: Panel ESP32 accesible y eventos sanitizados; recursos acotados y cierre real.
- Dependencias de integración: `sentinel-firmware-module-api`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `cebo-http`: Cebo: panel HTTP ficticio para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/cebo/ (module y http_service)`
- `firmware/test/cebo/`
- `docs/modules/cebo-http.md`

No editar carpetas de otros integrantes. Riesgos: Credenciales recibidas → descarte en memoria y pruebas que buscan marcadores de secreto en salidas. Conexiones lentas → límites/timeout y cierre al salir.
