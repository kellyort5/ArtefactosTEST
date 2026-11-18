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
