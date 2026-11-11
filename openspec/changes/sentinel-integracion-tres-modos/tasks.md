# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Núcleo completo

- [ ] 1.1 Registrar componentes y flags globales solicitados por autores; compilar firmware completo y verificar capacidades solo reales.
- [ ] 1.2 Verificar matriz de parada/liberación y errores con tests focalizados; documentar recursos por modo en docs/integration-three-modes.md.
- [ ] 1.3 Integrar OLED/botón con estados de tres módulos; comprobar físicamente modo, error, emparejamiento largo y vuelta Idle por comando sin secretos.
- [ ] 1.4 Reconciliar canales, boot/revisión y journal en transiciones; probar IDs estables y rechazo de comandos viejos.

## 2. Comprobación final

- [ ] 2.1 Ejecutar builds y pruebas críticas de firmware, contratos y app; registrar resultados y fallos por dueño sin cambiar código ajeno.
- [ ] 2.2 Recorrer tres modos, desconexión/reinicio y canales externos en hardware real con informes de P4; cerrar solo con todo el alcance integrado.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
