# Proposal

## Why

Los cuatro integrantes necesitan implementar tres modos y una app con mensajes compatibles. La base v1 admite solo perfiles y eventos de diagnóstico y sus validadores rechazan los campos funcionales nuevos.

## What Changes

- Responsable único de implementación y mantenimiento de este cambio, **Persona 1, coordinador y autor de Escudo**.
- Proponer un contrato v2 completo para comandos, estados, resultados paginados, métricas e incidentes de los tres modos, con ejemplos ficticios y límites verificables.
- **BREAKING** para clientes v1 al controlar funciones nuevas. La app nueva conserva lectura y control de la base v1 según su versión y reconoce eventos v1 persistidos sin alterar sus IDs.
- Definir el contrato HTTP limitado de registro push y la interfaz de entrega de notificaciones para Persona 3 y Persona 4.
- Dejar los cuatro artefactos listos para revisión. Las tareas de implementación permanecen pendientes.

## Capabilities

### New Capabilities

- `module-protocol`: intercambio versionado y acotado de funciones de Escudo, Sanitizador y Cebo, con compatibilidad explícita de la base inicial.

### Modified Capabilities

No se modifica una spec principal existente. Las siete capacidades v1 están en openspec/specs/; sentinel-core-foundation está archivado con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas. Este cambio depende de su código y contratos y añade la versión funcional.

## Impact

La futura implementación corresponde a `contracts/`, sus fixtures y pruebas. Firmware, app y broker consumirán esta definición mediante cambios separados con sus propios responsables. Los detalles propuestos v2 requieren revisar su compatibilidad antes de integrar consumidores. Las funciones completas siguen R-03, R-08 y R-10 y el Sanitizador se desarrolla en paralelo por instrucción actual del usuario.
