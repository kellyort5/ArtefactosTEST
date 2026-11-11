# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Dependencias para integrar: `sentinel-alertas-webhook`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** API real de registro/baja y envío FCM; aceptación verificada y recepción final a cargo del encargo Android.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-10 y H-08 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

Propuesta a revisar: FCM con SDK Admin oficial para enviar al Android Capacitor existente. Reutilizar enrutador/outbox/API del componente; no desarrollar servicio propio de notificaciones del sistema ni exigir MQTT permanente en Android. El equipo proporciona proyecto Firebase y credencial propia fuera de Git durante implementación; no hay una cuenta ya creada ni entrega probada. Revisar con usuario esta elección antes de dependencias. Referencia: [recepción FCM Android](https://firebase.google.com/docs/cloud-messaging/android/receive-messages).

Implementar POST/DELETE de registrations y estado según contrato. Registro máximo32 instalaciones, TTL30 días renovable; token en almacenamiento privado con permisos restrictivos, nunca respuesta/log/evento. Autorización por nodo con token propio configurado fuera de repo; actualizar por instalación, eliminar token inválido/expirado o baja. Enviar notification+data con event_id/device_id y contenido genérico de Cebo; no enviar IP privada, credenciales ni cuerpo de solicitud al proveedor. Reintentos/dedup por evento/canal reutilizados. FCM aceptado significa entregado al proveedor, no garantía de lectura. Compose/HTTPS vive en notifications/, P4 solo consume API.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `notifications/ (registro, proveedor FCM y configuración)`, `notifications/test/`, `docs/modules/push-server.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Proveedor/proyecto externo → entradas de despliegue documentadas y revisión de elección, no credenciales ficticiamente operativas.
- Tokens y permisos → baja/renovación y pruebas de acceso cruzado.

## Migration Plan

Trabajar en rama `feat/sentinel-alertas-push-servidor`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
