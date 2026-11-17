# device-connectivity Specification

## Purpose

Conectar la base de Sentinel Node con la aplicación por canales reales y autenticados, conservando las restricciones de radio propias de cada perfil operativo.

## Requirements

### Requirement: Control BLE autenticado por posesión física
El dispositivo SHALL autorizar nuevos clientes BLE solo durante una ventana activada físicamente y mediante emparejamiento autenticado y cifrado. Fuera de esa ventana solo SHALL aceptar acceso protegido de clientes previamente autorizados. El dispositivo SHALL recordar hasta tres teléfonos sin borrar automáticamente vínculos existentes.

#### Scenario: Cliente nuevo sin autorización
- **WHEN** un teléfono no autorizado intenta leer estado protegido, enviar comandos o configurar la red fuera de la ventana
- **THEN** el dispositivo rechaza el acceso y no divulga configuración o datos del nodo

#### Scenario: Emparejamiento autorizado
- **WHEN** el usuario abre la ventana mediante botón y completa en Android el código mostrado en OLED
- **THEN** el enlace protegido permite consultar estado y enviar una orden confirmada
- **AND** el material de emparejamiento no aparece en logs ni en mensajes MQTT

#### Scenario: Vínculos de una instalación anterior
- **WHEN** se detectan cuatro vínculos anteriores incompatibles con el límite de tres teléfonos
- **THEN** el dispositivo informa el problema y falla de forma segura antes de sobrescribir datos
- **AND** conserva los vínculos existentes sin borrarlos automáticamente

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

### Requirement: STA y MQTT seguros en el perfil Cebo
El dispositivo SHALL conectar a la red autorizada y al broker autenticado con validación TLS de identidad del servidor; publicar estado real y aceptar comandos autorizados en Cebo. SHALL reintentar fallos sin bloquear botón, pantalla o supervisor.

#### Scenario: Broker o contraseña incorrectos
- **WHEN** falla autenticación, certificado o conectividad del broker
- **THEN** el nodo informa el fallo y reintenta de forma acotada sin recurrir a TCP sin cifrar ni omitir la validación TLS

### Requirement: Escudo sin conectividad Wi-Fi activa
El perfil Escudo SHALL conservar BLE para control sin iniciar asociación, escaneo activo, MQTT directo ni servicios de pasarela. Las funciones pasivas disponibles SHALL operar mediante el gestor compartido conforme a las especificaciones de descubrimiento, captura, detección y recuperación de Escudo.

#### Scenario: Cambio desde un perfil conectado
- **WHEN** se aplica el perfil Escudo después de Cebo
- **THEN** cesa la conexión STA/MQTT y el control continúa o se reconcilia mediante BLE autorizado

### Requirement: Recuperación de transporte sin efectos duplicados
Tras reconexión los participantes SHALL recuperar estado y eventos disponibles con los mismos identificadores; no SHALL ejecutar automáticamente comandos vencidos ni conservar mensajes parciales de la sesión anterior.

#### Scenario: Pérdida y retorno del canal
- **WHEN** se desconecta y restablece el canal aplicable al perfil
- **THEN** se consulta el estado vigente y se recuperan pendientes según el cursor
- **AND** las funciones locales de la base siguen operando durante la desconexión
