# Spec Delta

## MODIFIED Requirements

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

### Requirement: Escudo sin conectividad Wi-Fi activa
El perfil Escudo SHALL conservar BLE para control sin iniciar asociación, escaneo activo, MQTT directo ni servicios de pasarela. Las funciones pasivas disponibles SHALL operar mediante el gestor compartido conforme a las especificaciones de descubrimiento, captura, detección y recuperación de Escudo.

#### Scenario: Cambio desde un perfil conectado
- **WHEN** se aplica el perfil Escudo después de Cebo
- **THEN** cesa la conexión STA/MQTT y el control continúa o se reconcilia mediante BLE autorizado
