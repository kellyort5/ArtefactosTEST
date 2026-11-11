# Spec Delta

## Purpose

El cierre debe distinguir lógica de detección probada de captura/interacciones reales, con evidencia reproducible del equipo.

## ADDED Requirements

### Requirement: Cobertura trazable

El laboratorio SHALL relacionar cada funcionalidad obligatoria con un caso, dueño, resultado y tipo de evidencia.

#### Scenario: Cobertura trazable verificable

- **WHEN** un caso solo dispone de fixture
- **THEN** se registra como simulado y la validación física queda pendiente

### Requirement: Pruebas autorizadas y acotadas

Las comprobaciones SHALL realizarse en equipos/redes autorizados con datos ficticios y límites sin almacenar credenciales recibidas.

#### Scenario: Pruebas autorizadas y acotadas verificable

- **WHEN** se prueba Cebo o una regla DNS
- **THEN** solo se usan clientes y destinos propios y se sanitiza el informe

### Requirement: Separación lógica y realidad

El cierre SHALL exigir captura, servicios, transporte y push reales además de casos de lógica, sin presentar mocks como integración completa.

#### Scenario: Separación lógica y realidad verificable

- **WHEN** existen tests de heurísticas pero no conexión real a la app
- **THEN** el módulo no se marca completado y queda visible qué validación falta
