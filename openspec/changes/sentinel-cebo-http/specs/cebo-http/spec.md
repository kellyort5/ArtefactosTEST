# Spec Delta

## Purpose

La base Cebo solo conecta MQTT; falta un servicio HTTP real seguro que genere interacciones normalizadas.

## ADDED Requirements

### Requirement: HTTP real ficticio

Cebo SHALL servir un panel HTTP ficticio en la LAN autorizada y detectar conexiones, accesos e intentos de autenticación.

#### Scenario: HTTP real ficticio verificable

- **WHEN** un cliente propio abre el panel y envía un formulario ficticio
- **THEN** recibe respuesta segura y se producen metadatos de acceso y autenticación

### Requirement: Privacidad HTTP

Cebo SHALL excluir credenciales, tokens, cookies y cuerpos completos de sus registros y no ejecutar acciones administrativas.

#### Scenario: Privacidad HTTP verificable

- **WHEN** se envía un marcador ficticio como contraseña y query
- **THEN** el marcador no aparece en eventos, logs, journal ni respuestas reflejadas

### Requirement: HTTP limitado al modo

El servicio SHALL limitar recursos y cerrar todas sus conexiones al detener Cebo.

#### Scenario: HTTP limitado al modo verificable

- **WHEN** se agota el límite o se cambia de modo
- **THEN** se rechaza el exceso y no queda un listener HTTP activo
