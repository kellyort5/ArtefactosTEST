# Spec Delta

## MODIFIED Requirements

### Requirement: Configuración privada de conectividad
El dispositivo SHALL recibir configuración Wi-Fi y MQTT de laboratorio mediante BLE protegido, validar sus límites y persistir la configuración aceptada durante uso normal, reinicios y cortes de alimentación, y mantener los secretos fuera de mensajes públicos, historial y logs.

#### Scenario: Configuración local válida
- **WHEN** un cliente BLE autorizado suministra parámetros de una red propia y broker confiable
- **THEN** el nodo permite aplicar la conexión únicamente al entrar en un perfil autorizado
- **AND** no devuelve la contraseña ni la publica como estado

#### Scenario: Recuperación tras reinicio sin USB
- **WHEN** se reinicia o recupera alimentación después de confirmar una configuración persistida
- **THEN** el nodo recupera la última configuración válida sin activar un modo ni asociarse automáticamente a Wi-Fi
- **AND** un reloj guardado antes del reinicio no se considera una hora UTC vigente

#### Scenario: Escritura de configuración fallida
- **WHEN** no puede persistirse una configuración nueva
- **THEN** el comando informa un fallo sin confirmar que la configuración quedó guardada
- **AND** conserva la configuración válida anterior sin publicar secretos

#### Scenario: Configuración ausente o inválida
- **WHEN** el almacenamiento no contiene configuración o contiene una entrada inválida
- **THEN** el nodo permanece sin conectividad configurada e informa el estado sin revelar datos sensibles ni borrar vínculos o historial
