# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

R-08 requiere diagnosticar DNS, HTTPS y portal cautivo antes de habilitar una pasarela protegida.

## What Changes

- Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar sanitizador: auditoría de conectividad conforme a R-08, R-09 y H-05.
- Cierre observable: Auditoría DNS/HTTPS/portal real por BLE, con motivos e inconclusos correctos.
- Dependencias de integración: `sentinel-firmware-module-api`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `sanitizer-audit`: Sanitizador: auditoría de conectividad para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/sanitizador/ (module y audit)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-audit.md`

No editar carpetas de otros integrantes. Riesgos: Portal y DNS divididos → resultado inconcluso y límites explícitos. Credenciales y TLS → solo BLE, configuración propia en NVS oficial según política posterior, sin secretos en logs/event/status ni promesa de cifrado físico; no bypass de certificados.
