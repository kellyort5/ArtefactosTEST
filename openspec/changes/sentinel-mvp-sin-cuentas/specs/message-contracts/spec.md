# Spec Delta

## ADDED Requirements

### Requirement: Contrato MQTT sin cuentas
configure_connectivity v1/v2 SHALL aceptar solo ssid, wifi_password y mqtt_uri con mqtt://, rechazando campos MQTT antiguos. La admisión SHALL exigir autenticación/cifrado en BLE, omitir autenticación/roles en MQTT y mantener validación de destino, perfil, boot, vigencia y duplicados. El ID de nodo SHALL ser una referencia de enrutamiento, no una credencial.

#### Scenario: Campos retirados
- **WHEN** un cliente envía mqtt_username, mqtt_password o CA de la configuración anterior
- **THEN** el mensaje se rechaza antes de guardar o ejecutar
- **AND** las herramientas usan los tipos y schemas regenerados

#### Scenario: MQTT anónimo válido
- **WHEN** se recibe una orden MQTT vigente del nodo y perfil adecuados sin identidad autenticada
- **THEN** la ausencia de usuario/rol no es causa de rechazo
- **AND** los rechazos por retained, boot, caducidad, revisión o canal incorrecto siguen vigentes

#### Scenario: BLE conserva autenticación
- **WHEN** una orden BLE llega sin pairing autorizado o sin cifrado
- **THEN** se rechaza aunque MQTT funcione anónimamente
