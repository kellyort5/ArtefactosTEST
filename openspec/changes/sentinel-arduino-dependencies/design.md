# Design

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md), docs/brief.md, arquitectura/decisiones vigentes y specs aplicables. El usuario solicita simplificar y ejecutar, conservando todo el Escudo. Baseline local protegido en /tmp/sentinel-simplification-before.zip y su mapa SHA256; rama feat/sentinel-arduino-migration con trabajo previo sin commit.

## Goals / Non-Goals

Conservar alcance, seguridad, contratos y datos. Sin nuevas funcionalidades de módulos, frameworks innecesarios, benchmarks ni publicaciones Git.

## Decisions

Destino confirmado para esta implementación: Arduino IDE Windows con ESP32S3 Dev Module oficial, flash16MiB y PSRAM OPI8MiB, y dependencias existentes fijadas. No desarrollar criptografía, clientes MQTT o drivers equivalentes a bibliotecas mantenidas. La elección exacta de versiones es tarea de compatibilidad breve. Mantener MQTT5, TLS con CA/nombre/fechas, BLE autenticado SC, journal e interfaces salvo revisión explícita del usuario de un impedimento concreto. No bajar garantías ni cambiar contratos/particiones en silencio. El responsable documenta la alternativa concreta y eleva al coordinador los cambios arquitectónicos necesarios antes de implementarlos.

### Adaptación aprobada por el usuario

El usuario aprobó explícitamente core oficial, hasta tres teléfonos recordados y la configuración previa de nueve claves, ahora sustituida por cuatro y retirada de cinco overrides propios con recetas oficiales, con las dos bibliotecas mantenidas ya probadas en un sketch oficial. La opción se instala una vez por versión, preserva claves ajenas y backup y no copia/reconstruye SDK. Una unidad con cuatro vínculos previos debe detectarse antes de escritura o evicción; preservar material y comunicar incompatibilidad, sin inventar otro store. MQTT5, TLS CA/nombre/fechas y BLE SC/MITM se mantienen.

La integración usa cierre MQTT asíncrono para no bloquear el supervisor. El dueño del núcleo añade Resources::network_stopping() con valor predeterminado false: solo un cierre expresamente pendiente conserva transitioning hasta ACK o el deadline existente de 5000ms. Otros fallos conservan error y ningún destino arranca antes de liberar recursos. Se mantiene Module API y los contratos externos; sus pruebas pertenecen al cambio de legibilidad.

### Propiedad exclusiva

SentinelNode.ino, SentinelConfig.h, partitions.csv; src/sentinel/ble.cpp, network.cpp, peripherals.cpp, arduino_requirements.cpp e include/sentinel.h, network.h, arduino_environment.h; nuevos adaptadores necesarios; firmware/arduino-package/** para retirada, firmware/arduino/** para la metadata mínima y firmware/SentinelNode/README.md para dependencias. No tocar runtime/portable/Escudo, tools/test, docs ni tasks.

## Risks / Trade-offs

Regresión o interferencia entre autores: interfaces existentes y propiedad por archivo, pruebas focalizadas y revisión del coordinador. Una limitación real se documenta y eleva; no se reduce el alcance para cerrar una tarea.

## Migration Plan

Aplicar incrementos pequeños y revisar contra el snapshot protegido. No descartar trabajo previo ni hacer commits, push, merge, archivo o borrados Git destructivos. Subida física solo si existe placa identificada; la falta de hardware se documenta sin inventar validación.
