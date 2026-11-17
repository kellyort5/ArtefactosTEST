# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

Las alertas internas y WSS no cumplen recepción con la aplicación cerrada.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar android: registro y recepción push conforme a R-10 y H-08.
- Cierre observable: Push real recibido con app cerrada y tap al incidente, con registro/baja seguros.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-app-cebo`, `sentinel-alertas-push-servidor`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-push`: Android: registro y recepción push para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/notifications/`
- `app/src/ (integración de recepción/navegación)`
- `app/android/ (fuentes y configuración pública)`
- `app/package.json`
- `package-lock.json`
- `app/test/push*`
- `docs/modules/app-push.md`

No editar carpetas de otros integrantes. Riesgos: Servicios Google/permisos/batería → comprobación funcional en teléfono concreto y límites documentados. Tap y carrera de sincronización → navegación con ID pendiente sin inventar incidente.
