# Encargo: Laboratorio y evidencia funcional

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-sin-cuentas/verification.md).

Separar inicialización USB con pérdida permitida de pruebas físicas de reinicio y corte sin upload tras commit. No cerrar pendientes mediante build/fixtures.

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-laboratorio-pruebas`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

El cierre debe distinguir lógica de detección probada de captura/interacciones reales, con evidencia reproducible del equipo.

Matriz y procedimientos listos pronto; cierre con evidencia real de todos los módulos y sin confundir fixture con hardware.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-laboratorio-pruebas/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-laboratorio-pruebas/specs/module-validation/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-laboratorio-pruebas/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-module-contracts](sentinel-module-contracts.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `simulator/ (fixtures/reproductor de mensajes, sin ataque por radio)`
- `docs/lab/`
- `docs/evidence/ (plantillas e informes sanitizados)`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Laboratorio listo para el equipo

1. **1.1** Crear docs/lab/README.md con topología, autorización y turnos de hardware; verificar que no requiere atacar redes ni incorporar herramientas ofensivas.
2. **1.2** Crear fixtures/reproductor de mensajes en simulator separado; verificar validación de contrato y etiqueta de origen simulado.
3. **1.3** Crear casos benignos/sospechosos y plantilla PASS/FAIL/PENDIENTE en docs/lab/; comprobar trazabilidad completa R-03/R-08/R-10.
4. **1.4** Documentar pasos de recepción HTTP/Telnet, DNS/DoH, BLE/MQTT y push con datos ficticios; revisar procedimientos con cada dueño.
### Ejecución y evidencia

1. **2.1** Consolidar evidencias reales entregadas por los tres autores y Android; comprobar que capturas/logs no contienen secretos.
2. **2.2** Registrar fallos y asignarlos al dueño de su spec; cerrar solo cuando se ejecuten los casos obligatorios y no quede interfaz sin integración.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Matriz y procedimientos listos pronto; cierre con evidencia real de todos los módulos y sin confundir fixture con hardware. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-laboratorio-pruebas/proposal.md) · [Tareas](../../openspec/changes/sentinel-laboratorio-pruebas/tasks.md)
