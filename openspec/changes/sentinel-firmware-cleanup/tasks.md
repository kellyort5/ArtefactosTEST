# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

El coordinador marca casillas con evidencia y línea exacta del CLI. Un responsable de implementación por cambio.

## 1. Implementación y verificación

- [x] 1.1 Inventariar y retirar únicamente recetas/entradas inactivas del firmware anterior, verificando que se conserva toda fuente funcional y una sola entrada Arduino.
- [x] 1.2 Sustituir generación Python requerida por fuente C++ distribuible y mantenimiento con Node existente; verificar equivalencia con schemas/fixtures y compilación sin generación ni scripts propios durante el build.
- [x] 1.3 Migrar runners de pruebas críticas a herramientas existentes sin perder casos ni aserciones y eliminar Python propio obsoleto; ejecutar núcleo/Escudo y cross-codec.
- [x] 1.4 Coordinar retirada de scripts del SDK propio tras confirmación del responsable de dependencias y entregar verification.md con inventario de eliminaciones y comandos actuales.
