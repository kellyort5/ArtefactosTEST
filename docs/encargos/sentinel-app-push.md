# Encargo: Android: registro y recepción push

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-app-push`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Las alertas internas y WS no cumplen recepción con la aplicación cerrada.

Push real recibido con app cerrada y tap al incidente, con registro/baja seguros.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-app-push/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-app-push/specs/app-push/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-app-push/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md)
- [sentinel-app-cebo](sentinel-app-cebo.md)
- [sentinel-alertas-push-servidor](sentinel-alertas-push-servidor.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `app/src/features/notifications/`
- `app/src/ (integración de recepción/navegación)`
- `app/android/ (fuentes y configuración pública)`
- `app/package.json`
- `package-lock.json`
- `app/test/push*`
- `docs/modules/app-push.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Integración Android

1. **1.1** Revisar proveedor/plugin y fijar dependencia compatible; verificar android:sync y build sin configuración privada en Git.
2. **1.2** Implementar permisos/registro/renovación/baja HTTPS; probar permiso denegado, cambio de token y acceso cruzado.
3. **1.3** Integrar recepción y tap con historial; probar evento disponible, pendiente y fuera de retención.
4. **1.4** Documentar configuración Firebase local y límites de cierre/force-stop en docs/modules/app-push.md; verificar que no hay claves Admin ni tokens versionados.
### Comprobación del incremento

1. **2.1** Ejecutar tests, npm run build, android:sync y android:assemble; instalar en teléfono y registrar instalación real.
2. **2.2** Cerrar app por recientes, generar incidente real Cebo y comprobar push/tap; registrar teléfono, permiso, hora e ID sin tokens.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Push real recibido con app cerrada y tap al incidente, con registro/baja seguros. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-app-push/proposal.md) · [Tareas](../../openspec/changes/sentinel-app-push/tasks.md)
