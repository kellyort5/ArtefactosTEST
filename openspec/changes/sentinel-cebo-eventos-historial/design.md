# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

Ver [motivación](proposal.md). La referencia de integración es `develop`: contratos v2, API de módulos, Escudo y app común/diagnóstico P1 ya están entregados por software. Consultar [estado y límites vigentes](../../../docs/mvp-policy.md) y [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md). La base y los incrementos archivados conservan sus comprobaciones físicas pendientes; este cambio no las acredita. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Dependencias para integrar: `sentinel-cebo-telnet`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Cebo clasifica ambos servicios y conserva/sincroniza eventos reales de manera acotada.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-06, R-10, H-06 y H-07 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Agregar por origen/protocolo en tabla inicial32 entradas con LRU, ventanas60 s y expiración5 min. Parámetros propuestos: acceso aislado informativo; ≥5 auth/60 s sospecha media; ≥20 interacciones/60 s alta repetición. Son indicios de comportamiento, no prueba de intrusión. Contar intentos antes de suprimir alertas repetidas; enfriamiento30 s por origen/protocolo mantiene métricas.

Usar journal64 NVS y cursor persistente del núcleo, nunca otro almacenamiento. emit_event persiste antes de publicar; MQTT QoS1 no demuestra que todos los consumidores leyeron, read_events permite recuperación explícita por cursor y dedup. Conectar eventos v2, métricas y cebo.stats a transportes del núcleo; recuperación tras caída del broker usa cola finita/journal y gap explícito por sobrescritura. El historial en app lo implementa P4 en su cambio; este autor entrega mensajes reales, no pantallas.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/SentinelNode/src/cebo/ (classifier, counters y event_adapter)`, `firmware/test/cebo/`, `docs/modules/cebo-events.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Ráfagas/desgaste → agregación y límites sin perder contadores; no campaña de resistencia previa.
- QoS1 duplicados → IDs/cursor estables y dedup del consumidor.

## Migration Plan

Flujo de firmware: Arduino IDE Windows con core oficial ESP32 **3.3.12**, placa **ESP32S3 Dev Module**, flash **16 MB**, QIO y PSRAM OPI, sketch `firmware/SentinelNode/SentinelNode.ino`. Seguir [configuración y bibliotecas vigentes](../../../docs/firmware.md) y [desarrollo de módulos](../../../docs/development.md). Usar los módulos C/C++ de `src/`, `SentinelConfig.h` y las factories existentes; conservar APIs ESP-IDF/FreeRTOS y el único gestor de recursos. El núcleo y los ajustes globales pertenecen a P1; P2/P3 solicitan cambios acotados, sin editar core ni crear otro gestor Wi-Fi/BLE. No se requiere SDK propio. Conservar responsables, contratos, funciones y comprobaciones físicas pendientes.

Trabajar en rama `feat/sentinel-cebo-eventos-historial`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. Las dependencias comunes de P1 ya están integradas en develop; comprobar la entrega de cada dependencia funcional aún pendiente antes del cierre.

## Persistencia al implementar este encargo

P3 persistirá configuración aceptada de Cebo e historial sanitizado mediante recursos centrales, con límites, commit antes de confirmar y recuperación tras reinicio/corte sin USB; no iniciar servicios automáticamente. Mantener eventos/IDs/cursores y dedup dentro del ring. No guardar credenciales recibidas por HTTP/Telnet/honeypot o terceros. NVS estándar no promete cifrado físico; secretos propios solo por canal autorizado, nunca Git/log/event/status. Webhook y push mantienen sus encargos completos.
