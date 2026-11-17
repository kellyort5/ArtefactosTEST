# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

R-08 requiere diagnosticar DNS, HTTPS y portal cautivo antes de habilitar una pasarela protegida.

## What Changes

- Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar sanitizador: auditoría de conectividad conforme a R-08, R-09 y H-05.
- Cierre observable: Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.
- Dependencias de integración: `sentinel-firmware-module-api`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `sanitizer-audit`: Sanitizador: auditoría de conectividad para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/sanitizador/ (module y audit)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-audit.md`

No editar carpetas de otros integrantes. Riesgos: Portal y DNS divididos → resultado inconcluso y límites explícitos. Credenciales y TLS → solo BLE, configuración propia en NVS oficial según política posterior, sin secretos en logs/event/status ni promesa de cifrado físico; no bypass de certificados.
