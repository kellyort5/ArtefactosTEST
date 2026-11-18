# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

La base Arduino funciona, pero concentra responsabilidades y contiene código compacto difícil de mantener por el equipo. El usuario solicita modularidad, legibilidad y comentarios útiles sin perder el Escudo.

## What Changes

Separar responsabilidades internas, reducir duplicación real, usar nombres claros y comentar decisiones, límites y concurrencia. Conservar API pública, contratos, detectores, persistencia y comportamiento.

La integración de dependencias añade una señal interna de cierre pendiente para conservar transiciones no bloqueantes y seguras; se prueba sin cambiar la API de módulos ni el comportamiento contractual.

## Capabilities

No cambia comportamiento contractual; `skip_specs: true`. Se revisan y conservan las specs funcionales aplicables; no se inventan requisitos de implementación.

## Impact

Núcleo portable, runtime y módulo Escudo dentro de firmware/SentinelNode/src/. Adaptadores de radio/BLE/MQTT pertenecen al cambio de dependencias.
