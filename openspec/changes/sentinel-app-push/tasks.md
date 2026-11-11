# Tasks

Responsable único de todas las casillas: **Persona 4**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Integración Android

- [ ] 1.1 Revisar proveedor/plugin y fijar dependencia compatible; verificar android:sync y build sin configuración privada en Git.
- [ ] 1.2 Implementar permisos/registro/renovación/baja HTTPS; probar permiso denegado, cambio de token y acceso cruzado.
- [ ] 1.3 Integrar recepción y tap con historial; probar evento disponible, pendiente y fuera de retención.
- [ ] 1.4 Documentar configuración Firebase local y límites de cierre/force-stop en docs/modules/app-push.md; verificar que no hay claves Admin ni tokens versionados.

## 2. Comprobación del incremento

- [ ] 2.1 Ejecutar tests, npm run build, android:sync y android:assemble; instalar en teléfono y registrar instalación real.
- [ ] 2.2 Cerrar app por recientes, generar incidente real Cebo y comprobar push/tap; registrar teléfono, permiso, hora e ID sin tokens.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
