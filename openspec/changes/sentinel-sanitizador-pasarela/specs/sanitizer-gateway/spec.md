# Spec Delta

## Purpose

La protección necesita un cliente conectado a Sentinel_Secure con salida IPv4 y estado/contadores reales.

## ADDED Requirements

### Requirement: Pasarela autenticada IPv4

Sanitizador SHALL ofrecer SoftAP autenticado, DHCP y NAT IPv4 hacia la red autorizada con subredes sin solapamiento.

#### Scenario: Pasarela autenticada IPv4 verificable

- **WHEN** un cliente se conecta con la clave configurada
- **THEN** recibe IP y puede alcanzar un endpoint IPv4 autorizado a través de NAT

### Requirement: Clientes y tráfico

Sanitizador SHALL informar clientes, IP/MAC, RSSI observado y bytes de subida/bajada, identificando cualquier dato no observable.

#### Scenario: Clientes y tráfico verificable

- **WHEN** un cliente envía y recibe tráfico de prueba
- **THEN** su registro muestra ambos contadores y RSSI real o desconocido explicado

### Requirement: Estado y parada segura

Sanitizador SHALL distinguir pasarela activa de protección DNS completa y cerrar AP/DHCP/NAT al abandonar el modo.

#### Scenario: Estado y parada segura verificable

- **WHEN** se pierde upstream o se cambia a Escudo
- **THEN** se muestra degradación o se liberan todos los servicios antes del siguiente modo

### Requirement: Configuración AP y upstream persistente

Sanitizador SHALL conservar configuración AP y upstream autorizada en NVS oficial durante uso/reinicio/corte, validar límites, confirmar solo tras commit y conservar la anterior ante fallo. SHALL restaurar en Idle sin iniciar servicios ni confiar en UTC anterior. USB permite reinicialización sin migración; sin promesa de cifrado físico ni credenciales de terceros o secretos en eventos/logs/estado público.

#### Scenario: Recuperación de configuración confirmada sin USB
- **WHEN** se reinicia o recupera alimentación después de confirmar persistencia de configuración AP y upstream
- **THEN** recupera la última configuración válida sin iniciar STA/AP/NAT/DNS/DoH automáticamente
- **AND** la caché de respuestas y las observaciones de clientes siguen siendo transitorias

#### Scenario: Fallo de guardado de configuración
- **WHEN** no puede completarse la escritura y commit de la nueva configuración
- **THEN** informa fallo sin confirmar aplicación y conserva la configuración válida anterior sin formato automático ni secretos expuestos
