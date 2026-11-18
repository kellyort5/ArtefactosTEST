# Design

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md), docs/brief.md, arquitectura/decisiones vigentes y specs aplicables. El usuario solicita simplificar y ejecutar, conservando todo el Escudo. Baseline local protegido en /tmp/sentinel-simplification-before.zip y su mapa SHA256; rama feat/sentinel-arduino-migration con trabajo previo sin commit.

## Goals / Non-Goals

Conservar alcance, seguridad, contratos y datos. Sin nuevas funcionalidades de módulos, frameworks innecesarios, benchmarks ni publicaciones Git.

## Decisions

Esperar contratos concretos y evidencias de los tres responsables para redactar el flujo final. Priorizar abrir sketch, instalar dependencias mantenidas, elegir placa/opciones, Verificar, Subir y Monitor desde Arduino IDE Windows. La guía de Persona1 incluye cableado, BLE/appv2, descubrimiento, captura, cuatro detectores, recuperación, resultados esperados y registro de pendientes. Completar specs faltantes revisando deltas existentes y código, sin inventar decisiones ni marcar tareas físicas; los archivos históricos se conservan como evidencia identificada. No archivar cambios ni ejecutar Git de publicación.

### Propiedad exclusiva

docs/**, README.md, CONTRIBUTING.md y coherencia de AGENTS.md; README del sketch después de dependencias; openspec/specs/** y artefactos de planificación pertinentes salvo tasks.md de otros autores. No tocar app/broker/contracts/notifications/simulator ni fuentes.

### Reconciliación de especificaciones

La auditoría de preparación encontró las 14 capacidades archivadas en el principal; las capacidades de cambios futuros siguen siendo deltas, no funcionalidades entregadas. Se revisaron los reemplazos exactos de preparation.md contra las specs completas. Este cambio incorpora deltas MODIFIED para cinco capacidades existentes, conservando todos sus escenarios y requisitos no afectados. Sustituye las prohibiciones heredadas de la base inicial por disponibilidad real según el registro de módulos y las specs aprobadas, sin habilitar servicios ni pantallas pendientes. El límite de tres teléfonos y el fallo seguro ante cuatro vínculos anteriores responden a la aprobación explícita del usuario. La alternativa de copiar todos los deltas activos daría por entregados módulos futuros y el SDK propio sustituido, por lo que se descarta. No se acredita ninguna prueba física mediante esta reconciliación.

## Risks / Trade-offs

Regresión o interferencia entre autores: interfaces existentes y propiedad por archivo, pruebas focalizadas y revisión del coordinador. Una limitación real se documenta y eleva; no se reduce el alcance para cerrar una tarea.

## Migration Plan

Aplicar incrementos pequeños y revisar contra el snapshot protegido. No descartar trabajo previo ni hacer commits, push, merge, archivo o borrados Git destructivos. Subida física solo si existe placa identificada; la falta de hardware se documenta sin inventar validación.

Los deltas de requisitos se alinean con la fuente vigente para no restaurar cuentas/TLS ni retirar escenarios P1 en una sincronización futura. Los resultados históricos y las fechas de esta entrega se conservan; el cambio posterior sentinel-mvp-sin-cuentas establece el alcance actual.
