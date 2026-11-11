# Encargo: Sanitizador: auditoría de conectividad

**Responsable único:** Persona 2 — firmware Auditor/Sanitizador. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-sanitizador-auditoria`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

R-08 requiere diagnosticar DNS, HTTPS y portal cautivo antes de habilitar una pasarela protegida.

Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-sanitizador-auditoria/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-sanitizador-auditoria/specs/sanitizer-audit/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-sanitizador-auditoria/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-firmware-module-api](sentinel-firmware-module-api.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/sanitizador/ (module, audit y CMake)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-audit.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Auditoría real

1. **1.1** Crear módulo Sanitizador con hooks y petición STA al gestor; compilar sin modificar runtime ajeno.
2. **1.2** Implementar configuración upstream BLE en RAM y cancelación; comprobar rechazo MQTT y ausencia de secretos en estado/logs.
3. **1.3** Implementar trabajos DNS/HTTPS/portal con timeout; probar clasificación de respuestas y errores con fixtures focalizados.
4. **1.4** Publicar audit_result/evento v2 y documentar destinos/criterios en docs/modules/sanitizador-audit.md; verificar todos los resultados.
### Comprobación del incremento

1. **2.1** Compilar pio run -d firmware y ejecutar auditoría real de red/endpoints controlados con un resultado HTTPS válido y uno inválido.
2. **2.2** Salir del modo durante auditoría; comprobar cancelación y liberación STA sin arrancar AP ni dejar tareas activas.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-sanitizador-auditoria/proposal.md) · [Tareas](../../openspec/changes/sentinel-sanitizador-auditoria/tasks.md)
