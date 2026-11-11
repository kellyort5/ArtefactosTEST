# Proposal

## Why

La recepción con Android cerrado requiere proveedor push y registro seguro de instalaciones, además de alertas MQTT.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar push: registro y envío en servidor conforme a R-10 y H-08.
- Cierre observable: API real de registro/baja y envío FCM; aceptación verificada y recepción final a cargo del encargo Android.
- Dependencias de integración: `sentinel-alertas-webhook-telegram`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `push-delivery`: Push: registro y envío en servidor para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `notifications/ (registro, proveedor FCM y configuración)`
- `notifications/test/`
- `docs/modules/push-server.md`

No editar carpetas de otros integrantes. Riesgos: Proveedor/proyecto externo → entradas de despliegue documentadas y revisión de elección, no credenciales ficticiamente operativas. Tokens y permisos → baja/renovación y pruebas de acceso cruzado.
