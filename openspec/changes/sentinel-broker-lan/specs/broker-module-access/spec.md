# Spec Delta

## Purpose

Integrar MQTT anónimo por IP en la LAN de confianza del equipo, con v1/v2 y notificaciones sin cuentas ni ACL.

## ADDED Requirements

### Requirement: Acceso MQTT LAN anónimo
El broker SHALL permitir TCP1883/WS9001 por IP sin TLS ni cuentas y conservar volumen. El despliegue SHALL documentar firewall/aislamiento y ausencia de autenticación/ACL, sin prometer comunicación en cualquier Wi-Fi.

#### Scenario: Acceso LAN verificable
- **WHEN** teléfono y ESP32 conectan por IP en la red del equipo sin cuenta
- **THEN** intercambian mensajes v1/v2 por TCP/WS sin CA
- **AND** el informe distingue prueba física de smoke local

### Requirement: Integración de alertas sin rol MQTT
P3 SHALL poder suscribir eventos contratados sin rol notify ni ACL. El consumidor SHALL validar contrato, nodo esperado e identidad de evento sin afirmar autenticidad del emisor anónimo.

#### Scenario: Suscripción de notificaciones
- **WHEN** el componente de P3 recibe eventos reales o simulados declarados
- **THEN** valida, deduplica y conserva su identidad/origen sin cuenta MQTT
- **AND** documenta que el broker no restringe lectura o publicación por identidad

### Requirement: Continuidad de base
El broker SHALL conservar eventos históricos v1 y no exponer servicios Cebo a Internet.

#### Scenario: Continuidad verificable
- **WHEN** se reenvía un evento v1
- **THEN** conserva topic/identidad y no se publica HTTP/Telnet a Internet
