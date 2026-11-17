# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-app-protocolo-v2`, `sentinel-sanitizador-auditoria`, `sentinel-sanitizador-doh`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** R-08 operable desde app real por BLE y métricas/diagnósticos verificables.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08, H-04, H-05 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Formularios BLE upstream/AP/DoH con límites del contrato, campos de clave transitorios y validación de subred/HTTPS sin sustituir validación del nodo. Secuencia visible: configurar, auditar, revisar DNS/HTTPS/portal, iniciar pasarela, observar protección/degradación. Controles parar y reglas put/remove/list paginadas con excepciones; clientes IP/MAC/RSSI/calidad y bytes subida/bajada, gráficos sencillos y métricas DNS/bloqueos/latencia. Mostrar IPv4 y cobertura de filtrado DNS, no prometer red segura universal ni resolver portal automáticamente. Cerrar requiere un cliente real usando AP/NAT/DNS/DoH junto a control BLE; se puede usar portátil como cliente y teléfono como controlador.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `app/src/features/sanitizador/`, `app/src/ (registro de ruta y store Sanitizador)`, `app/test/sanitizador*`, `docs/modules/app-sanitizador.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Pérdida de Internet del teléfono al cambiar red → BLE sigue controlando, estado explícito.
- Datos desconocidos/secretos → null con motivo y limpieza de formularios.

## Migration Plan

Trabajar en rama `feat/sentinel-app-sanitizador`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.

## Persistencia al implementar este encargo

P4 debe contemplar que reinstalar/actualizar por USB puede reiniciar identidad, bonds e historial/cursores. Si ocurre, retirar vínculos/caché obsoletos del teléfono, volver a emparejar/configurar y reconciliar desde metadata/estado nuevos; no reutilizar el cursor anterior frente a un historial reinicializado, aunque el device_id derivado de MAC siga igual. No prometer migración de datos entre versiones. Conservar resultados históricos por separado y no presentar mensajes antiguos como actuales. En reinicios sin upload sí se exige recuperar datos confirmados. Esta documentación no declara implementada una UI nueva.
