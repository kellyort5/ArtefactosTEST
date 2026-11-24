# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

La protección necesita un cliente conectado a Sentinel_Secure con salida IPv4 y estado/contadores reales.

## What Changes

- Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar sanitizador: sta+ap, dhcp y nat conforme a R-08 y H-04.
- Cierre observable: Cliente real con DHCP/NAT y métricas verificadas; cierre del modo sin recursos residuales.
- Dependencias de integración: `sentinel-sanitizador-auditoria`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `sanitizer-gateway`: Sanitizador: STA+AP, DHCP y NAT para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/sanitizador/ (gateway, clients y traffic)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-gateway.md`

No editar carpetas de otros integrantes. Riesgos: NAT y TLS compiten por memoria → límites y comprobación funcional con un cliente; optimizar solo si bloquea. Soporte RSSI/contadores → validar en placa al implementar, conservar métrica explícita desconocida si no observable.
