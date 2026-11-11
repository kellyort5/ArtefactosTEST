# Spec Delta

## Purpose

MQTT y una app cerrada no envían por sí mismos las alertas externas obligatorias de R-10.

## ADDED Requirements

### Requirement: Entrega de dos canales

El componente SHALL enviar alertas reales a un webhook y Telegram según configuración autorizada sin exponer secretos.

#### Scenario: Entrega de dos canales verificable

- **WHEN** un incidente Cebo válido llega por MQTT
- **THEN** ambos destinos configurados reciben una alerta correlacionada al evento

### Requirement: Repetición y recuperación acotadas

El componente SHALL deduplicar IDs, controlar alertas repetitivas y mantener reintentos limitados por canal.

#### Scenario: Repetición y recuperación acotadas verificable

- **WHEN** MQTT reentrega el mismo ID o falla un destino
- **THEN** no duplica alertas y el canal fallido reintenta dentro de límites visibles

### Requirement: Estado verificable

El componente SHALL informar estado y fallos de canales mediante acceso autenticado sin divulgar tokens ni contenidos sensibles.

#### Scenario: Estado verificable verificable

- **WHEN** la app autorizada consulta estado tras un fallo de Telegram
- **THEN** recibe estado sanitizado y contadores, sin token ni chat privado
