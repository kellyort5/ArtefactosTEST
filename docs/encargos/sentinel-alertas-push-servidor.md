# Encargo: Push: registro y envío en servidor

**Responsable único:** Persona 3 — firmware Cebo y servidor de notificaciones. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-alertas-push-servidor`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

La recepción con Android cerrado requiere proveedor push y registro seguro de instalaciones, además de alertas MQTT.

API real de registro/baja y envío FCM; aceptación verificada y recepción final a cargo del encargo Android.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-alertas-push-servidor/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-alertas-push-servidor/specs/push-delivery/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-alertas-push-servidor/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-alertas-webhook](sentinel-alertas-webhook.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `notifications/ (registro, proveedor FCM y configuración)`
- `notifications/test/`
- `docs/modules/push-server.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Registro y proveedor

1. **1.1** Revisar elección FCM y añadir SDK oficial compatible con lockfile propio; verificar test de adaptador y credencial local fuera de Git.
2. **1.2** Implementar endpoints de registro/renovación/baja con autorización y límites; probar acceso cruzado, expiración y reemplazo sin filtrar token.
3. **1.3** Integrar envío notification+data, errores y eliminación de token inválido; probar ID original, privacidad y dedup.
4. **1.4** Documentar proyecto, credencial, HTTPS, API y operación en docs/modules/push-server.md; comprobar ejemplos ficticios y permisos del volumen.
### Comprobación del incremento

1. **2.1** Ejecutar tests y enviar push real a instalación Android de P4; registrar aceptación y coordinar evidencia de recepción cerrada.
2. **2.2** Provocar fallo de proveedor y renovar/borrar registro; comprobar estado visible y ausencia de token en logs.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** API real de registro/baja y envío FCM; aceptación verificada y recepción final a cargo del encargo Android. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-alertas-push-servidor/proposal.md) · [Tareas](../../openspec/changes/sentinel-alertas-push-servidor/tasks.md)
