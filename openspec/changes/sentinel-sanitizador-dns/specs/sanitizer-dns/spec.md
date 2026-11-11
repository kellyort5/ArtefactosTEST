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
