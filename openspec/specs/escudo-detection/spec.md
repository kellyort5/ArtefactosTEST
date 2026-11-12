# escudo-detection Specification

## Purpose

Las observaciones necesitan detección completa y explicable; un solo detector no cumple R-03.

## Requirements

### Requirement: Detección de desconexiones

Escudo SHALL detectar ráfagas de desautenticación y desconexiones sospechosas mediante ventanas y emitir evidencia cuantificada.

#### Scenario: Detección de desconexiones verificable

- **WHEN** se supera y luego no se supera el umbral con observaciones válidas
- **THEN** solo el caso que lo supera emite el indicio correspondiente

### Requirement: Evil Twin con contexto

Escudo SHALL comparar BSSID del mismo SSID, canal, RSSI y seguridad sin clasificar como malicioso un SSID duplicado por sí solo.

#### Scenario: Evil Twin con contexto verificable

- **WHEN** dos AP legítimos comparten SSID sin otras diferencias sospechosas
- **THEN** se muestran ambos y no se declara un ataque

### Requirement: Indicios Karma pasivos

Escudo SHALL evaluar indicios compatibles con Karma únicamente con evidencia pasiva y explicar insuficiencia de evidencia y límites.

#### Scenario: Indicios Karma pasivos verificable

- **WHEN** se observan respuestas correlacionadas a varios SSID y un caso benigno
- **THEN** se diferencia el indicio del caso benigno sin generar tráfico activo

### Requirement: Alertas integradas

Cada detector SHALL publicar eventos y métricas reales con identidad, red, canal y tiempo/calidad por BLE.

#### Scenario: Alertas integradas verificable

- **WHEN** un detector conectado al flujo captura un patrón
- **THEN** la app recibe un evento tipado y la métrica aumenta sin atribuir un atacante
