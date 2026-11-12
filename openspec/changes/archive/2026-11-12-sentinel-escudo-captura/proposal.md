# Proposal

## Why

El catálogo debe alimentar observaciones 802.11 reales que puedan consumir todos los detectores.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar escudo: captura, parser y canales conforme a R-03, H-03 y H-06.
- Cierre observable: Captura real integrada con parser robusto, control BLE y cobertura por canal.
- Dependencias de integración: `sentinel-escudo-descubrimiento`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `escudo-capture`: Escudo: captura, parser y canales para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/escudo/ (capture, frame_parser, channel_scheduler)`
- `firmware/test/escudo/`
- `docs/modules/escudo-capture.md`

No editar carpetas de otros integrantes. Riesgos: Tramas truncadas → parser con límites y fixtures malformados. Una sola radio → scheduler y cobertura visibles; no garantía de observar todos los canales.
