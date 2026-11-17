# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

Preparar wolfMQTT y combinar manualmente la configuración de Arduino facilita errores y exige conocer detalles ajenos al trabajo del equipo. El usuario solicita dos instaladores PowerShell repetibles conservando las decisiones ya aprobadas.

## What Changes

- Descargar y verificar wolfMQTT2.2.0, preparar su estructura Arduino y aplicar los tres archivos del repositorio fuera del sketch.
- Mantener cuatro claves y retirar cinco overrides Sentinel antiguos de carga/exportación según política posterior, con respaldo y preservación de claves/comentarios ajenos e idempotencia.
- Detectar instalaciones correctas sin modificarlas; preparar cualquier reemplazo completo antes de sustituir la versión anterior y recuperarla ante fallo.
- Sustituir instrucciones manuales por comandos iniciales y pruebas focalizadas en Windows PowerShell.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `project-workspace`: preparación inicial automatizada e idempotente de dependencias y configuración Arduino, conservando datos y configuración ajena.

## Impact

firmware/arduino contiene instaladores, configuración existente y metadatos/hashes del proveedor; firmware/test contiene pruebas aisladas; se actualizan las guías de instalación. Se reutilizan PowerShell5.1, .NET, Arduino CLI instalado y archivos originales del proveedor. Sin nuevas dependencias, SDK propio, cambios de firmware funcional, ESP_SSLClient, contratos o Git de publicación. Riesgos: rutas personalizadas, descargas incompletas y archivos bloqueados; resolver con validación previa, staging y respaldo/recuperación.
