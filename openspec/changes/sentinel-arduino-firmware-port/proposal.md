# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Why

El usuario solicita Arduino IDE Windows para desarrollar y subir todo el firmware sin perder ningún avance del Escudo. El CMake actual, app_main y configuración requieren adaptación conservando los módulos y contratos.

## What Changes

Trasladar el árbol editable a un sketch modular, conservar API y lógica de Escudo, distribuir el validador local sin generación durante el build, adaptar inicialización, tests y documentación de todos los módulos. Compilar realmente desde herramientas Arduino Windows y preparar subida por COM; pruebas físicas solo con dispositivos detectados. No implementar por conveniencia los servicios ausentes de otros compañeros.

## Capabilities

### New Capabilities

- `arduino-firmware-runtime`: firmware modular Arduino y regresión completa de Escudo.

### Modified Capabilities

- `project-workspace`: documentar desarrollo/subida/monitor Arduino IDE Windows sin alterar el flujo de otros componentes.

## Impact

Fuentes/configuración/particiones/runners de `firmware/` y documentación de arquitectura/decisiones/desarrollo/encargos. La ruta vigente depende de sentinel-arduino-dependencies y del core oficial; sentinel-arduino-sdk conserva únicamente evidencia histórica sustituida. Riesgos: doble inicialización, ABI, pérdida de NVS, duplicación de fuentes o regresión. App/broker/contratos/notificaciones/simulador no cambian.

Fuente de ejecución: [plan de migración](../../../docs/firmware-arduino-migration.md). Responsable humano: P1; la autorización del usuario cubre implementación, no commits/push/merge.
