# Spec Delta

## Purpose

Las interacciones HTTP/Telnet necesitan clasificación, métricas, persistencia offline y recuperación del historial.

## ADDED Requirements

### Requirement: Clasificación de conducta

Cebo SHALL registrar origen, puertos, protocolo, tipo, tiempo/calidad, cantidad y frecuencia y clasificar patrones repetitivos con evidencia.

#### Scenario: Clasificación de conducta verificable

- **WHEN** un origen hace accesos aislados y luego varios intentos ficticios
- **THEN** los contadores y la sospecha reflejan la diferencia sin atribución concluyente

### Requirement: Registro persistente offline

Cebo SHALL conservar eventos pendientes en un búfer persistente limitado y recuperarlos tras desconexión/reinicio con IDs estables.

#### Scenario: Registro persistente offline verificable

- **WHEN** MQTT cae, ocurre una interacción y se reinicia el nodo
- **THEN** el evento sigue disponible y se sincroniza al volver el canal

### Requirement: Historial MQTT deduplicable

Cebo SHALL publicar eventos y métricas por MQTT seguro y permitir lectura paginada con detección de huecos.

#### Scenario: Historial MQTT deduplicable verificable

- **WHEN** se repite una entrega o el cursor ya fue sobrescrito
- **THEN** el consumidor reconoce la identidad repetida o recibe gap explícito
