# Design

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

> **Antecedente sustituido:** este cambio registra el SDK propio que se construyó antes de la simplificación aprobada. El destino vigente es el core oficial 3.3.12, tres teléfonos recordados y configuración mínima de protección de datos de [sentinel-arduino-dependencies](../sentinel-arduino-dependencies/design.md). No reinstalar ni reconstruir este paquete; no sincronizar su capacidad `arduino-sdk` como requisito actual. Las evidencias y casillas anteriores conservan su alcance histórico.

## Context

El usuario autorizó ejecución completa y coordinación Orca. Ver [plan](../../../docs/firmware-arduino-migration.md); firmware/base y Escudo existentes se conservan, sin modificar componentes externos. El core estándar tiene MQTT5/TLS-fechas/SC-only distintos y autoformato NVS.

## Goals / Non-Goals

**Goals:** SDK aislado, reproducible y comprobado en Windows que permita Arduino IDE compilar/subir firmware seguro N16R8.
**Non-Goals:** degradar seguridad, publicar secretos/binarios en Git, alterar contratos o crear una distribución universal de placas.

## Decisions

Preparación Linux/WSL de bibliotecas oficiales, uso cotidiano Arduino IDE Windows. Referencias iniciales fijadas por el plan; si la compilación prueba incompatibilidad, registrar corrección concreta. No editar el paquete global de Boards Manager. Instalar una plataforma manual Sentinel con herramientas Windows verificadas. Config efectiva, bibliotecas y fuentes del core deben coincidir; un define no reconfigura .a. Mantener MQTT5, validación CA/hostname/fechas, NimBLE SC-only/nivel4, NAPT y N16R8. Parche de arranque NVS mínimo sin borrado automático, antes de setup. Evidencia de símbolos, build mínimo e instalación real. Código fuente/parches/config/manifiesto en Git, SDK/ZIP/cache fuera.

## Risks / Trade-offs

Compilación del SDK puede tardar; limitar target S3. El parche del core exige verificación de error antes de setup. No mezclar herramientas Linux y Windows ni headers de versiones distintas. El SDK es dependencia del port final, no prueba de funcionamiento en placa. Ante fallo registrar causa y corregir; no quitar gates de seguridad.
