# Encargo: App: Cebo, historial y alertas

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P4 debe contemplar que reinstalar/actualizar por USB puede reiniciar identidad, bonds e historial/cursores. Si ocurre, retirar vínculos/caché obsoletos del teléfono, volver a emparejar/configurar y reconciliar desde metadata/estado nuevos; no reutilizar el cursor anterior frente a un historial reinicializado, aunque el device_id derivado de MAC siga igual. No prometer migración de datos entre versiones. Conservar resultados históricos por separado y no presentar mensajes antiguos como actuales. En reinicios sin upload sí se exige recuperar datos confirmados. Esta documentación no declara implementada una UI nueva.

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-app-cebo`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Los servicios Cebo requieren visualización MQTT en tiempo real, historial y visibilidad del envío de alertas.

Cebo real visible por MQTT, control confirmado, historial y alertas internas/estado externo.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-app-cebo/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-app-cebo/specs/app-cebo/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-app-cebo/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md)
- [sentinel-cebo-eventos-historial](sentinel-cebo-eventos-historial.md)
- [sentinel-alertas-webhook](sentinel-alertas-webhook.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `app/src/features/cebo/`
- `app/src/ (registro de ruta y store Cebo)`
- `app/test/cebo*`
- `docs/modules/app-cebo.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Pantalla Cebo

1. **1.1** Implementar controles/estado de servicios y métricas/gráficos; probar ACK pendiente/rechazado y ambos protocolos.
2. **1.2** Implementar tabla/filtros/historial y alertas internas; verificar dedup, gap y ausencia de campos sensibles.
3. **1.3** Integrar API de estado de notificaciones con configuración autorizada; probar error/autorización y documentar en docs/modules/app-cebo.md.
4. **1.4** Ejecutar npm run test:app y npm run build; verificar gráficos y estado responsive.
### Comprobación del incremento

1. **2.1** Compilar Android y observar interacciones reales HTTP/Telnet por WS; comprobar alerta interna e historial.
2. **2.2** Caer/restaurar MQTT y un canal externo; registrar recuperación y estados reales sin duplicación.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Cebo real visible por MQTT, control confirmado, historial y alertas internas/estado externo. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-app-cebo/proposal.md) · [Tareas](../../openspec/changes/sentinel-app-cebo/tasks.md)
