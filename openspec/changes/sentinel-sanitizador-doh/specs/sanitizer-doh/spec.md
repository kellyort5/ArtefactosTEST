# Spec Delta

## Purpose

Completar la ruta real del DNS local con cifrado TLS y recuperación sin fallback plano.

## ADDED Requirements

### Requirement: DNS mediante DoH validado

Sanitizador SHALL reenviar consultas permitidas a un endpoint DoH HTTPS validado y devolver respuestas DNS coherentes a sus clientes.

#### Scenario: DNS mediante DoH validado verificable

- **WHEN** un cliente del AP resuelve un nombre permitido
- **THEN** recibe la respuesta a través de DoH con validación TLS y la métrica correspondiente

### Requirement: Fallo cerrado DNS

Sanitizador SHALL devolver error DNS y declarar degradación si falla DoH sin recurrir automáticamente a DNS plano.

#### Scenario: Fallo cerrado DNS verificable

- **WHEN** el endpoint presenta certificado inválido o deja de responder
- **THEN** el cliente recibe SERVFAIL y el estado identifica el fallo

### Requirement: Protección integrada verificable

Sanitizador SHALL declarar protección activa solo con upstream, AP/NAT, DNS filtrado y DoH operativos y permitir control BLE.

#### Scenario: Protección integrada verificable verificable

- **WHEN** un cliente navega y bloquea un dominio mientras BLE consulta estado
- **THEN** los componentes funcionan juntos y el bloqueo/estado se reflejan realmente
