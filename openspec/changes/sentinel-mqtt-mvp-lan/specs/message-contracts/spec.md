# Spec Delta

## MODIFIED Requirements

### Requirement: Mensajes versionados y acotados
Los participantes SHALL intercambiar mensajes validados de la versión soportada y negociada conforme a module-protocol con tipo, identidad de nodo, arranque, identificador, secuencia, tiempo y calidad temporal. SHALL rechazar mensajes malformados, excesivos o de versión no soportada sin modificar el estado del dispositivo. Los eventos históricos v1 SHALL conservar formato e identidad originales.

#### Scenario: Datos incompatibles
- **WHEN** llega un mensaje de versión desconocida, identidad incorrecta o tamaño superior al límite documentado
- **THEN** se descarta o rechaza con diagnóstico acotado sin ejecutar órdenes ni registrar su contenido sensible

#### Scenario: Configurar MQTT del MVP
- **WHEN** se envía configure_connectivity v1 o v2 mediante BLE protegido
- **THEN** el contrato exige SSID, contraseña Wi-Fi, URI mqtt:// sin credenciales/ruta/query, usuario y contraseña MQTT, sin ca_certificate
- **AND** rechaza el formato MQTT/TLS anterior sin aplicarlo y conserva los límites de tamaño
