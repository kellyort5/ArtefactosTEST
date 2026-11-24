# Proposal

## Why

Las observaciones necesitan detección completa y explicable; un solo detector no cumple R-03.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar escudo: todos los detectores conforme a R-03, R-09 y H-06.
- Cierre observable: Cuatro detectores integrados; casos benignos/sospechosos focalizados y eventos válidos, sin confundir indicios con certeza.
- Dependencias de integración: `sentinel-escudo-captura`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `escudo-detection`: Escudo: todos los detectores para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/escudo/ (detectors y evidence)`
- `firmware/test/escudo/`
- `docs/modules/escudo-detectors.md`

No editar carpetas de otros integrantes. Riesgos: Falsos positivos → evidencia, confianza de indicio y casos benignos obligatorios. Karma no siempre observable → informar límites y cobertura, nunca confirmar atribución.
