# Spec Delta

## MODIFIED Requirements

### Requirement: Configuración privada de conectividad
El dispositivo SHALL recibir solo SSID, contraseña Wi-Fi y mqtt_uri por BLE protegido, validarlos y persistirlos durante uso normal/reinicio/corte. SHALL omitir cuenta MQTT y CA, rechazar el formato anterior y mantener la contraseña Wi-Fi fuera de mensajes públicos, historial y logs.

#### Scenario: Configuración local válida
- **WHEN** un cliente BLE autorizado suministra red propia y broker mqtt:// sin cuenta
- **THEN** persiste antes de confirmar y conecta solo al entrar en el perfil aplicable
- **AND** no publica contraseña Wi-Fi

#### Scenario: Recuperación tras reinicio sin USB
- **WHEN** se reinicia después de confirmar una configuración válida
- **THEN** la recupera en Idle sin conectarse automáticamente ni restaurar UTC obsoleta

#### Scenario: Escritura de configuración fallida
- **WHEN** falla el guardado
- **THEN** rechaza sin confirmar persistencia ni perder la configuración válida previa

#### Scenario: Configuración ausente o inválida
- **WHEN** falta configuración o su formato es antiguo/inválido
- **THEN** informa que requiere aprovisionamiento sin borrar NVS, bonds o historial automáticamente

### Requirement: STA y MQTT anónimos en el perfil Cebo
El dispositivo SHALL usar STA en red autorizada de confianza y MQTT5/TCP anónimo, sin TLS/CA/cuenta, para eventos y órdenes Cebo. SHALL mantener reconexión no bloqueante, validación de mensajes y tiempo monotónico sin exigir UTC. La seguridad de transporte MQTT queda fuera del MVP.

#### Scenario: Broker no disponible
- **WHEN** falla conectividad del broker
- **THEN** informa y reintenta sin bloquear supervisor, botón o display ni pedir cuentas

#### Scenario: Reinicio sin hora sincronizada
- **WHEN** entra en Cebo con configuración válida sin UTC
- **THEN** conecta anónimamente y publica tiempo monotónico con calidad desconocida
