# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

El repositorio conserva recetas ESP-IDF/PlatformIO y scripts propios que confunden el flujo Arduino. El usuario pide retirar el firmware anterior y evitar Python como paso del proyecto.

## What Changes

Eliminar configuración y entradas obsoletas, sustituir herramientas Python necesarias antes de retirarlas y conservar una sola fuente compilable. Mantener pruebas críticas y contratos sin instalaciones manuales para compilar desde el IDE.

## Capabilities

No cambia comportamiento contractual; `skip_specs: true`. Se revisan y conservan las specs funcionales aplicables; no se inventan requisitos de implementación.

## Impact

Archivos heredados firmware, herramientas, validadores distribuibles y runners de pruebas; no se borra lógica funcional ni se formatea hardware.
