# Tasks

Responsable único de todas las casillas: **Persona 2**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Cliente e integración

- [ ] 1.1 Implementar configuración HTTPS/CA persistente y POST DoH acotado; probar commit/fallo de guardado, recuperación en Idle tras reinicio/corte sin USB, respuesta válida, ID incorrecto, tamaño excesivo y tipo de contenido.
- [ ] 1.2 Validar TLS/hora y fallos cerrados; comprobar certificado inválido y timeout sin DNS plano.
- [ ] 1.3 Integrar caché, reintentos, métricas y estado de protección; documentar bootstrap y límites en docs/modules/sanitizador-doh.md.
- [ ] 1.4 Compilar y probar consultas permitidas/bloqueadas desde cliente AP con NAT+DoH reales; registrar endpoint y resultado sin secretos.

## 2. Comprobación del conjunto

- [ ] 2.1 Cortar/restaurar upstream/DoH y consultar BLE; verificar degradación, SERVFAIL, recuperación y control disponible.
- [ ] 2.2 Adjuntar evidencia funcional de auditoría/pasarela/DNS/DoH y pedir revisión del módulo completo sin prometer rendimiento.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
