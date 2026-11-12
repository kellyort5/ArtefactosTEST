# module-dispatch Specification

## Purpose

Resources y Profile solo ejecutan la base; los otros autores necesitan integrar sus componentes sin editar runtime ni crear controladores Wi-Fi duplicados.

## Requirements

### Requirement: Exclusión de modos

El supervisor SHALL activar como máximo un modo y rechazar una transición cuando el modo anterior no libera sus recursos.

#### Scenario: Exclusión de modos verificable

- **WHEN** un servicio no confirma su parada
- **THEN** el destino no arranca y el error queda visible

### Requirement: Despacho de funcionalidades reales

El firmware SHALL despachar órdenes autorizadas a módulos registrados y anunciar capacidades según su disponibilidad real.

#### Scenario: Despacho de funcionalidades reales verificable

- **WHEN** se pide una capacidad de un módulo no incluido
- **THEN** se devuelve no disponible sin afirmar que está implementada

### Requirement: Propiedad de recursos y eventos

El sistema SHALL centralizar recursos de radio, identidad y persistencia de eventos; los módulos SHALL mantener acotado su trabajo pendiente.

#### Scenario: Propiedad de recursos y eventos verificable

- **WHEN** dos módulos solicitan radio o se llena una cola
- **THEN** solo el modo activo obtiene recursos y la saturación se contabiliza sin bloquear el control
