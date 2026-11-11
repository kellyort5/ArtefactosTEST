# Proposal

## Why

Cambios aislados no acreditan recursos compartidos, transiciones, OLED/botón y recuperación del sistema completo.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar integración final de los tres modos conforme a R-03, R-05, R-06, R-07, R-08, R-10 y R-11.
- Asignar a Persona 1 las alertas recientes en OLED conforme a la página 4 del PDF: mostrar un resumen de la alerta más reciente sin depender de abrir la app y comprobarlo en la pantalla física.
- Cierre observable: Tres modos completos e integrados, controles físicos y recuperación reales; fallos por componente devueltos a su autor.
- Dependencias de integración: `sentinel-escudo-recuperacion`, `sentinel-sanitizador-doh`, `sentinel-cebo-eventos-historial`, `sentinel-app-escudo`, `sentinel-app-sanitizador`, `sentinel-app-push`, `sentinel-broker-lan`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `system-integration`: Integración final de los tres modos para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/sentinel/ (registro final y recursos compartidos)`
- `firmware/main/`
- `firmware/sdkconfig.defaults`
- `firmware/test/integration/`
- `docs/integration-three-modes.md`

No editar carpetas de otros integrantes. Riesgos: Una placa compartida → coordinar comprobaciones y registrar build/estado. Integración tardía → cada cambio anterior ya aporta verificación; aquí comprobar recorridos cruzados, no desarrollar funciones faltantes.
