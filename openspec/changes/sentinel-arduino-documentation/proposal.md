# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

Las guías y algunas specs describen la plataforma propia y validaciones históricas. El equipo necesita instrucciones coherentes con el firmware simplificado y una guía desde cero para Persona 1.

## What Changes

Actualizar documentación, decisiones, encargos, guías y specs faltantes a partir del resultado comprobado de los otros tres cambios. Conservar evidencia histórica y pendientes reales, sin declarar capacidades o pruebas no realizadas. Reconciliar los requisitos de la base inicial con module-dispatch, module-protocol y los incrementos de Escudo ya aprobados; reflejar el límite de tres teléfonos aprobado por el usuario, conservando los vínculos anteriores sin borrado automático.

## Capabilities

### New Capabilities

Ninguna. Los módulos futuros conservan sus cambios activos; no se copian como entregados.

### Modified Capabilities

- `device-runtime`: disponibilidad real de módulos registrados, sin prohibir el Escudo ya incorporado.
- `device-connectivity`: radio pasiva conforme a las specs de Escudo y límite aprobado de tres teléfonos con preservación de datos previos.
- `message-contracts`: versiones soportadas y negociadas, eventos históricos v1 intactos y capacidades reales.
- `project-workspace`: distinguir la infraestructura compartida de los módulos realmente disponibles.
- `mobile-control`: presentar como pendientes únicamente las capacidades o pantallas todavía ausentes.

## Impact

docs/**, README.md, CONTRIBUTING.md, AGENTS.md solo si procede, firmware/SentinelNode/README.md una vez liberado por dependencias, y specs/artefactos OpenSpec necesarios. No modificar código ni casillas de otros cambios.

Los deltas de requisitos se alinean con la fuente vigente para no restaurar cuentas/TLS ni retirar escenarios P1 en una sincronización futura. Los resultados históricos y las fechas de esta entrega se conservan; el cambio posterior sentinel-mvp-sin-cuentas establece el alcance actual.
