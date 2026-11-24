# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

El coordinador marca casillas con evidencia y línea exacta del CLI. Un responsable de implementación por cambio.

## 1. Implementación y verificación

- [x] 1.1 Identificar dependencias oficiales/mantenidas mínimas mediante fuentes primarias y una matriz breve de compatibilidad, versiones y garantías; elevar incompatibilidades concretas sin investigación extensa.
- [x] 1.2 Adaptar transportes y arranque al paquete oficial reutilizando bibliotecas, conservando interfaces, seguridad y datos; verificar compilación y rutas reales de dependencias.
- [x] 1.3 Fijar instalación/configuración de ESP32S3 Dev Module y retirar plataforma propia cuando deje de ser necesaria; entregar instrucciones y comprobar que no se usa el SDK Sentinel instalado.
- [x] 1.4 Compilar y enlazar el firmware completo con CLI nativo del Arduino oficial, con las recetas mínimas aprobadas y sin generadores ni scripts propios durante cada build, y entregar verification.md con evidencia y pendientes físicos.
