# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

> **Antecedente sustituido:** este cambio registra el SDK propio que se construyó antes de la simplificación aprobada. El destino vigente es el core oficial 3.3.12, tres teléfonos recordados y configuración mínima de protección de datos de [sentinel-arduino-dependencies](../sentinel-arduino-dependencies/design.md). No reinstalar ni reconstruir este paquete; no sincronizar su capacidad `arduino-sdk` como requisito actual. Las evidencias y casillas anteriores conservan su alcance histórico.

Responsable humano: Persona 1 — Jefry. El usuario autoriza ejecución y coordinación Orca; no autoriza commits/push/merge. El coordinador marca cada casilla con evidencia del autor y la línea exacta del CLI.

## 1. Implementación

- [x] 1.1 Registrar versiones, manifiesto/configuración y controles verificables del paquete ESP32-S3 N16R8.
- [x] 1.2 Reconstruir bibliotecas reales con MQTT5, TLS con fechas, NimBLE SC-only/nivel4 y NAPT, sin mezclar SDK.
- [x] 1.3 Incorporar y verificar el parche de arranque que impide borrado NVS automático antes de setup.
- [x] 1.4 Implementar e instalar plataforma/placa Sentinel aislada con herramientas nativas Windows y verificador.

## 2. Verificación y documentación

- [x] 2.1 Compilar y enlazar un sketch real de capacidades con el CLI/IDE Windows y registrar salida/configuración efectiva.
- [x] 2.2 Documentar construcción, instalación, checksums y evidencia reproducible sin versionar binarios ni secretos.
