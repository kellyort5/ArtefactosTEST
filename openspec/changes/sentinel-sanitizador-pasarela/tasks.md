# Tasks

Responsable único de todas las casillas: **Persona 2**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Pasarela y métricas

- [ ] 1.1 Declarar requisitos de Kconfig al núcleo y configurar netifs mediante sentinel_api; verificar build con forwarding/NAPT y sin drivers duplicados.
- [ ] 1.2 Implementar AP autenticado, DHCP y detección de solapamiento; probar validaciones de clave/subred y documentar configuración en docs/modules/sanitizador-gateway.md.
- [ ] 1.3 Integrar NAPT y parada/reconexión; comprobar que no hay port forwarding ni protección IPv6 anunciada.
- [ ] 1.4 Implementar lista de clientes, RSSI y contadores IPv4 por dirección; probar cálculo/delta y páginas v2 con casos focalizados.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar y conectar un teléfono o portátil al AP real; verificar DHCP, salida IPv4 y subida/bajada con endpoint del equipo.
- [ ] 2.2 Cambiar modo y desconectar upstream; registrar cierre completo y degradación visible sin campaña de rendimiento.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
