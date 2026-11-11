# Tasks

Responsable único de todas las casillas: **Persona 3**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Registro y proveedor

- [ ] 1.1 Revisar elección FCM y añadir SDK oficial compatible con lockfile propio; verificar test de adaptador y credencial local fuera de Git.
- [ ] 1.2 Implementar endpoints de registro/renovación/baja con autorización y límites; probar acceso cruzado, expiración y reemplazo sin filtrar token.
- [ ] 1.3 Integrar envío notification+data, errores y eliminación de token inválido; probar ID original, privacidad y dedup.
- [ ] 1.4 Documentar proyecto, credencial, HTTPS, API y operación en docs/modules/push-server.md; comprobar ejemplos ficticios y permisos del volumen.

## 2. Comprobación del incremento

- [ ] 2.1 Ejecutar tests y enviar push real a instalación Android de P4; registrar aceptación y coordinar evidencia de recepción cerrada.
- [ ] 2.2 Provocar fallo de proveedor y renovar/borrar registro; comprobar estado visible y ausencia de token en logs.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
