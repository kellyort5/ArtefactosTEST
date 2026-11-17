# Encargo: App: Auditor y Sanitizador completo

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-app-sanitizador`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

R-08 necesita configuración BLE, resultados de auditoría, clientes/métricas y controles de reglas/DoH accesibles.

R-08 operable desde app real por BLE y métricas/diagnósticos verificables.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-app-sanitizador/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-app-sanitizador/specs/app-sanitizer/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-app-sanitizador/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md)
- [sentinel-sanitizador-auditoria](sentinel-sanitizador-auditoria.md)
- [sentinel-sanitizador-doh](sentinel-sanitizador-doh.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `app/src/features/sanitizador/`
- `app/src/ (registro de ruta y store Sanitizador)`
- `app/test/sanitizador*`
- `docs/modules/app-sanitizador.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Pantalla Sanitizador

1. **1.1** Implementar formularios BLE y flujo de auditoría/job_id; probar validaciones, pendiente/error y limpieza de claves.
2. **1.2** Implementar pasarela/clientes/RSSI/tráfico/estado y gráficos; verificar datos desconocidos y degradación sin éxito falso.
3. **1.3** Implementar reglas DNS paginadas y métricas; probar bloqueo/excepción y documentar cobertura en docs/modules/app-sanitizador.md.
4. **1.4** Ejecutar tests app/build y compilar Android; verificar vistas responsive y operaciones compatibles con contrato.
### Comprobación del incremento

1. **2.1** Con ESP32 y cliente AP reales, configurar/auditar/navegar/bloquear y consultar BLE; registrar métricas de ambas direcciones.
2. **2.2** Cortar DoH y detener pasarela; verificar error/recuperación y ausencia de claves persistidas.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** R-08 operable desde app real por BLE y métricas/diagnósticos verificables. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-app-sanitizador/proposal.md) · [Tareas](../../openspec/changes/sentinel-app-sanitizador/tasks.md)
