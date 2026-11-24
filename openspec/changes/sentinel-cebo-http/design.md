# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

Ver [motivación](proposal.md). La referencia de integración es `develop`: contratos v2, API de módulos, Escudo y app común/diagnóstico P1 ya están entregados por software. Consultar [estado y límites vigentes](../../../docs/mvp-policy.md) y [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md). La base y los incrementos archivados conservan sus comprobaciones físicas pendientes; este cambio no las acredita. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Dependencias para integrar: `sentinel-firmware-module-api`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Panel ESP32 accesible y eventos sanitizados; recursos acotados y cierre real.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-10 y H-06 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Registrar Cebo por sentinel_api y esperar LAN autorizada del gestor. Usar esp_http_server puerto 80, panel estático ficticio en / y /admin, formulario /login sin autenticación administrativa real. Toda entrada recibe respuesta fija segura; nada se ejecuta. Límite inicial 4 sockets HTTP, timeout 5 s y cuerpo procesable ≤1024; limpiar porciones transitorias de body inmediatamente. Nunca persistir usuarios/contraseñas/cookies/Authorization ni URLs con query; guardar solo ruta canónica de lista conocida o null.

Emitir interacción de conexión/acceso/auth_attempt con IP/puertos/tiempo y metadatos, mediante interfaz compartida de Cebo que la agregación del siguiente cambio consume. No escribir directamente journal/MQTT. Inicio/parada por cebo.start/stop requieren perfil Cebo y red lista; parada cancela sockets antes de liberar modo. Sin exposición pública, NAT entrante ni panel de administración real. Alternativa de un mock HTTP de PC no satisface el artefacto ESP32.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/SentinelNode/src/cebo/ (module y http_service)`, `firmware/test/cebo/`, `docs/modules/cebo-http.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Credenciales recibidas → descarte en memoria y pruebas que buscan marcadores de secreto en salidas.
- Conexiones lentas → límites/timeout y cierre al salir.

## Migration Plan

Flujo de firmware: Arduino IDE Windows con core oficial ESP32 **3.3.12**, placa **ESP32S3 Dev Module**, flash **16 MB**, QIO y PSRAM OPI, sketch `firmware/SentinelNode/SentinelNode.ino`. Seguir [configuración y bibliotecas vigentes](../../../docs/firmware.md) y [desarrollo de módulos](../../../docs/development.md). Usar los módulos C/C++ de `src/`, `SentinelConfig.h` y las factories existentes; conservar APIs ESP-IDF/FreeRTOS y el único gestor de recursos. El núcleo y los ajustes globales pertenecen a P1; P2/P3 solicitan cambios acotados, sin editar core ni crear otro gestor Wi-Fi/BLE. No se requiere SDK propio. Conservar responsables, contratos, funciones y comprobaciones físicas pendientes.

Trabajar en rama `feat/sentinel-cebo-http`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. Las dependencias comunes de P1 ya están integradas en develop; comprobar la entrega de cada dependencia funcional aún pendiente antes del cierre.

## Persistencia al implementar este encargo

P3 persistirá configuración aceptada de Cebo e historial sanitizado mediante recursos centrales, con límites, commit antes de confirmar y recuperación tras reinicio/corte sin USB; no iniciar servicios automáticamente. Mantener eventos/IDs/cursores y dedup dentro del ring. No guardar credenciales recibidas por HTTP/Telnet/honeypot o terceros. NVS estándar no promete cifrado físico; secretos propios solo por canal autorizado, nunca Git/log/event/status. Webhook y push mantienen sus encargos completos.
