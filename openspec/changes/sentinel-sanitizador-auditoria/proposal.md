# Proposal

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

- `firmware/components/sanitizador/ (module, audit y CMake)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-audit.md`

No editar carpetas de otros integrantes. Riesgos: Portal y DNS divididos → resultado inconcluso y límites explícitos. Credenciales y TLS → solo BLE, RAM, sin logs; no bypass de certificados.
