# Spec Delta

## MODIFIED Requirements

### Requirement: Adaptadores reales separados de la interfaz
La aplicación SHALL ofrecer adaptadores BLE y MQTT que compartan contratos y normalicen estado, órdenes, confirmaciones y eventos. Las credenciales de operación SHALL suministrarse en ejecución y no quedar incorporadas al paquete o historial.

#### Scenario: Configuración de conexión
- **WHEN** el usuario configura el nodo BLE o endpoint y cuenta MQTT autorizados
- **THEN** la app establece BLE protegido o MQTT/WS autenticado real o presenta el error verificable sin mostrar secretos

#### Scenario: Android en red de confianza
- **WHEN** se instala la app en Android y se configura `ws://IP-DEL-PC:9001/mqtt` con una cuenta autorizada
- **THEN** puede conectar sin CA, hostname especial ni configuración del router cuando la red permite comunicación entre clientes
- **AND** la app permite el tráfico WS necesario sin desactivar validación HTTPS
