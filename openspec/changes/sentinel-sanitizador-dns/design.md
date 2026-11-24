# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

Ver [motivación](proposal.md). La referencia de integración es `develop`: contratos v2, API de módulos, Escudo y app común/diagnóstico P1 ya están entregados por software. Consultar [estado y límites vigentes](../../../docs/mvp-policy.md) y [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md). La base y los incrementos archivados conservan sus comprobaciones físicas pendientes; este cambio no las acredita. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Dependencias para integrar: `sentinel-sanitizador-pasarela`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** DNS local real, reglas/caché probadas y ningún fallback DNS plano; DoH aún explícitamente pendiente.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-08 y H-05 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Servidor DNS local en UDP53 y manejo TCP53 cuando el cliente lo requiere, limitado a interfaz/clientes SoftAP. Parser DNS con validación de longitudes, compresión sin ciclos y una pregunta por solicitud; máximo inicial 1232 bytes de consulta y 4096 de respuesta, 4 solicitudes pendientes y caché 32. Reglas v2 individuales hasta 32, nombre normalizado ASCII/punycode, minúsculas y sin punto final; match exacto o sufijo con frontera de punto. Allow específico tiene precedencia sobre block más amplio; empate mismo dominio resuelto por acción allow explícita. Bloqueo devuelve NXDOMAIN y contador, no una página que capture credenciales.

Inicialmente dominios permitidos sin respuesta en caché devuelven SERVFAIL con DoH no configurado; no reenviar silenciosamente a DNS sin cifrar. DoH se conecta en el siguiente cambio, cuya ausencia se muestra como incompleto. Usar interfaz interna de resolución asíncrona que el cliente DoH implementará. Reglas aceptadas y configuración propia DNS persistentes mediante NVS oficial y recursos centrales, con límites, commit antes de confirmar y restauración en Idle sin activar resolución. La caché de respuestas permanece transitoria; política de reglas/datos visible en la app. No almacenar credenciales de terceros ni secretos en logs/event/status; USB sin garantía de migración. Caché respeta TTL y límite, no mezcla clientes de upstream. Filtrar DNS local no garantiza bloquear IP directa/DoH de otras apps.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/SentinelNode/src/sanitizador/ (dns_server, rules y dns_cache)`, `firmware/test/sanitizador/`, `docs/modules/sanitizador-dns.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Paquetes DNS malformados → tests de compresión/ciclos/límites.
- Evasión de DNS → documentar cobertura sin prometer filtrado de todo tráfico.

## Migration Plan

Flujo de firmware: Arduino IDE Windows con core oficial ESP32 **3.3.12**, placa **ESP32S3 Dev Module**, flash **16 MB**, QIO y PSRAM OPI, sketch `firmware/SentinelNode/SentinelNode.ino`. Seguir [configuración y bibliotecas vigentes](../../../docs/firmware.md) y [desarrollo de módulos](../../../docs/development.md). Usar los módulos C/C++ de `src/`, `SentinelConfig.h` y las factories existentes; conservar APIs ESP-IDF/FreeRTOS y el único gestor de recursos. El núcleo y los ajustes globales pertenecen a P1; P2/P3 solicitan cambios acotados, sin editar core ni crear otro gestor Wi-Fi/BLE. No se requiere SDK propio. Conservar responsables, contratos, funciones y comprobaciones físicas pendientes.

Trabajar en rama `feat/sentinel-sanitizador-dns`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. Las dependencias comunes de P1 ya están integradas en develop; comprobar la entrega de cada dependencia funcional aún pendiente antes del cierre.

## Persistencia al implementar este encargo

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.
