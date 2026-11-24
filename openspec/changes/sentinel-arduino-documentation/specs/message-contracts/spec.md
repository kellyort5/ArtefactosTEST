# Spec Delta

## MODIFIED Requirements

### Requirement: Mensajes versionados y acotados
Los participantes SHALL intercambiar mensajes validados de la versión soportada y negociada conforme a module-protocol con tipo, identidad de nodo, arranque, identificador, secuencia, tiempo y calidad temporal. SHALL rechazar mensajes malformados, excesivos o de versión no soportada sin modificar el estado del dispositivo. Los eventos históricos v1 SHALL conservar formato e identidad originales.

#### Scenario: Datos incompatibles
- **WHEN** llega un mensaje de versión desconocida, identidad incorrecta o tamaño superior al límite documentado
- **THEN** se descarta o rechaza con diagnóstico acotado sin ejecutar órdenes ni registrar su contenido sensible

#### Scenario: Configurar MQTT del MVP
- **WHEN** se envía configure_connectivity v1 o v2 mediante BLE protegido
- **THEN** el contrato exige SSID, contraseña Wi-Fi, URI mqtt:// sin credenciales/ruta/query, sin usuario ni contraseña MQTT, sin ca_certificate
- **AND** rechaza el formato MQTT/TLS anterior sin aplicarlo y conserva los límites de tamaño

### Requirement: Estado y extensiones sin capacidades ficticias
El estado SHALL exponer revisión, perfil, fase, conectividad y capacidades reales. Las operaciones de detección, honeypot, alertas externas y Sanitizador que todavía no estén implementadas SHALL aparecer no disponibles.

#### Scenario: Estado de un perfil preparado
- **WHEN** el perfil Cebo o Escudo está configurado
- **THEN** se distingue disponibilidad de transporte de disponibilidad del módulo funcional
- **AND** no se rellenan métricas o incidentes de seguridad con valores ficticios
