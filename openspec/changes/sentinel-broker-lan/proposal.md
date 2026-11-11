# Proposal

## Why

El broker base usa certificados localhost y ACL v1; ESP32/teléfono y notificaciones necesitan nombres LAN y permisos concretos.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar broker: lan, v2 y rol de alertas conforme a R-04.
- Cierre observable: TLS/WSS LAN real y ACL v1/v2 por nodo, con rol mínimo de notificaciones.
- Dependencias de integración: `sentinel-module-contracts`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `broker-module-access`: Broker: LAN, v2 y rol de alertas para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `broker/ (config, scripts, pruebas y README)`
- `docs/modules/broker-lan.md`

No editar carpetas de otros integrantes. Riesgos: SAN/CA y red host → comprobar teléfono/ESP en LAN; localhost de emulador no lo sustituye. ACL demasiado amplia → test cruzado por nodo y rol.
