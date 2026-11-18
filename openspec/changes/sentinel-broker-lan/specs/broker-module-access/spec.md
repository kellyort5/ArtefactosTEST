# Spec Delta

## Purpose

Acceso MQTT autenticado por IP en LAN de confianza para teléfono, ESP32 e integraciones, con permisos exactos v1/v2.

## ADDED Requirements

### Requirement: Acceso MQTT LAN autenticado

El broker SHALL aceptar clientes MQTT5/TCP y WS por IP en redes de confianza autorizadas sin certificados ni configuración del router, conservando autenticación y ACL.

#### Scenario: Acceso LAN verificable

- **WHEN** teléfono y ESP32 conectan por IP con sus cuentas autorizadas
- **THEN** intercambian mensajes de sus topics sin CA
- **AND** el diagnóstico distingue aislamiento de red y firewall local de errores de cuenta

### Requirement: Permisos mínimos v2

El broker SHALL limitar publicación/suscripción por nodo y rol, incluyendo un rol de notificaciones de solo lectura.

#### Scenario: Permisos mínimos v2 verificable

- **WHEN** notify intenta publicar comandos o leer otro nodo
- **THEN** se rechazan ambas acciones y se permiten sus eventos autorizados

### Requirement: Continuidad de base

El broker SHALL conservar transporte de eventos históricos v1 y no exponer los servicios Cebo a Internet.

#### Scenario: Continuidad de base verificable

- **WHEN** se reenvía un evento v1 después de actualizar el nodo
- **THEN** se entrega por su topic original y no se habilita publicación pública de HTTP/Telnet
