# Tasks

Responsable único de todas las casillas: **Persona 4**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Pantalla Cebo

- [ ] 1.1 Implementar controles/estado de servicios y métricas/gráficos; probar ACK pendiente/rechazado y ambos protocolos.
- [ ] 1.2 Implementar tabla/filtros/historial y alertas internas; verificar dedup, gap y ausencia de campos sensibles.
- [ ] 1.3 Integrar API de estado de notificaciones con configuración autorizada; probar error/autorización y documentar en docs/modules/app-cebo.md.
- [ ] 1.4 Ejecutar npm run test:app y npm run build; verificar gráficos y estado responsive.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar Android y observar interacciones reales HTTP/Telnet por WS; comprobar alerta interna e historial.
- [ ] 2.2 Caer/restaurar MQTT y un canal externo; registrar recuperación y estados reales sin duplicación.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
