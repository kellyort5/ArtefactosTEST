# Encargo: Cebo: clasificación, persistencia y MQTT

**Responsable único:** Persona 3 — firmware Cebo y servidor de notificaciones. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-cebo-eventos-historial`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Las interacciones HTTP/Telnet necesitan clasificación, métricas, persistencia offline y recuperación del historial.

Cebo clasifica ambos servicios y conserva/sincroniza eventos reales de manera acotada.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-cebo-eventos-historial/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-cebo-eventos-historial/specs/cebo-events/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-cebo-eventos-historial/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-cebo-telnet](sentinel-cebo-telnet.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/cebo/ (classifier, counters y event_adapter)`
- `firmware/test/cebo/`
- `docs/modules/cebo-events.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Eventos y persistencia

1. **1.1** Implementar tabla/ventanas/clasificación de ambos servicios; probar aislado, repetido, expiración y límite con reloj controlado.
2. **1.2** Adaptar bus a eventos/métricas v2 y cebo.stats; validar campos/privacidad con fixtures y comprobar contadores aun con alertas suprimidas.
3. **1.3** Conectar journal central y recuperación MQTT por cursor; probar persistencia y gap con anillo pequeño de prueba.
4. **1.4** Documentar umbrales, retención y flujo de sincronización en docs/modules/cebo-events.md; cotejar con R-06.
### Comprobación del incremento

1. **2.1** Compilar y generar interacción HTTP/Telnet real con datos ficticios; verificar evento y métricas MQTT con identidad autorizada.
2. **2.2** Cortar/restaurar broker y reiniciar brevemente; registrar recuperación con mismo ID, sin secretos y con límites visibles.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Cebo clasifica ambos servicios y conserva/sincroniza eventos reales de manera acotada. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-cebo-eventos-historial/proposal.md) · [Tareas](../../openspec/changes/sentinel-cebo-eventos-historial/tasks.md)
