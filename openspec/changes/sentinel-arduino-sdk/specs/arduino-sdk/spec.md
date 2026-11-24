# Spec Delta

## Purpose

Entregar una plataforma Arduino ESP32-S3 reproducible para el desarrollo Windows de Sentinel sin debilitar red, BLE, TLS o persistencia.

## ADDED Requirements

### Requirement: SDK con capacidades reales

El paquete SHALL proporcionar MQTT5, NimBLE SC-only nivel4, TLS con comprobación de CA/nombre/fechas, forwarding/NAPT y memoria N16R8 mediante bibliotecas compiladas coherentes.

#### Scenario: Rechazo de SDK incompatible

- **WHEN** se intenta usar el SDK estándar sin las capacidades exigidas
- **THEN** la verificación/build falla explícitamente y no se oculta cambiando defines locales.

### Requirement: Arranque no destructivo

El core SHALL conservar NVS ante errores de inicialización y no formatear identidad, bonds o historial automáticamente.

#### Scenario: Error NVS antes de setup

- **WHEN** NVS no puede inicializarse
- **THEN** el core registra el fallo sin borrar particiones y Sentinel conserva un estado seguro.

### Requirement: Instalación Windows aislada

El paquete SHALL fijar versiones/checksums, instalarse sin reemplazar el core oficial y usar herramientas nativas Windows compatibles con Arduino IDE.

#### Scenario: Compilación nativa verificable

- **WHEN** se instala la distribución preparada y se verifica el ejemplo de capacidades en Windows
- **THEN** la salida identifica plataforma/SDK esperados y enlaza las APIs requeridas; los binarios no se versionan en Git.
