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

### Requirement: Configuración endpoint y confianza DoH persistente

Sanitizador SHALL conservar configuración endpoint y confianza DoH autorizada en NVS oficial durante uso/reinicio/corte, validar límites, confirmar solo tras commit y conservar la anterior ante fallo. SHALL restaurar en Idle sin iniciar servicios ni confiar en UTC anterior. USB permite reinicialización sin migración; sin promesa de cifrado físico ni credenciales de terceros o secretos en eventos/logs/estado público.

#### Scenario: Recuperación de configuración confirmada sin USB
- **WHEN** se reinicia o recupera alimentación después de confirmar persistencia de configuración endpoint y confianza DoH
- **THEN** recupera la última configuración válida sin iniciar STA/AP/NAT/DNS/DoH automáticamente
- **AND** la caché de respuestas y las observaciones de clientes siguen siendo transitorias

#### Scenario: Fallo de guardado de configuración
- **WHEN** no puede completarse la escritura y commit de la nueva configuración
- **THEN** informa fallo sin confirmar aplicación y conserva la configuración válida anterior sin formato automático ni secretos expuestos
