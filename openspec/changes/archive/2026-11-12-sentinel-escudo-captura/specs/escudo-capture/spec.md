# Spec Delta

## Purpose

El catálogo debe alimentar observaciones 802.11 reales que puedan consumir todos los detectores.

## ADDED Requirements

### Requirement: Observaciones reales acotadas

Escudo SHALL analizar Beacon, Deauthentication y Disassociation capturados realmente y descartar entradas truncadas sin perder control.

#### Scenario: Observaciones reales acotadas verificable

- **WHEN** llegan tramas válidas y truncadas
- **THEN** se generan observaciones válidas y aumenta el contador de descartes sin fallo

### Requirement: Prioridad y recorrido

Escudo SHALL priorizar el canal del objetivo y recorrer canales permitidos registrando cobertura y pausas.

#### Scenario: Prioridad y recorrido verificable

- **WHEN** se monitorea una red y existe otra con el mismo SSID en otro canal
- **THEN** se producen observaciones de ambos canales con cobertura identificable

### Requirement: Ciclo de captura

Escudo SHALL iniciar, detener y cambiar la captura mediante órdenes confirmadas sin asociarse ni generar tráfico de sondeo.

#### Scenario: Ciclo de captura verificable

- **WHEN** se detiene y cambia el objetivo
- **THEN** cesa la captura anterior y solo se procesa la generación vigente
