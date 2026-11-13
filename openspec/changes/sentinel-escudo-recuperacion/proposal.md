# Proposal

## Why

El Escudo completo debe conservar eventos cuando BLE cae y mostrar periodos sin cobertura.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar escudo: desconexión y coexistencia conforme a R-03, R-06, H-03 y H-07.
- Cierre observable: Evento recuperable tras desconexión y reinicio, huecos visibles y funciones conservadas.
- Dependencias de integración: `sentinel-escudo-detectores`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `escudo-recovery`: Escudo: desconexión y coexistencia para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/escudo/ (coverage, reconnect y backlog)`
- `firmware/test/escudo/`
- Ampliación aprobada durante implementación: `firmware/components/sentinel/include/module_runtime.h`, `module_runtime.cpp` y `runtime.cpp`, para compartir la rutina existente de persistencia con las pruebas sin cambiar contratos ni almacén.
- `docs/modules/escudo-recovery.md`

No editar carpetas de otros integrantes. Riesgos: Capacidad finita → sobrescritura visible y no prometer historial ilimitado. Contención BLE/captura → pausa solo si falla funcionalmente y registrada en cobertura.
