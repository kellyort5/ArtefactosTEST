# Proposal

## Why

El cierre debe distinguir lógica de detección probada de captura/interacciones reales, con evidencia reproducible del equipo.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar laboratorio y evidencia funcional conforme a R-03, R-08, R-09, R-10 y R-11.
- Cierre observable: Matriz y procedimientos listos pronto; cierre con evidencia real de todos los módulos y sin confundir fixture con hardware.
- Dependencias de integración: `sentinel-module-contracts`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `module-validation`: Laboratorio y evidencia funcional para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `simulator/ (fixtures/reproductor de mensajes, sin ataque por radio)`
- `docs/lab/`
- `docs/evidence/ (plantillas e informes sanitizados)`

No editar carpetas de otros integrantes. Riesgos: Falsos positivos/negativos y fixtures → casos benignos y evidencia explícitamente etiquetada. Hardware compartido → turnos cortos por módulo y registro de versión para no pisar validaciones.
