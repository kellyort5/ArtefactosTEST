# Tasks

Responsable único de todas las casillas: **Persona 3**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Eventos y persistencia

- [ ] 1.1 Implementar tabla/ventanas/clasificación de ambos servicios; probar aislado, repetido, expiración y límite con reloj controlado.
- [ ] 1.2 Adaptar bus a eventos/métricas v2 y cebo.stats; validar campos/privacidad con fixtures y comprobar contadores aun con alertas suprimidas.
- [ ] 1.3 Conectar journal central y recuperación MQTT por cursor; probar persistencia y gap con anillo pequeño de prueba.
- [ ] 1.4 Documentar umbrales, retención y flujo de sincronización en docs/modules/cebo-events.md; cotejar con R-06.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar y generar interacción HTTP/Telnet real con datos ficticios; verificar evento y métricas MQTT con device_id/topic esperados sobre el broker anónimo; esa coincidencia no autentica al emisor.
- [ ] 2.2 Cortar/restaurar broker y reiniciar brevemente; registrar recuperación con mismo ID, sin secretos y con límites visibles.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
