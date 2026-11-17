# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

El paquete SDK propio resolvió compatibilidad, pero añade mantenimiento y preparación que el usuario quiere evitar. Se solicita aprovechar dependencias existentes y simplificar Arduino IDE.

## What Changes

Adoptar paquete oficial ESP32 y bibliotecas mantenidas cuando permitan conservar funciones y garantías. Fijar versiones y opciones de placa, adaptar transportes/periféricos y eliminar la necesidad de construir un SDK propio.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `project-workspace`: compilación autónoma del firmware con dependencias distribuidas, sin SDK propio ni scripts manuales.

## Impact

Entry point, adaptadores BLE/Wi-Fi/MQTT/periféricos, configuración de placa y dependencias. No modificar app, broker, contratos externos ni lógica del Escudo.
