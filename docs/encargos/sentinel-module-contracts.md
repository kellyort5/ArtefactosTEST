# Encargo: Contratos funcionales compartidos

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** implementación y comprobaciones técnicas publicadas en el [PR #2](https://github.com/Jefry-P/ArtefactosProyecto/pull/2); revisión final del usuario pendiente en la tarea 2.2. Ver [evidencia y límites](../../contracts/evidence-v2.md). **Rama:** `feat/sentinel-module-contracts`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

La base v1 rechaza campos funcionales nuevos; los tres autores de firmware y la app necesitan un vocabulario común antes de integrar.

Contratos compilados y ejemplos de todas las operaciones/familias aceptados o rechazados según sus límites; revisión explícita antes de consumidores.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-module-contracts/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-module-contracts/specs/module-protocol/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-module-contracts/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

Parte de la base existente; no requiere otro cambio nuevo.

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `contracts/schemas/v2/`
- `contracts/src/ (codec y exportaciones)`
- `contracts/fixtures/v2/`
- `contracts/test/`
- `contracts/README.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Esquemas y compatibilidad

1. **1.1** Crear esquemas v2 de sobre, estado y métricas; verificar ejemplos de los tres perfiles y rechazo de campos desconocidos con npm run check:contracts.
2. **1.2** Definir uniones de eventos y resultados paginados con límites/unidades del diseño; comprobar fixtures válidos y sobredimensionados.
3. **1.3** Añadir ramas de comandos y ACK para cada operación de la tabla; comprobar correlación y casos inválidos/secretos.
4. **1.4** Extender el codec/exportaciones para v1 y v2 sin modificar semántica v1; verificar fixtures existentes y eventos históricos.
5. **1.5** Documentar negociación, topics, paginación y API de registro/push en contracts/README.md; comprobar que cada operación tiene ejemplo y consumidor.
### Contrato listo para consumidores

1. **2.1** Añadir fixtures compartidos benignos/sospechosos y ejemplos HTTP de registro sin secretos reales; verificar tamaño UTF-8 y nombres de campos.
2. **2.2** Ejecutar npm run check:contracts y npm run build -w contracts; registrar resultados y obtener revisión del usuario y un consumidor del contrato.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Contratos compilados y ejemplos de todas las operaciones/familias aceptados o rechazados según sus límites; revisión explícita antes de consumidores. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-module-contracts/proposal.md) · [Tareas](../../openspec/changes/sentinel-module-contracts/tasks.md)
