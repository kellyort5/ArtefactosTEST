# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

La base Cebo solo conecta MQTT; falta un servicio HTTP real seguro que genere interacciones normalizadas.

## What Changes

- Responsable único: **Persona 3 — firmware Cebo y servidor de notificaciones**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar cebo: panel http ficticio conforme a R-10 y H-06.
- Cierre observable: Panel ESP32 accesible y eventos sanitizados; recursos acotados y cierre real.
- Dependencias de integración: `sentinel-firmware-module-api`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `cebo-http`: Cebo: panel HTTP ficticio para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/cebo/ (module y http_service)`
- `firmware/test/cebo/`
- `docs/modules/cebo-http.md`

No editar carpetas de otros integrantes. Riesgos: Credenciales recibidas → descarte en memoria y pruebas que buscan marcadores de secreto en salidas. Conexiones lentas → límites/timeout y cierre al salir.
