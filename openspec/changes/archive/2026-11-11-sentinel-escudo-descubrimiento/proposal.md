# Proposal

## Why

El perfil Escudo actual solo declara estado; necesita catálogo pasivo y selección real antes de capturar una red.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar escudo: descubrimiento y objetivo conforme a R-03 y H-03.
- Cierre observable: Catálogo y objetivo reales por BLE; selección confirmada; ausencia de asociación/MQTT.
- Dependencias de integración: `sentinel-firmware-module-api`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `escudo-discovery`: Escudo: descubrimiento y objetivo para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/escudo/ (module, discovery, target y CMake)`
- `firmware/test/escudo/`
- `docs/modules/escudo-discovery.md`

No editar carpetas de otros integrantes. Riesgos: Scan y BLE comparten radio → atender fin de scan por evento y mantener control BLE; no escanear en callback. Catálogo incompleto → límite y paginación visibles, nuevo scan explícito.
