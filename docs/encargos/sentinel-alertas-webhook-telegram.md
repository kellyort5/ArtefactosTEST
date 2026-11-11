# Encargo: Alertas: componente, webhook y Telegram

**Responsable único:** Persona 3 — firmware Cebo y servidor de notificaciones. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-alertas-webhook-telegram`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

MQTT y una app cerrada no envían por sí mismos las alertas externas obligatorias de R-10.

Webhook y Telegram reales, con dedup/reintentos y estado autenticado; infraestructura mínima documentada.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-alertas-webhook-telegram/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-alertas-webhook-telegram/specs/notification-dispatch/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-alertas-webhook-telegram/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-module-contracts](sentinel-module-contracts.md)
- [sentinel-broker-lan](sentinel-broker-lan.md)
- [sentinel-cebo-eventos-historial](sentinel-cebo-eventos-historial.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `notifications/ (servicio Node, package y lockfile propios, Compose y README)`
- `notifications/test/`
- `docs/modules/notifications.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Componente y canales

1. **1.1** Crear paquete/lockfile y Compose propios de notifications con MQTT TLS/ACL de lectura; verificar conexión autorizada y rechazo cross-node.
2. **1.2** Implementar enrutado, dedup, outbox y límites persistentes; probar repetición, fallo, vencimiento y recuperación con fixtures.
3. **1.3** Implementar webhook HTTPS y Telegram; comprobar entrega en destinos de prueba propios y redacción de errores.
4. **1.4** Implementar API HTTPS de estado con autenticación por nodo/CORS; probar rechazo de otro nodo y ausencia de secretos.
5. **1.5** Documentar configuración, arranque y retención en notifications/README.md y docs/modules/notifications.md; verificar comandos sin valores reales.
### Comprobación del incremento

1. **2.1** Ejecutar tests del paquete y recibir alerta de un incidente real del ESP32 en ambos destinos; registrar IDs y conteos sin tokens.
2. **2.2** Interrumpir un canal y reentregar evento; comprobar independencia, supresión y recuperación limitadas.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Webhook y Telegram reales, con dedup/reintentos y estado autenticado; infraestructura mínima documentada. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-alertas-webhook-telegram/proposal.md) · [Tareas](../../openspec/changes/sentinel-alertas-webhook-telegram/tasks.md)
