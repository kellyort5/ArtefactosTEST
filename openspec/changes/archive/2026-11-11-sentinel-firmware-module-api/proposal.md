# Proposal

## Why

Resources y Profile solo ejecutan la base; los otros autores necesitan integrar sus componentes sin editar runtime ni crear controladores Wi-Fi duplicados.

## What Changes

- Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar api de módulos y supervisor conforme a R-02, R-05, R-06, R-07, R-11.
- Cierre observable: Build ESP-IDF y pruebas de exclusión/gate pasan; API revisada por consumidores; hooks integrables sin editar módulos ajenos.
- Dependencias de integración: `sentinel-module-contracts`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `module-dispatch`: API de módulos y supervisor para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/sentinel_api/`
- `firmware/components/sentinel/ (supervisor, runtime, wire, network y registro central)`
- `firmware/main/`
- `firmware/CMakeLists.txt`
- `firmware/sdkconfig.defaults`
- `firmware/test/`
- `docs/development.md`

No editar carpetas de otros integrantes. Riesgos: Interfaz nueva compartida → revisar firmas con Personas 2 y 3 antes de consumidores. Callbacks y parada → colas acotadas, prueba de fallo de liberación y un solo dueño de drivers.
