# Encargo: Sanitizador: auditoría de conectividad

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.

**Responsable único:** Persona 2 — firmware Auditor/Sanitizador. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-sanitizador-auditoria`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-13):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

R-08 requiere diagnosticar DNS, HTTPS y portal cautivo antes de habilitar una pasarela protegida.

Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-sanitizador-auditoria/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-sanitizador-auditoria/specs/sanitizer-audit/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-sanitizador-auditoria/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-firmware-module-api](sentinel-firmware-module-api.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/SentinelNode/src/sanitizador/ (module y audit)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-audit.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Auditoría real

1. **1.1** Crear módulo Sanitizador con hooks y petición STA al gestor; compilar sin modificar runtime ajeno.
2. **1.2** Implementar configuración upstream BLE validada/persistente en NVS oficial y cancelación; comprobar rechazo MQTT y ausencia de secretos en estado/logs.
3. **1.3** Implementar trabajos DNS/HTTPS/portal con timeout; probar clasificación de respuestas y errores con fixtures focalizados.
4. **1.4** Publicar audit_result/evento v2 y documentar destinos/criterios en docs/modules/sanitizador-audit.md; verificar todos los resultados.
### Comprobación del incremento

1. **2.1** Compilar desde Arduino IDE Windows y ejecutar auditoría real de red/endpoints controlados con un resultado HTTPS válido y uno inválido.
2. **2.2** Salir del modo durante auditoría; comprobar cancelación y liberación STA sin arrancar AP ni dejar tareas activas.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-sanitizador-auditoria/proposal.md) · [Tareas](../../openspec/changes/sentinel-sanitizador-auditoria/tasks.md)
