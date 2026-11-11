# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-escudo-recuperacion`, `sentinel-sanitizador-doh`, `sentinel-cebo-eventos-historial`, `sentinel-app-escudo`, `sentinel-app-sanitizador`, `sentinel-app-push`, `sentinel-broker-lan`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Tres modos completos e integrados, controles físicos y recuperación reales; fallos por componente devueltos a su autor.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-05, R-06, R-07, R-08, R-10 y R-11 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Integrar registros de los componentes ya revisados, manteniendo contratos/API aprobados. Único autor P1 modifica núcleo y configuración global; si encuentra fallo de módulo o app lo devuelve a su dueño y spec, no se convierten dos personas en autores del mismo cambio. Revisar matriz de recursos y ciclo stop/release/start con pruebas críticas y recorrido en placa.

OLED muestra modo/subestado, objetivo o clientes/servicios, conectividad/error y emparejamiento de base; botón corto rota modos disponibles y pulsación larga de 3 s conserva apertura de emparejamiento durante 120 s, sin acción corta adicional. Idle se solicita por comando y cierra listeners. No incluir secretos. Estado coherente tras reconectar, cambios BLE↔MQTT según modo y boot/revisión; preservar journal común, capacidades reales y credenciales de configuración en RAM. Si Sanitizador está en portal o DoH falla, no declarar protección activa. Si Escudo pausa para BLE, mostrar hueco. No declarar sistema completo si falta webhook o push cerrado. Comparaciones prolongadas de rendimiento/autonomía posteriores.

**Asignación confirmada por el usuario:** Persona 1 integra también las alertas recientes en la OLED SSD1306 128×32, según la página 4 del PDF. Reutilizar el flujo central de eventos y el journal existentes para presentar un resumen compacto de la alerta más reciente: módulo de origen, tipo y referencia temporal disponible; cuando no exista alerta, mostrarlo explícitamente. Actualizar el resumen al llegar una nueva alerta, incluso sin teléfono conectado, sin bloquear captura, servicios o supervisor. Si falta hora UTC, usar referencia relativa o indicar hora desconocida. Conservar la prioridad temporal de la pantalla de emparejamiento y recuperar después la presentación normal con la alerta reciente. La app conserva el detalle e historial; esta asignación no añade otro almacén, una interfaz de navegación histórica en OLED ni cambios de contratos.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sentinel/ (registro final y recursos compartidos)`, `firmware/main/`, `firmware/sdkconfig.defaults`, `firmware/test/integration/`, `docs/integration-three-modes.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Una placa compartida → coordinar comprobaciones y registrar build/estado.
- Integración tardía → cada cambio anterior ya aporta verificación; aquí comprobar recorridos cruzados, no desarrollar funciones faltantes.

## Migration Plan

Trabajar en rama `feat/sentinel-integracion-tres-modos`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
