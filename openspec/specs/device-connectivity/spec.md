# device-connectivity Specification

## Purpose

Conectar la base de Sentinel Node con la aplicación por BLE autenticado y MQTT anónimo, conservando las restricciones de radio propias de cada perfil operativo.

## Requirements

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
