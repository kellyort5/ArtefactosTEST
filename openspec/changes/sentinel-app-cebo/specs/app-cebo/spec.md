# Spec Delta

## Purpose

Los servicios Cebo requieren visualización MQTT en tiempo real, historial y visibilidad del envío de alertas.

## ADDED Requirements

### Requirement: Tiempo real y control Cebo

La app SHALL mostrar estado HTTP/Telnet, métricas e incidentes reales por MQTT y confirmar control según disponibilidad.

#### Scenario: Tiempo real y control Cebo verificable

- **WHEN** un cliente accede a HTTP y Telnet
- **THEN** aparecen ambos tipos de interacción y los contadores correspondientes

### Requirement: Historial e incidentes

La app SHALL conservar y recuperar historial limitado con dedup, filtros y huecos explícitos sin mostrar datos sensibles.

#### Scenario: Historial e incidentes verificable

- **WHEN** MQTT se desconecta y después repite un evento recuperado
- **THEN** el historial contiene un único registro por ID e indica pérdidas si existen

### Requirement: Alertas y entrega externa

La app SHALL mostrar alertas internas y estado autenticado de canales externos diferenciando envío de lectura.

#### Scenario: Alertas y entrega externa verificable

- **WHEN** un canal externo falla y otro acepta una alerta
- **THEN** la pantalla muestra ambos estados y el incidente sigue accesible
