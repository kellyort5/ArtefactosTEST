# Spec Delta

## Purpose

El usuario necesita seleccionar redes y ver todos los detectores/cobertura del Escudo real, no solo tarjetas de diagnóstico.

## ADDED Requirements

### Requirement: Descubrimiento y control de objetivo

La app SHALL mostrar catálogo real por BLE y permitir selección, inicio, detención y cambio de objetivo con confirmación.

#### Scenario: Descubrimiento y control de objetivo verificable

- **WHEN** se elige una red propia desde el teléfono
- **THEN** el nodo aplica el objetivo y la pantalla refleja ACK/estado real

### Requirement: Actividad e incidentes completos

La app SHALL mostrar gráficos de actividad/volumen, incidentes y evidencia de todos los detectores con red, canal y tiempo/calidad.

#### Scenario: Actividad e incidentes completos verificable

- **WHEN** llega evento de cada detector y un SSID duplicado benigno
- **THEN** muestra datos y lenguaje de indicio sin convertir duplicado en ataque

### Requirement: Cobertura e historial offline

La app SHALL recuperar pendientes BLE y mostrar pausas y huecos de retención sin duplicar incidentes.

#### Scenario: Cobertura e historial offline verificable

- **WHEN** se desconecta y reconecta al Escudo
- **THEN** recupera eventos por cursor y muestra pérdida/cobertura si existe
