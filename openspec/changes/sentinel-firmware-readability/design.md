# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md), docs/brief.md, arquitectura/decisiones vigentes y specs aplicables. El usuario solicita simplificar y ejecutar, conservando todo el Escudo. Baseline local protegido en /tmp/sentinel-simplification-before.zip y su mapa SHA256; rama feat/sentinel-arduino-migration con trabajo previo sin commit.

## Goals / Non-Goals

Conservar alcance, seguridad, contratos y datos. Sin nuevas funcionalidades de módulos, frameworks innecesarios, benchmarks ni publicaciones Git.

## Decisions

Refactorizar por responsabilidades concretas, sin frameworks ni capas genéricas nuevas. Mantener un supervisor, contexto y journal. Los comentarios se redactan en español y explican motivos, propiedad y límites; no narran cada línea. Includes relativos compatibles con el Arduino oficial. La validación de contratos no se debilita.

### Propiedad exclusiva

src/sentinel/runtime.cpp, core.cpp, wire.cpp, digest.cpp, protocol_v2.cpp, module_runtime.cpp, optional_modules.cpp; sus headers portables salvo sentinel.h/network.h/arduino_environment.h; src/escudo/*.cpp y sus headers; CMake/Kconfig inactivos quedan bajo limpieza; nuevos archivos internos estrictamente necesarios. No tocar adaptadores, herramientas, pruebas C++ ajenas, docs ni tasks.

## Risks / Trade-offs

Regresión o interferencia entre autores: interfaces existentes y propiedad por archivo, pruebas focalizadas y revisión del coordinador. Una limitación real se documenta y eleva; no se reduce el alcance para cerrar una tarea.

## Migration Plan

Aplicar incrementos pequeños y revisar contra el snapshot protegido. No descartar trabajo previo ni hacer commits, push, merge, archivo o borrados Git destructivos. Subida física solo si existe placa identificada; la falta de hardware se documenta sin inventar validación.

## Integración necesaria del cierre asíncrono

Como parte del relevo a bibliotecas mantenidas, se añade una señal interna de parada de red pendiente con default false. ModuleSupervisor conserva transición, lease y deadline mientras el worker cierra socket/tarea; un false por error real mantiene el tratamiento anterior. No cambia Module API, seguridad, contratos ni el timeout de 5000ms. El autor de código posee core.h/module_runtime.cpp y un nuevo caso independiente network_stop_test.cpp; limpieza registra ese caso en su runner reutilizado. Los tests C++ previos permanecen intactos. Verificar ACK tardío, exclusión, fallo real, timeout y reintento desde Idle sin declarar liberación anticipada.
