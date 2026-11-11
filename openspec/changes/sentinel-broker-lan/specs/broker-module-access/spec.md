# Spec Delta

## Purpose

El broker base usa certificados localhost y ACL v1; ESP32/teléfono y notificaciones necesitan nombres LAN y permisos concretos.

## ADDED Requirements

### Requirement: Identidad TLS LAN

El broker SHALL aceptar clientes TLS/WSS de la LAN autorizada con hostname/SAN verificables y rechazar identidades inválidas.

#### Scenario: Identidad TLS LAN verificable

- **WHEN** el teléfono/ESP conecta con CA correcta y luego con hostname incorrecto
- **THEN** la primera conexión funciona y la segunda falla sin omitir TLS

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
