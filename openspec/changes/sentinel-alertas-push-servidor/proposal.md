# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

La recepción con Android cerrado requiere proveedor push y registro seguro de instalaciones, además de alertas MQTT.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar push: registro y envío en servidor conforme a R-10 y H-08.
- Cierre observable: API real de registro/baja y envío FCM; aceptación verificada y recepción final a cargo del encargo Android.
- Dependencias de integración: `sentinel-alertas-webhook`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `push-delivery`: Push: registro y envío en servidor para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `notifications/ (registro, proveedor FCM y configuración)`
- `notifications/test/`
- `docs/modules/push-server.md`

No editar carpetas de otros integrantes. Riesgos: Proveedor/proyecto externo → entradas de despliegue documentadas y revisión de elección, no credenciales ficticiamente operativas. Tokens y permisos → baja/renovación y pruebas de acceso cruzado.
