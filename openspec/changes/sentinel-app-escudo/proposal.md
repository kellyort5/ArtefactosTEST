# Proposal

## Why

El usuario necesita seleccionar redes y ver todos los detectores/cobertura del Escudo real, no solo tarjetas de diagnóstico.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar app: escudo completo conforme a R-03, R-06 y H-08.
- Cierre observable: Pantalla Escudo completa integrada por BLE, con control, gráficos, incidentes y recuperación reales.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-escudo-descubrimiento`, `sentinel-escudo-detectores`, `sentinel-escudo-recuperacion`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-escudo`: App: Escudo completo para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/escudo/`
- `app/src/ (registro de ruta y store Escudo)`
- `app/test/escudo*`
- `docs/modules/app-escudo.md`

No editar carpetas de otros integrantes. Riesgos: UI puede adelantar firmware → distinguir fixture de dato real y no marcar cierre por fixture. Alertas ambiguas → evidencia/indicio y SSID duplicado sin etiqueta malicioso.
