# Encargo: Escudo: desconexión y coexistencia

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** 3/6 tareas verificadas en software; coexistencia y pruebas físicas 1.3/2.1/2.2 pendientes. Ver [evidencia](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/verification.md). **Rama prevista:** `feat/sentinel-escudo-recuperacion`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-13):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

El Escudo completo debe conservar eventos cuando BLE cae y mostrar periodos sin cobertura.

Evento recuperable tras desconexión y reinicio, huecos visibles y funciones conservadas.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/design.md) y los [casos de aceptación](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/specs/escudo-recovery/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-escudo-detectores](sentinel-escudo-detectores.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/SentinelNode/src/escudo/ (coverage, reconnect y backlog)`
- `firmware/test/escudo/`
- Ampliación aprobada por el usuario: `firmware/SentinelNode/src/sentinel/include/module_runtime.h`, `module_runtime.cpp` y `runtime.cpp`, únicamente para reutilizar la rutina de persistencia del núcleo en producción y pruebas.
- `docs/modules/escudo-recovery.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Recuperación

1. **1.1** Conectar eventos Escudo al journal compartido; verificar recuperación de un evento y cursor tras reinicio breve.
2. **1.2** Implementar reporte de cobertura/gap al sincronizar; probar anillo pequeño de prueba sin llenar flash repetidamente.
3. **1.3** Comprobar coexistencia real y aplicar pausa controlada solo si hay fallo bloqueante; verificar reanudación y documento del resultado.
4. **1.4** Documentar retención, sincronización y política de pausa en docs/modules/escudo-recovery.md; cotejar órdenes con v2.
### Comprobación del incremento

1. **2.1** Compilar y ejecutar desconexión/reconexión BLE en ESP32/teléfono; verificar mismo ID/cursor y ausencia de duplicado en historial.
2. **2.2** Comprobar captura, selección y detectores disponibles después de recuperar; adjuntar evidencia breve.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Evento recuperable tras desconexión y reinicio, huecos visibles y funciones conservadas. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/proposal.md) · [Tareas](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/tasks.md)
