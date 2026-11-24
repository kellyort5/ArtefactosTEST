# secure-mqtt-broker Specification

## Purpose

Proporcionar un intermediario MQTT reproducible para dispositivo y aplicación, con acceso anónimo en LAN de confianza sin TLS, cuentas ni ACL.

## Requirements

### Requirement: MQTT anónimo en LAN de confianza
El broker SHALL permitir MQTT5/TCP1883 y WS9001 sin TLS, usuarios, contraseñas ni ACL. SHALL conservar datos persistentes y no exigir preparación de secretos. La documentación SHALL indicar que cualquier cliente con acceso puede leer/publicar y que device_id no autentica.

#### Scenario: Arranque sin cuentas
- **WHEN** se inicia Compose sin material privado previo
- **THEN** dos clientes conectan y publican/suscriben por TCP/WS sin credenciales
- **AND** el reinicio conserva el volumen sin instalar certificados o generar cuentas

#### Scenario: Sin aislamiento por identidad
- **WHEN** un cliente alcanza el broker y usa un topic de otro nodo
- **THEN** no se presupone filtrado de autorización por rol o identidad
- **AND** se documenta uso en LAN propia/autorizada de confianza sin exposición Internet

### Requirement: Persistencia MQTT con alcance explícito
El broker SHALL conservar su estado MQTT configurado en almacenamiento persistente entre reinicios y documentar que no reemplaza el registro local del dispositivo ni un archivo histórico de incidentes.

#### Scenario: Reinicio del contenedor
- **WHEN** se reinicia el broker conservando el almacenamiento configurado
- **THEN** el estado retenido puede recuperarse al reconectar y el nodo informa disponibilidad actualizada
- **AND** ningún cliente interpreta un estado retenido antiguo como prueba de conexión actual
