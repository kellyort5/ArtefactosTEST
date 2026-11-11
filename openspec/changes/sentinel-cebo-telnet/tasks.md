# Tasks

Responsable único de todas las casillas: **Persona 3**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Telnet simulado

- [ ] 1.1 Implementar listener/sesiones en puerto23 con estados e IAC acotado; probar entradas fragmentadas y línea excesiva.
- [ ] 1.2 Integrar bus de interacciones y datos no sensibles; verificar que un marcador ficticio no sale a logs/eventos.
- [ ] 1.3 Unificar límites HTTP/Telnet y parada; probar timeout y cierre del par de servicios.
- [ ] 1.4 Documentar interacción esperada y límites en docs/modules/cebo-telnet.md; comprobar instrucciones con cliente LAN.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar y conectar cliente Telnet real en red propia junto a HTTP; observar eventos y respuestas simuladas.
- [ ] 2.2 Salir de Cebo y comprobar puertos80/23 cerrados y ninguna tarea/sesión residual.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
