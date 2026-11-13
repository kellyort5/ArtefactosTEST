# Proposal

## Why

La app base solo valida v1 y no tiene control/estado funcional de tres módulos; hace falta un adaptador común antes de pantallas.

## What Changes

- Responsable único: **Persona 1 — Jefry: app común v2 por reasignación explícita del usuario**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar app: protocolo funcional y navegación conforme a R-03, R-04, R-06, R-07 y H-08.
- Cierre observable: App nueva mantiene base y acepta protocolo funcional, con navegación/control/historial comunes.
- Dependencias de integración: `sentinel-module-contracts`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-module-protocol`: App: protocolo funcional y navegación para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/ (controller, store, journal, transports y navegación)`
- `app/test/`
- `app/package.json`
- `package.json`
- `package-lock.json`
- `app/README.md`

No editar carpetas de otros integrantes. Riesgos: Retained y versiones mezcladas → leer estado vivo y pruebas de boot/revisión. Duplicados/reconexión → journal estable y fixtures comunes.
