# Encargo: Integración final de los tres modos

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-integracion-tres-modos`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-13):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

Cambios aislados no acreditan recursos compartidos, transiciones, OLED/botón y recuperación del sistema completo.

Tres modos completos e integrados, controles físicos y recuperación reales; fallos por componente devueltos a su autor.

**Asignación explícita de Persona 1:** integrar las alertas recientes en la OLED, como exige la página 4 del PDF. Mostrar un resumen de la alerta más reciente con módulo de origen, tipo y referencia temporal disponible, también sin teléfono conectado. Reutilizar eventos/journal del núcleo y conservar la pantalla de emparejamiento; el detalle histórico continúa en la app.

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

- `firmware/SentinelNode/src/sentinel/ (registro final y recursos compartidos)`
- `firmware/SentinelNode/SentinelNode.ino`
- `firmware/SentinelNode/SentinelConfig.h` y requisitos del SDK en `firmware/arduino-package/`
- `firmware/test/integration/`
- `docs/integration-three-modes.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Núcleo completo

1. **1.1** Registrar componentes y flags globales solicitados por autores; compilar firmware completo y verificar capacidades solo reales.
2. **1.2** Verificar matriz de parada/liberación y errores con tests focalizados; documentar recursos por modo en docs/integration-three-modes.md.
3. **1.3** Integrar OLED/botón con estados de tres módulos; comprobar físicamente modo, error, emparejamiento largo y vuelta Idle por comando sin secretos.
4. **1.4** Reconciliar canales, boot/revisión y journal en transiciones; probar IDs estables y rechazo de comandos viejos.
5. **1.5** Integrar el resumen de la alerta más reciente en OLED desde el flujo central; comprobar físicamente origen/tipo/tiempo disponible, ausencia de alertas, actualización con una nueva alerta, operación sin teléfono y prioridad del emparejamiento, sin secretos ni bloqueos.
### Comprobación final

1. **2.1** Ejecutar builds y pruebas críticas de firmware, contratos y app; registrar resultados y fallos por dueño sin cambiar código ajeno.
2. **2.2** Recorrer tres modos, desconexión/reinicio y canales externos en hardware real con informes de P4; cerrar solo con todo el alcance integrado.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Tres modos completos e integrados, controles físicos, alertas recientes en OLED y recuperación reales; fallos por componente devueltos a su autor. Una pantalla, fixture o mock aislado no acredita integración real. La tarea 1.5 requiere observación de la OLED física, no solo de logs o dashboard. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-integracion-tres-modos/proposal.md) · [Tareas](../../openspec/changes/sentinel-integracion-tres-modos/tasks.md)
