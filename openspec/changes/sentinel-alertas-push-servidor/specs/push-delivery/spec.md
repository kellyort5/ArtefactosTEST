# Spec Delta

## Purpose

La recepción con Android cerrado requiere proveedor push y registro seguro de instalaciones, además de alertas MQTT.

## ADDED Requirements

### Requirement: Registro autenticado de instalaciones

El servidor SHALL registrar, renovar y eliminar tokens de instalaciones autorizadas por nodo con retención y capacidad limitadas.

#### Scenario: Registro autenticado de instalaciones verificable

- **WHEN** una instalación renueva token y otra solicita un nodo no permitido
- **THEN** se sustituye el token propio y se rechaza la identidad cruzada

### Requirement: Push correlacionado

El servidor SHALL enviar push visible con identificación del incidente y minimizar los datos enviados al proveedor.

#### Scenario: Push correlacionado verificable

- **WHEN** ocurre un incidente real Cebo y existe instalación registrada
- **THEN** se envía una notificación con ID original y sin datos sensibles

### Requirement: Errores de entrega

El servidor SHALL retirar tokens inválidos y distinguir aceptación del proveedor de recepción comprobada en teléfono.

#### Scenario: Errores de entrega verificable

- **WHEN** el proveedor rechaza un token o acepta el envío
- **THEN** se actualiza estado sin marcar lectura ni recepción inexistentes
