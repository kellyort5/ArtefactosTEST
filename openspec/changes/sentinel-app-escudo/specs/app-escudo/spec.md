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

### Requirement: Interfaz de producto sin panel temporal

La app SHALL ofrecer las funciones del Escudo desde su pantalla normal, sin panel Diagnóstico P1, flag de compilación ni API de envío de sobres deliberadamente inválidos o replay diagnóstico en el controller de producto. SHALL conservar las comprobaciones negativas de protocolo como pruebas de software.

#### Scenario: APK normal y configuración antigua

- **WHEN** se compila el APK, incluso si el entorno conserva la antigua variable de diagnóstico
- **THEN** solo se ofrecen los controles normales y no reaparece el panel ni sus acciones de prueba
- **AND** el emparejamiento BLE protegido y la salida serie local de firmware sin OLED conservan su comportamiento
