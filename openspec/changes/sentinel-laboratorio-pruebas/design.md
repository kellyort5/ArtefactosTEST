# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-module-contracts`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Matriz y procedimientos listos pronto; cierre con evidencia real de todos los módulos y sin confundir fixture con hardware.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-08, R-09, R-10 y R-11 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Preparar matriz de casos por alcance R-03/R-08/R-10 con responsable ejecutor del módulo y coordinador P4. La Persona4 redacta procedimientos y consolida evidencia; no implementa correcciones dentro de firmware ajeno. Fixtures de observaciones/mensajes tipados en simulator/ sirven para UI y heurísticas; deben etiquetarse como simulados y nunca contarse como captura o push reales. Los tests críticos de cada módulo los escribe su autor en su propio cambio.

Red propia/aislada: AP autorizado, ESP32, teléfono, PC/portátil y endpoints de prueba del equipo. Usar accesos HTTP/Telnet normales con datos ficticios y repeticiones manuales acotadas para Cebo; auditoría/DNS de dominios propios controlados y certificados válidos/inválidos de laboratorio para Sanitizador; Beacons reales y fixtures positivos/negativos para Escudo. No incluir generadores de deauth, herramientas de explotación ni inyección radio. La autenticidad de la conexión detector-captura se verifica observando el flujo real benigno y trazabilidad del mismo parser/detector de tests, sin afirmar haber validado un ataque en aire si no se hizo.

Informe por caso: build/commit de referencia si existe, hardware, red autorizada, pasos, esperado/obtenido, tipo real/fixture, evidencia sanitizada, PASS/FAIL/PENDIENTE y autor. P4 recoge informes, cada dueño corrige su cambio. No benchmark ni carga sostenida. El laboratorio empieza con plantillas/fixtures, se cierra solo después de ejecución conjunta de módulos reales.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../sentinel-module-contracts/design.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `simulator/ (fixtures/reproductor de mensajes, sin ataque por radio)`, `docs/lab/`, `docs/evidence/ (plantillas e informes sanitizados)`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Falsos positivos/negativos y fixtures → casos benignos y evidencia explícitamente etiquetada.
- Hardware compartido → turnos cortos por módulo y registro de versión para no pisar validaciones.

## Migration Plan

Trabajar en rama `feat/sentinel-laboratorio-pruebas`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
