## Purpose

Proporcionar un intermediario MQTT reproducible para dispositivo y aplicación, con cifrado, autenticación y aislamiento de permisos sin exponer credenciales administrativas.

## ADDED Requirements

### Requirement: Listeners cifrados sin acceso anónimo
El broker SHALL ofrecer MQTT/TLS y MQTT sobre WSS con TLS 1.2 o superior, autenticación obligatoria y certificados verificables. No SHALL habilitar listeners de acceso sin cifrar o anónimo como alternativa de desarrollo.

#### Scenario: Arranque con configuración privada
- **WHEN** se inicia el broker con certificados y credenciales locales válidos
- **THEN** el nodo y la app pueden conectar mediante sus listeners cifrados
- **AND** una conexión anónima o con contraseña incorrecta se rechaza

#### Scenario: Material privado ausente
- **WHEN** faltan certificados, claves o archivo de usuarios
- **THEN** el arranque falla con instrucciones de configuración sin generar credenciales públicas ni habilitar acceso inseguro

### Requirement: Permisos separados por identidad y nodo
El broker SHALL restringir cada nodo a publicar su estado, métricas, eventos y confirmaciones y a recibir sus órdenes. Los observadores SHALL ser de solo lectura y los operadores solo SHALL ordenar sobre nodos autorizados.

#### Scenario: Publicación fuera de alcance
- **WHEN** un nodo publica con la identidad de otro nodo o un observador publica un comando
- **THEN** el broker deniega la operación y no entrega el mensaje a otros clientes

### Requirement: Configuración y datos privados fuera del repositorio
El despliegue SHALL separar configuración fuente, datos persistentes y secretos locales; generar usuarios y certificados de laboratorio sin guardar contraseñas, claves privadas o contenido sensible en Git o logs.

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
