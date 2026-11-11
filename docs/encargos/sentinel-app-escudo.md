# Encargo: App: Escudo completo

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-app-escudo`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

El usuario necesita seleccionar redes y ver todos los detectores/cobertura del Escudo real, no solo tarjetas de diagnóstico.

Pantalla Escudo completa integrada por BLE, con control, gráficos, incidentes y recuperación reales.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-app-escudo/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-app-escudo/specs/app-escudo/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-app-escudo/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md)
- [sentinel-escudo-descubrimiento](sentinel-escudo-descubrimiento.md)
- [sentinel-escudo-detectores](sentinel-escudo-detectores.md)
- [sentinel-escudo-recuperacion](sentinel-escudo-recuperacion.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `app/src/features/escudo/`
- `app/src/ (registro de ruta y store Escudo)`
- `app/test/escudo*`
- `docs/modules/app-escudo.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Pantalla Escudo

1. **1.1** Implementar catálogo paginado y selección/controles BLE; verificar estados vacío, oculto, pendiente, rechazo y objetivo aplicado.
2. **1.2** Implementar gráficos, tabla y detalle de cuatro detectores; comprobar fixtures benignos/sospechosos y lenguaje de indicio.
3. **1.3** Integrar cobertura/historial y sincronización; probar dedup/gap y documentar flujo en docs/modules/app-escudo.md.
4. **1.4** Ejecutar npm run test:app y npm run build; comprobar vista responsive y acciones con contratos.
### Comprobación del incremento

1. **2.1** Compilar Android y seleccionar/monitorear una red real por BLE con ESP32; verificar gráficas y estado con datos reales.
2. **2.2** Desconectar/reconectar BLE y comprobar recuperación sin doble incidente; adjuntar captura/evidencia sanitizada.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Pantalla Escudo completa integrada por BLE, con control, gráficos, incidentes y recuperación reales. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-app-escudo/proposal.md) · [Tareas](../../openspec/changes/sentinel-app-escudo/tasks.md)
