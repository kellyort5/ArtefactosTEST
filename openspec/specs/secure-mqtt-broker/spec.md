# secure-mqtt-broker Specification

## Purpose

Proporcionar un intermediario MQTT reproducible para dispositivo y aplicación, con autenticación y aislamiento de permisos sin exponer credenciales administrativas.

## Requirements

### Requirement: Listeners LAN autenticados sin certificados
El broker SHALL ofrecer MQTT5/TCP en 1883 y MQTT5/WS en 9001 para redes de confianza autorizadas, con autenticación obligatoria y sin certificados. SHALL permitir acceso LAN sin cambios de router y documentar que el tráfico no está cifrado.

#### Scenario: Arranque con configuración privada
- **WHEN** se inicia el broker con credenciales locales válidas
- **THEN** el nodo y la app pueden conectar mediante TCP y WS sin CA
- **AND** una conexión anónima o con contraseña incorrecta se rechaza

#### Scenario: Material privado ausente
- **WHEN** falta el archivo de usuarios o ACL
- **THEN** el arranque falla con instrucciones de configuración sin generar credenciales públicas ni habilitar acceso anónimo

#### Scenario: Acceso desde teléfono
- **WHEN** el teléfono y el PC pueden comunicarse en una red de confianza
- **THEN** la app accede al broker usando la IP LAN del PC sin instalar certificados ni configurar DNS o el router
- **AND** las instrucciones distinguen firewall local y aislamiento de clientes de fallos de autenticación

### Requirement: Permisos separados por identidad y nodo
El broker SHALL restringir cada nodo a publicar su estado, métricas, eventos y confirmaciones y a recibir sus órdenes. Los observadores SHALL ser de solo lectura y los operadores solo SHALL ordenar sobre nodos autorizados.

#### Scenario: Publicación fuera de alcance
- **WHEN** un nodo publica con la identidad de otro nodo o un observador publica un comando
- **THEN** el broker deniega la operación y no entrega el mensaje a otros clientes

### Requirement: Configuración y datos privados fuera del repositorio
El despliegue SHALL separar configuración fuente, datos persistentes y secretos locales; generar usuarios de laboratorio sin requerir certificados, sin guardar contraseñas, claves privadas o contenido sensible en Git o logs.

#### Scenario: Preparación de un entorno
- **WHEN** el equipo sigue las instrucciones de configuración
- **THEN** crea su propio material privado fuera del seguimiento de Git y conserva configuraciones y ACL revisables
- **AND** no recibe una cuenta administrativa incorporada al frontend

### Requirement: Persistencia MQTT con alcance explícito
El broker SHALL conservar su estado MQTT configurado en almacenamiento persistente entre reinicios y documentar que no reemplaza el registro local del dispositivo ni un archivo histórico de incidentes.

#### Scenario: Reinicio del contenedor
- **WHEN** se reinicia el broker conservando el almacenamiento configurado
- **THEN** el estado retenido puede recuperarse al reconectar y el nodo informa disponibilidad actualizada
- **AND** ningún cliente interpreta un estado retenido antiguo como prueba de conexión actual
