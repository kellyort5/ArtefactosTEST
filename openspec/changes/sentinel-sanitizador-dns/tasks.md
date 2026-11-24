# Tasks

Responsable único de todas las casillas: **Persona 2**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. DNS y reglas

- [ ] 1.1 Implementar parser/servidor UDP y TCP acotado; probar truncamiento, ciclos de compresión y saturación.
- [ ] 1.2 Implementar put/remove/list de reglas y matching normalizado con persistencia oficial; probar límite, excepción allow, frontera de subdominio, commit antes de confirmar y recuperación en Idle tras reinicio/corte sin USB.
- [ ] 1.3 Implementar caché TTL e interfaz del resolvedor asíncrono; verificar expiración y SERVFAIL sin resolver cifrado.
- [ ] 1.4 Publicar eventos/métricas y documentar cobertura/límites en docs/modules/sanitizador-dns.md; validar mensajes v2.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar y consultar desde un cliente real del AP; verificar NXDOMAIN, SERVFAIL y reglas por BLE.
- [ ] 2.2 Comprobar que el puerto DNS no atiende clientes externos al AP y que cambiar modo libera sockets.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
