# Spec Delta

## Purpose

R-10 incluye puerto 23 y autenticación simulada además de HTTP; ambos son obligatorios.

## ADDED Requirements

### Requirement: Servicio Telnet real

Cebo SHALL aceptar sesiones Telnet en puerto 23 y detectar conexión, entrada e intento de autenticación sin ejecutar comandos.

#### Scenario: Servicio Telnet real verificable

- **WHEN** un cliente Telnet propio introduce datos ficticios
- **THEN** recibe respuesta simulada y se genera metadato sin ejecutar la entrada

### Requirement: Privacidad Telnet

El servicio SHALL descartar líneas/credenciales y no almacenarlas ni reflejarlas en eventos.

#### Scenario: Privacidad Telnet verificable

- **WHEN** se envía un marcador secreto ficticio fragmentado
- **THEN** el evento contiene solo interacción y origen y el marcador no aparece en logs ni almacenamiento

### Requirement: Límites de sesiones

Cebo SHALL acotar sesiones, líneas y tiempos y liberar HTTP/Telnet al abandonar el modo.

#### Scenario: Límites de sesiones verificable

- **WHEN** llega una línea excesiva o se cierra el modo
- **THEN** se limita/rechaza la entrada y los dos listeners quedan cerrados
