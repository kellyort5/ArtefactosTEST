# Spec Delta

## MODIFIED Requirements

### Requirement: Adaptadores reales separados de la interfaz
La aplicación SHALL reutilizar BLE protegido y MQTT/WS anónimo con contratos compartidos. SHALL conectar indicando endpoint e ID del nodo, sin login, usuarios, contraseña MQTT ni selector/estado de rol. SHALL conservar los secretos Wi-Fi fuera de historial y paquete.

#### Scenario: Configuración de conexión
- **WHEN** se configura nodo BLE o endpoint WS e ID de nodo
- **THEN** establece el canal real o informa fallo sin pedir cuentas

#### Scenario: Android en red de confianza
- **WHEN** se instala Android y configura ws://IP-DEL-PC:9001/mqtt
- **THEN** conecta sin CA ni credenciales MQTT
- **AND** conserva permisos Android necesarios, pairing BLE y confianza HTTPS del sistema

## ADDED Requirements

### Requirement: Control sin roles MQTT
La aplicación SHALL admitir órdenes públicas por MQTT conectado con estado vivo de Cebo, sin perfiles operador/observador. SHALL mantener comprobación de nodo, topic, boot/revisión/TTL, ACK y BLE exclusivo para operaciones privadas. Estas comprobaciones no SHALL presentarse como autenticación MQTT.

#### Scenario: Sesión anónima
- **WHEN** el canal MQTT está conectado y Cebo informa estado vivo
- **THEN** puede enviar una orden pública sin rol
- **AND** una operación BLE exclusiva no se publica en MQTT
