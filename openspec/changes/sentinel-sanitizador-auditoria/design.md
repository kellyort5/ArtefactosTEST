# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

Ver [motivación](proposal.md). La referencia de integración es `develop`: contratos v2, API de módulos, Escudo y app común/diagnóstico P1 ya están entregados por software. Consultar [estado y límites vigentes](../../../docs/mvp-policy.md) y [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md). La base y los incrementos archivados conservan sus comprobaciones físicas pendientes; este cambio no las acredita. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-firmware-module-api`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08, R-09 y H-05 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Registrar el módulo Sanitizador usando sentinel_api. Configuración upstream por BLE autenticado validada/persistente en NVS oficial con commit antes de confirmar y restauración segura en Idle; pedir STA al gestor sin SoftAP todavía. Auditoría asíncrona con timeout inicial 5 s por prueba, resultado por job_id y cancelación al salir. Usar resolución DNS de ESP-IDF y esp_http_client con mbedTLS: cadena/hostname/vigencia, sin deshabilitar verificación para conseguir un ok. Hora desconocida da inconcluso/error de tiempo.

Prueba de portal usa URL HTTP controlada por el equipo y respuesta esperada; redirección/respuesta alterada es indicio de portal, no MITM confirmado. No introducir login automático ni seguir formularios. URLs HTTPS/portal y nombre DNS se configuran como destinos propios/autorizados; no guardar cuerpo/cabeceras. Comparaciones DNS pueden ser diferencias legítimas: conservar evidencia y explicación. Alternativa de diagnosticar solo ping no satisface DNS/certificados/portal.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/SentinelNode/src/sanitizador/ (module y audit)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-audit.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Portal y DNS divididos → resultado inconcluso y límites explícitos.
- Credenciales y TLS → solo BLE, configuración propia en NVS oficial según política posterior, sin secretos en logs/event/status ni promesa de cifrado físico; no bypass de certificados.

## Migration Plan

Flujo de firmware: Arduino IDE Windows con core oficial ESP32 **3.3.12**, placa **ESP32S3 Dev Module**, flash **16 MB**, QIO y PSRAM OPI, sketch `firmware/SentinelNode/SentinelNode.ino`. Seguir [configuración y bibliotecas vigentes](../../../docs/firmware.md) y [desarrollo de módulos](../../../docs/development.md). Usar los módulos C/C++ de `src/`, `SentinelConfig.h` y las factories existentes; conservar APIs ESP-IDF/FreeRTOS y el único gestor de recursos. El núcleo y los ajustes globales pertenecen a P1; P2/P3 solicitan cambios acotados, sin editar core ni crear otro gestor Wi-Fi/BLE. No se requiere SDK propio. Conservar responsables, contratos, funciones y comprobaciones físicas pendientes.

Trabajar en rama `feat/sentinel-sanitizador-auditoria`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. Las dependencias comunes de P1 ya están integradas en develop; comprobar la entrega de cada dependencia funcional aún pendiente antes del cierre.

## Persistencia al implementar este encargo

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.
