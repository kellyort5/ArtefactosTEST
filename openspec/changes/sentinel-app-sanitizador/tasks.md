# Tasks

Responsable único de todas las casillas: **Persona 4**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Pantalla Sanitizador

- [ ] 1.1 Implementar formularios BLE y flujo de auditoría/job_id; probar validaciones, pendiente/error y limpieza de claves.
- [ ] 1.2 Implementar pasarela/clientes/RSSI/tráfico/estado y gráficos; verificar datos desconocidos y degradación sin éxito falso.
- [ ] 1.3 Implementar reglas DNS paginadas y métricas; probar bloqueo/excepción y documentar cobertura en docs/modules/app-sanitizador.md.
- [ ] 1.4 Ejecutar tests app/build y compilar Android; verificar vistas responsive y operaciones compatibles con contrato.

## 2. Comprobación del incremento

- [ ] 2.1 Con ESP32 y cliente AP reales, configurar/auditar/navegar/bloquear y consultar BLE; registrar métricas de ambas direcciones.
- [ ] 2.2 Cortar DoH y detener pasarela; verificar error/recuperación y ausencia de claves persistidas.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
