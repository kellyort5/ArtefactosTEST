# Proposal

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

> **Antecedente sustituido:** este cambio registra el SDK propio que se construyó antes de la simplificación aprobada. El destino vigente es el core oficial 3.3.12, tres teléfonos recordados y configuración mínima de protección de datos de [sentinel-arduino-dependencies](../sentinel-arduino-dependencies/design.md). No reinstalar ni reconstruir este paquete; no sincronizar su capacidad `arduino-sdk` como requisito actual. Las evidencias y casillas anteriores conservan su alcance histórico.

## Why

Arduino-ESP32 estándar no incluye las opciones MQTT5/TLS/SC-only exigidas y su arranque puede borrar NVS. El usuario autorizó ejecutar la migración completa a Arduino IDE Windows conservando Escudo y todas las garantías.

## What Changes

Construir e instalar un paquete aislado para ESP32-S3 N16R8 con versiones fijadas, SDK seguro, política NVS no destructiva y herramientas nativas Windows. La preparación externa del SDK es una fase del port, no el uso diario. No modificar contratos, módulos ni servicios externos.

## Capabilities

### New Capabilities

- `arduino-sdk`: paquete verificable con MQTT5, TLS con fechas, NimBLE SC-only, NAPT y arranque sin formato automático.

### Modified Capabilities

Ninguna.

## Impact

`firmware/arduino-package/`, instalador/verificador/builder del paquete en `firmware/tools/` y evidencia OpenSpec. Dependencias: fuentes oficiales, toolchain y espacio local. Riesgos: ABI, bibliotecas precompiladas incompletas, arranque previo a setup y distribución Windows. No publicar artefactos externos ni ejecutar operaciones Git sin autorización.

Fuente de ejecución: [plan de migración](../../../docs/firmware-arduino-migration.md). Responsable humano: P1; la autorización del usuario cubre implementación, no commits/push/merge.
