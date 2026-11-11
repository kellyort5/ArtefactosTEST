# Tasks

Responsable único de todas las casillas: **Persona 3**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Componente y canales

- [ ] 1.1 Crear paquete/lockfile y Compose propios de notifications con MQTT TLS/ACL de lectura; verificar conexión autorizada y rechazo cross-node.
- [ ] 1.2 Implementar enrutado, dedup, outbox y límites persistentes; probar repetición, fallo, vencimiento y recuperación con fixtures.
- [ ] 1.3 Implementar webhook HTTPS; comprobar entrega en destinos de prueba propios y redacción de errores.
- [ ] 1.4 Implementar API HTTPS de estado con autenticación por nodo/CORS; probar rechazo de otro nodo y ausencia de secretos.
- [ ] 1.5 Documentar configuración, arranque y retención en notifications/README.md y docs/modules/notifications.md; verificar comandos sin valores reales.

## 2. Comprobación del incremento

- [ ] 2.1 Ejecutar tests del paquete y recibir alerta de un incidente real del ESP32 en el destino webhook; registrar IDs y conteos sin tokens.
- [ ] 2.2 Interrumpir y restablecer el webhook y reentregar un evento; comprobar supresión y recuperación limitadas.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
