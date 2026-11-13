# Encargo: App: protocolo funcional y navegación

**Responsable único:** Persona 1 — Jefry, por reasignación expresa del usuario el 2026-11-12. **Estado:** software comprobado, 5/6 tareas; control físico 2.1 pendiente. Consultar tareas y verificación. **Rama prevista:** `feat/sentinel-app-protocolo-v2`. La reasignación afecta solo a esta spec; P4 conserva pantallas específicas, Android push, broker y laboratorio.

## Qué debes entregar

La app base solo valida v1 y no tiene control/estado funcional de tres módulos; hace falta un adaptador común antes de pantallas.

App nueva mantiene base y acepta protocolo funcional, con navegación/control/historial comunes.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-app-protocolo-v2/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-app-protocolo-v2/specs/app-module-protocol/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-app-protocolo-v2/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-module-contracts](sentinel-module-contracts.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `app/src/ (controller, store, journal, transports y navegación)`
- `app/test/`
- `app/package.json`
- `package.json`
- `package-lock.json`
- `app/README.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4; este adaptador común lo entrega P1), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Adaptadores comunes

1. **1.1** Actualizar transporte/controlador al codec v1/v2 y negociación; probar incompatibilidad, retained y boot/revisión.
2. **1.2** Extender store/journal para familias de tres módulos y paginación; probar dedup, eventos antiguos y gap.
3. **1.3** Crear navegación/secciones de módulos con capacidades reales y estados pendientes; verificar no disponible/rechazado sin éxito falso.
4. **1.4** Limpiar secretos transitorios y documentar conexión/versiones en app/README.md; comprobar que no hay persistencia de claves.
### Comprobación del incremento

1. **2.1** Ejecutar npm run test:app y npm run build; verificar arranque web y control básico de base real.
2. **2.2** Ejecutar npm run android:sync y npm run android:assemble; comprobar permisos/conexión BLE en teléfono o dejar evidencia pendiente.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** App nueva mantiene base y acepta protocolo funcional, con navegación/control/historial comunes. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-app-protocolo-v2/proposal.md) · [Tareas](../../openspec/changes/sentinel-app-protocolo-v2/tasks.md)
