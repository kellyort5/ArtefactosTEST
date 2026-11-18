# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-app-protocolo-v2`, `sentinel-escudo-descubrimiento`, `sentinel-escudo-detectores`, `sentinel-escudo-recuperacion`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Pantalla Escudo completa integrada por BLE, con control, gráficos, incidentes y recuperación reales.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-06 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Pantalla catálogo con scan pasivo, páginas, SSID/BSSID/canal/RSSI/auth/cipher/PMF, SSID oculto y estado de scan. Seleccionar objetivo por BSSID y controlar inicio/detención/cambio con ACK. Gráficas sencillas SVG o elementos React de actividad/volumen e intervalos, sin librería de gráficos adicional necesaria; tabla de incidentes con tiempo/calidad, red/canal, evidencia y detalle de indicio. Mostrar todos los detectores incluyendo Karma inconcluso, cobertura/huecos, pendientes y recuperación BLE. Usar diseño responsive existente para teléfono/web; no usar WS como sustituto del BLE obligatorio. Fixtures permiten construir UI antes del firmware, pero cierre exige ESP32 real.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `app/src/features/escudo/`, `app/src/ (registro de ruta y store Escudo)`, `app/test/escudo*`, `docs/modules/app-escudo.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- UI puede adelantar firmware → distinguir fixture de dato real y no marcar cierre por fixture.
- Alertas ambiguas → evidencia/indicio y SSID duplicado sin etiqueta malicioso.

## Migration Plan

Trabajar en rama `feat/sentinel-app-escudo`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.

## Persistencia al implementar este encargo

P4 debe contemplar que reinstalar/actualizar por USB puede reiniciar identidad, bonds e historial/cursores. Si ocurre, retirar vínculos/caché obsoletos del teléfono, volver a emparejar/configurar y reconciliar desde metadata/estado nuevos; no reutilizar el cursor anterior frente a un historial reinicializado, aunque el device_id derivado de MAC siga igual. No prometer migración de datos entre versiones. Conservar resultados históricos por separado y no presentar mensajes antiguos como actuales. En reinicios sin upload sí se exige recuperar datos confirmados. Esta documentación no declara implementada una UI nueva.
