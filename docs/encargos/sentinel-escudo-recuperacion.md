# Encargo: Escudo: desconexión y coexistencia

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-escudo-recuperacion`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

El Escudo completo debe conservar eventos cuando BLE cae y mostrar periodos sin cobertura.

Evento recuperable tras desconexión y reinicio, huecos visibles y funciones conservadas.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-escudo-recuperacion/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-escudo-recuperacion/specs/escudo-recovery/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-escudo-recuperacion/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-escudo-detectores](sentinel-escudo-detectores.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/escudo/ (coverage, reconnect y backlog)`
- `firmware/test/escudo/`
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

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-escudo-recuperacion/proposal.md) · [Tareas](../../openspec/changes/sentinel-escudo-recuperacion/tasks.md)
