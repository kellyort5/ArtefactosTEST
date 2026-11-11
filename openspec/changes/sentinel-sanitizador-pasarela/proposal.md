# Proposal

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

No se modifica una capacidad principal existente. Las siete capacidades canónicas de la base están en openspec/specs/ y su cambio está archivado con comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/components/sanitizador/ (gateway, clients y traffic)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-gateway.md`

No editar carpetas de otros integrantes. Riesgos: NAT y TLS compiten por memoria → límites y comprobación funcional con un cliente; optimizar solo si bloquea. Soporte RSSI/contadores → validar en placa al implementar, conservar métrica explícita desconocida si no observable.
