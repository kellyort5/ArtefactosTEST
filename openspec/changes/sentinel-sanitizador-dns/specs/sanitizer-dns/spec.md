# Spec Delta

## Purpose

El tráfico DNS atendido por la pasarela debe aplicar reglas, caché y contadores antes de integrar el reenvío cifrado.

## ADDED Requirements

### Requirement: Resolución local filtrada

Sanitizador SHALL atender DNS de clientes SoftAP y bloquear dominios por reglas exactas o de subdominio con precedencia explícita.

#### Scenario: Resolución local filtrada verificable

- **WHEN** se consulta dominio bloqueado y una excepción permitida
- **THEN** el primero devuelve NXDOMAIN y el segundo sigue la ruta permitida sin falso match de sufijo

### Requirement: Caché y límites

El DNS SHALL respetar TTL, acotar consultas y rechazar paquetes malformados sin bloquear el control del dispositivo.

#### Scenario: Caché y límites verificable

- **WHEN** llega una consulta truncada o la caché expira
- **THEN** se rechaza la primera y no se devuelve una respuesta vencida

### Requirement: Sin degradación a DNS abierto

Sanitizador SHALL informar fallo cuando no exista un resolvedor cifrado disponible y contar consultas, bloqueos y fallos.

#### Scenario: Sin degradación a DNS abierto verificable

- **WHEN** un nombre permitido requiere upstream aún no configurado
- **THEN** devuelve SERVFAIL visible y no envía DNS sin cifrar

### Requirement: Configuración DNS y reglas persistente

Sanitizador SHALL conservar configuración DNS y reglas autorizada en NVS oficial durante uso/reinicio/corte, validar límites, confirmar solo tras commit y conservar la anterior ante fallo. SHALL restaurar en Idle sin iniciar servicios ni confiar en UTC anterior. USB permite reinicialización sin migración; sin promesa de cifrado físico ni credenciales de terceros o secretos en eventos/logs/estado público.

#### Scenario: Recuperación de configuración confirmada sin USB
- **WHEN** se reinicia o recupera alimentación después de confirmar persistencia de configuración DNS y reglas
- **THEN** recupera la última configuración válida sin iniciar STA/AP/NAT/DNS/DoH automáticamente
- **AND** la caché de respuestas y las observaciones de clientes siguen siendo transitorias

#### Scenario: Fallo de guardado de configuración
- **WHEN** no puede completarse la escritura y commit de la nueva configuración
- **THEN** informa fallo sin confirmar aplicación y conserva la configuración válida anterior sin formato automático ni secretos expuestos
