# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## 1. Instaladores y comprobaciones

- [x] 1.1 Implementar instalación wolfMQTT y helpers mínimos con versión/hash fijados, descarga o ZIP local, staging, respaldo e idempotencia; comprobar instalación, reparación y hash incorrecto en Windows PowerShell.
- [x] 1.2 Implementar combinación conservadora de las nueve claves para core3.3.12; comprobar preservación de claves/comentarios ajenos, duplicados, respaldo, no-op y core ausente.
- [x] 1.3 Incorporar pruebas focalizadas reproducibles, actualizar las guías a los dos comandos y verificar rutas y ejemplos, sin cambios de firmware funcional ni secretos.

## 2. Revisión integrada

- [x] 2.1 Ejecutar comprobaciones nativas con descarga real y destinos temporales, WhatIf sobre rutas reales, validación OpenSpec y revisión de diff; registrar resultados y límites en verification.md.

Nota de vigencia: la tarea 1.2 registra la preparación histórica de nueve claves. La política posterior y la retirada comprobada de cinco recetas propias pertenecen a [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md); no se atribuye esa prueba al resultado anterior.
