# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

Ver [motivación](proposal.md). La referencia de integración es `develop`: contratos v2, API de módulos, Escudo y app común/diagnóstico P1 ya están entregados por software. Consultar [estado y límites vigentes](../../../docs/mvp-policy.md) y [verificación integrada](../sentinel-mvp-sin-cuentas/verification.md). La base y los incrementos archivados conservan sus comprobaciones físicas pendientes; este cambio no las acredita. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Dependencias para integrar: `sentinel-app-protocolo-v2`, `sentinel-app-cebo`, `sentinel-alertas-push-servidor`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Push real recibido con app cerrada y tap al incidente, con registro/baja seguros.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-10 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Propuesta a revisar: plugin oficial @capacitor/push-notifications compatible con Capacitor8 y FCM del servidor, fijando versión y lock al implementar. Android usa FCM y requiere configuración de un proyecto propio; referencia [plugin oficial](https://capacitorjs.com/docs/apis/push-notifications). Mantener google-services.json real fuera de Git y documentar instalación local; ejemplo sin valores reales. No incluir claves Admin en app.

Pedir permiso Android13+, registrar token mediante HTTPS autenticado del contrato, renovar al cambiar y dar de baja al desvincular instalación. Token transitorio/local privado mínimo, nunca log. Manejar recepción foreground, notification tray en background/cerrada por recientes y tap para abrir Cebo por event_id. Si el evento no está local, sincronizar historial autorizado cuando haya canal, mostrar pendiente/retención si no se consigue; no fabricar detalle. Test real obligatorio con app retirada de recientes, permiso otorgado y red; force-stop manual del sistema y restricciones del fabricante se documentan aparte sin promesa universal. Emulador apoya UI, no acredita recepción real cerrada.

La clave técnica de acceso HTTPS se conserva conforme al contrato de notificaciones; no crear usuarios, roles ni permisos de aplicación. P3 proporciona el endpoint/configuración privada a P4; MQTT permanece anónimo y no requiere certificados del broker.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../archive/2026-11-11-sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `app/src/features/notifications/`, `app/src/ (integración de recepción/navegación)`, `app/android/ (fuentes y configuración pública)`, `app/package.json`, `package-lock.json`, `app/test/push*`, `docs/modules/app-push.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Servicios Google/permisos/batería → comprobación funcional en teléfono concreto y límites documentados.
- Tap y carrera de sincronización → navegación con ID pendiente sin inventar incidente.

## Migration Plan

Trabajar en rama `feat/sentinel-app-push`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. Las dependencias comunes de P1 ya están integradas en develop; comprobar la entrega de cada dependencia funcional aún pendiente antes del cierre.
