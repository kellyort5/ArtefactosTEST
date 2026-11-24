# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

Completar la ruta real del DNS local con cifrado TLS y recuperación sin fallback plano.

## What Changes

- Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar sanitizador: doh y protección completa conforme a R-08, H-04 y H-05.
- Cierre observable: Auditoría, NAT, DNS y DoH reales integrados; TLS obligatorio y fallo sin fallback plano.
- Dependencias de integración: `sentinel-sanitizador-dns`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `sanitizer-doh`: Sanitizador: DoH y protección completa para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/sanitizador/ (doh_client y estado de protección)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-doh.md`

No editar carpetas de otros integrantes. Riesgos: Carga TLS/NAT conjunta → comprobar flujo básico y limitar trabajos, no benchmarks previos. Arranque de confianza/hora → estado explícito; prohibido desactivar verificación.
