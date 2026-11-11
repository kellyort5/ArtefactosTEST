# Spec Delta

## Purpose

R-08 requiere diagnosticar DNS, HTTPS y portal cautivo antes de habilitar una pasarela protegida.

## ADDED Requirements

### Requirement: Conexión para auditar

Sanitizador SHALL configurar la red de salida por BLE autorizado y conectarse sin activar todavía una pasarela protegida.

#### Scenario: Conexión para auditar verificable

- **WHEN** se configura la red propia y se solicita auditoría
- **THEN** el nodo conecta STA y muestra auditoría en progreso sin afirmar protección

### Requirement: Auditoría explicable

Sanitizador SHALL comprobar DNS, certificados HTTPS y posible portal cautivo con resultados y evidencia por prueba.

#### Scenario: Auditoría explicable verificable

- **WHEN** falla DNS o se detecta una redirección de portal
- **THEN** la app recibe el diagnóstico diferenciado y sus limitaciones

### Requirement: Verificación TLS obligatoria

Sanitizador SHALL validar cadena, hostname y vigencia y reportar falta de hora sin omitir la validación.

#### Scenario: Verificación TLS obligatoria verificable

- **WHEN** se presenta certificado inválido o reloj desconocido
- **THEN** no se declara HTTPS validado y se informa el motivo
