# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../../docs/technical-decisions.md), [arquitectura](../../../../docs/architecture.md) y [encargos](../../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-escudo-captura`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Cuatro detectores integrados; casos benignos/sospechosos focalizados y eventos válidos, sin confundir indicios con certeza.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-09 y H-06 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Implementar cuatro detectores conectados a observaciones reales, con tablas/ventanas acotadas. Parámetros iniciales propuestos: ráfaga ≥10 Deauth/1 s; desconexiones ≥5 Deauth/Disassoc dirigidas al objetivo/5 s; enfriamiento 30 s por detector/red. Son umbrales iniciales, no exactitud demostrada. Evil Twin compara BSSID del mismo SSID, cambios de canal/RSSI y seguridad/cifrado/PMF; SSID duplicado aislado produce contexto, no veredicto malicioso. Registrar qué diferencias justifican el indicio.

Karma: correlacionar Probe Request/Response observables, SSID solicitados y respuestas de un BSSID para varios SSID en una ventana, junto a la evidencia disponible. No solicitar redes falsas ni emitir pruebas activas. Si no hay evidencia suficiente, indicar cobertura/evidencia insuficiente; el detector existe y procesa observaciones, sin inventar una alerta. Usar fixtures deterministas positivos/negativos para lógica, conectados al mismo código usado por captura. La validación en aire prueba la captura de datos benignos y su conexión a los detectores; no requiere una herramienta de ataque.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../../contracts/module-protocol.md) y [spec principal](../../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/escudo/ (detectors y evidence)`, `firmware/test/escudo/`, `docs/modules/escudo-detectors.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Falsos positivos → evidencia, confianza de indicio y casos benignos obligatorios.
- Karma no siempre observable → informar límites y cobertura, nunca confirmar atribución.

## Migration Plan

Trabajar en rama `feat/sentinel-escudo-detectores`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.

## Concreción del incremento

Se reutilizan `Capture`, `Observation`, el supervisor, `ModuleContext` y los mensajes v2; no hay cambio arquitectónico ni contrato adicional. [Guía de implementación](../../../../docs/modules/escudo-detectors.md) con límites y criterios iniciales: tres SSID correlacionados/10 s para Karma, solicitud/respuesta hasta 2 s; referencia Evil Twin hasta 30 s, degradación de seguridad/PMF o canal distinto junto a diferencia RSSI ≥20 dB. Tablas/ventanas limitadas y enfriamiento por familia/BSSID. Son detalles ajustables, no exactitud validada.

El flujo interno de captura alimenta siempre los detectores; el observador opcional se conserva adicionalmente. Los contadores se integran en las métricas existentes de captura y las pérdidas de incidentes en `coverage.dropped_events`. El journal y el transporte siguen siendo compartidos. La documentación de captura/encargos se actualiza para reflejar la dependencia archivada y evitar anunciar detectores ausentes. El usuario autorizó continuar con la prueba física 2.1 pendiente; no se modifica su casilla ni el alcance.
