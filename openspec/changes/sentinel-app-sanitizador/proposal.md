# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

R-08 necesita configuración BLE, resultados de auditoría, clientes/métricas y controles de reglas/DoH accesibles.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar app: auditor y sanitizador completo conforme a R-08, H-04, H-05 y H-08.
- Cierre observable: R-08 operable desde app real por BLE y métricas/diagnósticos verificables.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-sanitizador-auditoria`, `sentinel-sanitizador-doh`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-sanitizer`: App: Auditor y Sanitizador completo para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/sanitizador/`
- `app/src/ (registro de ruta y store Sanitizador)`
- `app/test/sanitizador*`
- `docs/modules/app-sanitizador.md`

No editar carpetas de otros integrantes. Riesgos: Pérdida de Internet del teléfono al cambiar red → BLE sigue controlando, estado explícito. Datos desconocidos/secretos → null con motivo y limpieza de formularios.
