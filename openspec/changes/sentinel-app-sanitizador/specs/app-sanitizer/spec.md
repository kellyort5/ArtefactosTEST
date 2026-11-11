# Spec Delta

## Purpose

R-08 necesita configuración BLE, resultados de auditoría, clientes/métricas y controles de reglas/DoH accesibles.

## ADDED Requirements

### Requirement: Configuración y auditoría

La app SHALL configurar upstream/AP/DoH por BLE y mostrar resultados DNS/HTTPS/portal con motivos y estados inconclusos.

#### Scenario: Configuración y auditoría verificable

- **WHEN** el nodo detecta portal o falta de hora
- **THEN** la pantalla explica el diagnóstico sin marcar protección activa

### Requirement: Control y métricas de pasarela

La app SHALL controlar la pasarela y mostrar clientes, RSSI/calidad, tráfico de subida/bajada y estado de NAT/DNS/DoH.

#### Scenario: Control y métricas de pasarela verificable

- **WHEN** un cliente real usa la pasarela y luego falla DoH
- **THEN** se ven métricas y degradación con error DNS

### Requirement: Reglas y cobertura

La app SHALL administrar reglas DNS y mostrar bloqueos y límites IPv4 sin conservar claves del formulario.

#### Scenario: Reglas y cobertura verificable

- **WHEN** se añade bloqueo y una excepción y se limpia la pantalla
- **THEN** ambas reglas se reflejan y no quedan claves persistidas
