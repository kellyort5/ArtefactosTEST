# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md), docs/brief.md, arquitectura/decisiones vigentes y specs aplicables. El usuario solicita simplificar y ejecutar, conservando todo el Escudo. Baseline local protegido en /tmp/sentinel-simplification-before.zip y su mapa SHA256; rama feat/sentinel-arduino-migration con trabajo previo sin commit.

## Goals / Non-Goals

Conservar alcance, seguridad, contratos y datos. Sin nuevas funcionalidades de módulos, frameworks innecesarios, benchmarks ni publicaciones Git.

## Decisions

Eliminar primero solo restos demostrablemente inactivos. Reutilizar Node24 ya presente para mantenimiento y tests del repositorio, sin agregar frameworks. El IDE recibe el validador C++ necesario como fuente distribuible revisada, con comprobación contra contratos fuera del build diario; no exige generar archivos al compilar. Se conserva este archivo como fuente necesaria, no como caché/binario. Mantener todos los casos críticos; sustituir runners, no borrar sus aserciones. La retirada del paquete SDK y sus scripts se coordina con dependencias; no romper el build durante el relevo. Herramientas internas del paquete oficial no equivalen a scripts Python propios exigidos al compañero.

### Propiedad exclusiva

firmware/tools/**, firmware/test/** (runners y soporte; casos C++ solo ajustes de rutas justificados), firmware/CMakeLists.txt, src/ legado, platformio.ini, requirements.txt, sdkconfig*, CMakeLists/Kconfig inactivos del sketch, .gitignore solo exclusiones pertinentes, y src/sentinel/v2_validator.inc como fuente derivada única. No tocar otros .cpp/.h del sketch, paquete SDK hasta coordinación, docs ni tasks.

## Risks / Trade-offs

Regresión o interferencia entre autores: interfaces existentes y propiedad por archivo, pruebas focalizadas y revisión del coordinador. Una limitación real se documenta y eleva; no se reduce el alcance para cerrar una tarea.

## Migration Plan

Aplicar incrementos pequeños y revisar contra el snapshot protegido. No descartar trabajo previo ni hacer commits, push, merge, archivo o borrados Git destructivos. Subida física solo si existe placa identificada; la falta de hardware se documenta sin inventar validación.
