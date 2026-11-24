# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

El tráfico DNS atendido por la pasarela debe aplicar reglas, caché y contadores antes de integrar el reenvío cifrado.

## What Changes

- Responsable único: **Persona 2 — firmware Auditor/Sanitizador**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar sanitizador: dns local y filtrado conforme a R-08 y H-05.
- Cierre observable: DNS local real, reglas/caché probadas y ningún fallback DNS plano; DoH aún explícitamente pendiente.
- Dependencias de integración: `sentinel-sanitizador-pasarela`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `sanitizer-dns`: Sanitizador: DNS local y filtrado para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `firmware/SentinelNode/src/sanitizador/ (dns_server, rules y dns_cache)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-dns.md`

No editar carpetas de otros integrantes. Riesgos: Paquetes DNS malformados → tests de compresión/ciclos/límites. Evasión de DNS → documentar cobertura sin prometer filtrado de todo tráfico.
