# Tasks

Responsable único de todas las casillas: **Persona 2**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Auditoría real

- [ ] 1.1 Crear módulo Sanitizador con hooks y petición STA al gestor; compilar sin modificar runtime ajeno.
- [ ] 1.2 Implementar configuración upstream BLE persistida en NVS oficial y cancelación; comprobar rechazo MQTT, commit antes de confirmar, recuperación Idle sin UTC viejo y ausencia de secretos en estado/logs.
- [ ] 1.3 Implementar trabajos DNS/HTTPS/portal con timeout; probar clasificación de respuestas y errores con fixtures focalizados.
- [ ] 1.4 Publicar audit_result/evento v2 y documentar destinos/criterios en docs/modules/sanitizador-audit.md; verificar todos los resultados.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar `firmware/SentinelNode` desde Arduino IDE Windows con la placa `Sentinel ESP32-S3 N16R8 (secure SDK)` y ejecutar auditoría real de red/endpoints controlados con un resultado HTTPS válido y uno inválido.
- [ ] 2.2 Salir del modo durante auditoría; comprobar cancelación y liberación STA sin arrancar AP ni dejar tareas activas.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
