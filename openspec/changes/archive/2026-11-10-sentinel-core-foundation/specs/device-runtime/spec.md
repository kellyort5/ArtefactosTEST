## Purpose

Proporcionar un dispositivo que arranque de manera verificable y controle perfiles de recursos, errores y periféricos sin ejecutar simultáneamente los modos de Sentinel Node.

## ADDED Requirements

### Requirement: Arranque identificable de la placa objetivo
El dispositivo SHALL arrancar en ESP32-S3 N16R8 e informar identidad, identificador de arranque, versión, memoria detectada y estado inicial seguro sin iniciar un modo de seguridad automáticamente.

#### Scenario: Arranque sin configuración de red
- **WHEN** se enciende la placa sin credenciales de laboratorio
- **THEN** informa estado de reposo y disponibilidad de configuración local sin conectarse a ninguna red
- **AND** una discrepancia de memoria o fallo de periférico se comunica como diagnóstico sin divulgar secretos

### Requirement: Supervisor exclusivo y confirmación de transiciones
El dispositivo SHALL serializar solicitudes de app y botón, mantener como máximo un perfil operativo y confirmar el resultado aplicado o rechazado. SHALL liberar los recursos del perfil anterior antes de activar los del siguiente.

#### Scenario: Transición de Cebo a Escudo de base
- **WHEN** se acepta una solicitud válida al perfil Escudo desde Cebo
- **THEN** se detienen MQTT y STA antes de confirmar el nuevo perfil
- **AND** el estado identifica que captura y detección todavía no están implementadas

#### Scenario: Fallo o solicitudes concurrentes
- **WHEN** una transición falla o llega otra orden con revisión obsoleta
- **THEN** no hay dos perfiles activos y la orden incompatible se rechaza explícitamente
- **AND** un fallo de liberación deja estado de error sin iniciar recursos incompatibles

### Requirement: Recursos condicionados al perfil
El dispositivo SHALL impedir asociación Wi-Fi, MQTT directo y pasarela en Escudo; permitir conectividad STA/MQTT en Cebo; y rechazar Sanitizador y operaciones específicas todavía no implementadas.

#### Scenario: Solicitud de una función excluida
- **WHEN** se pide iniciar captura, honeypot o Sanitizador en esta base
- **THEN** responde con operación no disponible sin simular detección, abrir HTTP/Telnet ni activar NAT

### Requirement: Estado físico coherente con el supervisor
La pantalla SHALL mostrar perfil, fase, conectividad y errores básicos; el botón SHALL solicitar acciones al supervisor sin activar recursos por su cuenta.

#### Scenario: Uso físico del dispositivo
- **WHEN** se pulsa el botón según el patrón documentado
- **THEN** la solicitud se refleja en el estado y la pantalla muestra el resultado real
- **AND** la pulsación destinada a autorizar BLE no se interpreta también como cambio de perfil

#### Scenario: Pantalla ausente durante arranque
- **WHEN** la pantalla configurada no responde
- **THEN** el dispositivo informa el error por consola y conserva control seguro
- **AND** no autoriza nuevos emparejamientos que dependan de mostrar un código en esa pantalla
