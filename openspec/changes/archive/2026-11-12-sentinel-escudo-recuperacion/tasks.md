# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado: **3/6 tareas verificadas en software**. El usuario autorizó continuar conservando las pruebas físicas **1.3, 2.1 y 2.2 pendientes**. Ver [evidencia](verification.md).

## 1. Recuperación

- [x] 1.1 Conectar eventos Escudo al journal compartido; verificar recuperación de un evento y cursor tras reinicio breve.
- [x] 1.2 Implementar reporte de cobertura/gap al sincronizar; probar anillo pequeño de prueba sin llenar flash repetidamente.
- [ ] 1.3 Comprobar coexistencia real y aplicar pausa controlada solo si hay fallo bloqueante; verificar reanudación y documento del resultado.
- [x] 1.4 Documentar retención, sincronización y política de pausa en docs/modules/escudo-recovery.md; cotejar órdenes con v2.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar y ejecutar desconexión/reconexión BLE en ESP32/teléfono; verificar mismo ID/cursor y ausencia de duplicado en historial.
- [ ] 2.2 Comprobar captura, selección y detectores disponibles después de recuperar; adjuntar evidencia breve.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
