# message-contracts Specification

## Purpose

Definir un contrato compartido de intercambio entre ESP32 y aplicación que conserve identidad, validación y resultados de comandos independientemente de BLE o MQTT.

## Requirements

### Requirement: Mensajes versionados y acotados
Los participantes SHALL intercambiar mensajes validados de la versión soportada y negociada conforme a module-protocol con tipo, identidad de nodo, arranque, identificador, secuencia, tiempo y calidad temporal. SHALL rechazar mensajes malformados, excesivos o de versión no soportada sin modificar el estado del dispositivo. Los eventos históricos v1 SHALL conservar formato e identidad originales.

#### Scenario: Datos incompatibles
- **WHEN** llega un mensaje de versión desconocida, identidad incorrecta o tamaño superior al límite documentado
- **THEN** se descarta o rechaza con diagnóstico acotado sin ejecutar órdenes ni registrar su contenido sensible

#### Scenario: Configurar MQTT del MVP
- **WHEN** se envía configure_connectivity v1 o v2 mediante BLE protegido
- **THEN** el contrato exige SSID, contraseña Wi-Fi, URI mqtt:// sin credenciales/ruta/query, sin usuario ni contraseña MQTT, sin ca_certificate
- **AND** rechaza el formato MQTT/TLS anterior sin aplicarlo y conserva los límites de tamaño

### Requirement: Comandos frescos y autorizados
Las órdenes SHALL identificar comando, nodo y arranque destinatarios, revisión esperada y vencimiento verificable con el tiempo del dispositivo. Una orden no autorizada, retenida, vencida o dirigida a otro arranque SHALL rechazarse sin efectos.

#### Scenario: Orden pendiente de otro arranque
- **WHEN** se entrega una orden cuyo arranque o revisión ya no coincide con el estado actual
- **THEN** se comunica rechazo correlacionado y se conserva el estado vigente

#### Scenario: Orden sin reloj UTC válido
- **WHEN** la app envía una orden basada en el estado y tiempo monotónico recién consultados
- **THEN** el nodo puede comprobar su vigencia sin asociarse a Wi-Fi para obtener hora UTC

### Requirement: Confirmación idempotente del resultado
La respuesta SHALL correlacionarse con el comando y distinguir recepción de resultado aplicado o rechazado. Un duplicado idéntico vigente SHALL reutilizar el resultado conocido sin repetir la operación; reutilizar identificador con otro contenido SHALL rechazarse.

#### Scenario: Reentrega de comando
- **WHEN** el mismo comando válido llega otra vez por el mismo canal o por otro autorizado
- **THEN** se conserva un único efecto y se devuelve su resultado conocido
- **AND** la app no considera un acuse MQTT como confirmación de aplicación

### Requirement: Estado y extensiones sin capacidades ficticias
El estado SHALL exponer revisión, perfil, fase, conectividad y capacidades reales. Las operaciones de detección, honeypot, alertas externas y Sanitizador que todavía no estén implementadas SHALL aparecer no disponibles.

#### Scenario: Estado de un perfil preparado
- **WHEN** el perfil Cebo o Escudo está configurado
- **THEN** se distingue disponibilidad de transporte de disponibilidad del módulo funcional
- **AND** no se rellenan métricas o incidentes de seguridad con valores ficticios

### Requirement: Entrega BLE completa y recuperación de sesión
Los participantes SHALL preservar el mismo mensaje lógico al fragmentarlo por BLE, limitar longitud y tiempo de ensamblado y descartar fragmentos incompletos de sesiones anteriores.

#### Scenario: Desconexión durante un mensaje
- **WHEN** se interrumpe BLE antes de completar el mensaje
- **THEN** no se ejecuta una orden parcial
- **AND** al reconectar se consulta estado y se retransmite un mensaje completo vigente

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
