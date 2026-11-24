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
- **AND** el material de emparejamiento no aparece en logs persistentes, historial ni mensajes MQTT; la salida local de pruebas solo permite el código temporal conforme al escenario siguiente

#### Scenario: Vínculos de una instalación anterior
- **WHEN** se detectan cuatro vínculos anteriores incompatibles con el límite de tres teléfonos
- **THEN** el dispositivo informa el problema y falla de forma segura antes de sobrescribir datos
- **AND** conserva los vínculos existentes sin borrarlos automáticamente

#### Scenario: Código local sin OLED en modo de pruebas
- **WHEN** se habilita explícitamente la visualización serie, el usuario abre la ventana con el botón y comienza un emparejamiento real
- **THEN** muestra el código temporal solo por la consola local y permite completar el mismo emparejamiento autenticado y cifrado
- **AND** fuera de ventana o si falla toda salida local rechaza el intento sin Just Works, código fijo ni autorización remota
- **AND** el código no se incorpora a historial, MQTT, formularios o evidencia pública; contraseñas y material de bonds nunca se imprimen

#### Scenario: Configuración de producto por defecto
- **WHEN** la opción de pruebas no está habilitada
- **THEN** conserva el flujo OLED original y nunca muestra el código por serie

### Requirement: Escudo sin conectividad Wi-Fi activa
El perfil Escudo SHALL conservar BLE para control sin iniciar asociación, escaneo activo, MQTT directo ni servicios de pasarela. Las funciones pasivas disponibles SHALL operar mediante el gestor compartido conforme a las especificaciones de descubrimiento, captura, detección y recuperación de Escudo.

#### Scenario: Cambio desde un perfil conectado
- **WHEN** se aplica el perfil Escudo después de Cebo
- **THEN** cesa la conexión STA/MQTT y el control continúa o se reconcilia mediante BLE autorizado
