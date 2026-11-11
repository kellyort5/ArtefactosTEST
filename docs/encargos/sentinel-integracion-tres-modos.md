# Encargo: Integración final de los tres modos

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-integracion-tres-modos`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Cambios aislados no acreditan recursos compartidos, transiciones, OLED/botón y recuperación del sistema completo.

Tres modos completos e integrados, controles físicos y recuperación reales; fallos por componente devueltos a su autor.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-integracion-tres-modos/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-integracion-tres-modos/specs/system-integration/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-integracion-tres-modos/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-escudo-recuperacion](sentinel-escudo-recuperacion.md)
- [sentinel-sanitizador-doh](sentinel-sanitizador-doh.md)
- [sentinel-cebo-eventos-historial](sentinel-cebo-eventos-historial.md)
- [sentinel-app-escudo](sentinel-app-escudo.md)
- [sentinel-app-sanitizador](sentinel-app-sanitizador.md)
- [sentinel-app-push](sentinel-app-push.md)
- [sentinel-broker-lan](sentinel-broker-lan.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/sentinel/ (registro final y recursos compartidos)`
- `firmware/main/`
- `firmware/sdkconfig.defaults`
- `firmware/test/integration/`
- `docs/integration-three-modes.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Núcleo completo

1. **1.1** Registrar componentes y flags globales solicitados por autores; compilar firmware completo y verificar capacidades solo reales.
2. **1.2** Verificar matriz de parada/liberación y errores con tests focalizados; documentar recursos por modo en docs/integration-three-modes.md.
3. **1.3** Integrar OLED/botón con estados de tres módulos; comprobar físicamente modo, error, emparejamiento largo y vuelta Idle por comando sin secretos.
4. **1.4** Reconciliar canales, boot/revisión y journal en transiciones; probar IDs estables y rechazo de comandos viejos.
### Comprobación final

1. **2.1** Ejecutar builds y pruebas críticas de firmware, contratos y app; registrar resultados y fallos por dueño sin cambiar código ajeno.
2. **2.2** Recorrer tres modos, desconexión/reinicio y canales externos en hardware real con informes de P4; cerrar solo con todo el alcance integrado.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Tres modos completos e integrados, controles físicos y recuperación reales; fallos por componente devueltos a su autor. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-integracion-tres-modos/proposal.md) · [Tareas](../../openspec/changes/sentinel-integracion-tres-modos/tasks.md)
