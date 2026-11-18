# Spec Delta

## REMOVED Requirements

### Requirement: Listeners LAN autenticados sin certificados
**Reason**: el usuario elimina autenticación MQTT del MVP.
**Migration**: conectar sin cuenta por TCP1883 o WS9001.

### Requirement: Permisos separados por identidad y nodo
**Reason**: se eliminan roles y ACL en todos los consumidores MQTT.
**Migration**: los topics enrutan mensajes y no acreditan identidad.

### Requirement: Configuración y datos privados fuera del repositorio
**Reason**: la preparación de cuentas queda sustituida por despliegue directo.
**Migration**: conservar secretos ajenos a MQTT fuera de Git y el volumen existente.

## ADDED Requirements

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
