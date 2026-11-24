# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

Los servicios Cebo requieren visualización MQTT en tiempo real, historial y visibilidad del envío de alertas.

## What Changes

- Responsable único: **Persona 4 — app, Android, broker y coordinación del laboratorio**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Entregar app: cebo, historial y alertas conforme a R-04, R-06, R-10 y H-08.
- Cierre observable: Cebo real visible por MQTT, control confirmado, historial y alertas internas/estado externo.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-cebo-eventos-historial`, `sentinel-alertas-webhook`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-cebo`: App: Cebo, historial y alertas para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

No se modifica una capacidad principal existente. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/cebo/`
- `app/src/ (registro de ruta y store Cebo)`
- `app/test/cebo*`
- `docs/modules/app-cebo.md`

No editar carpetas de otros integrantes. Riesgos: Historial retenido limitado → gap visible, no historial infinito. Estado backend/nodo diferente → presentación separada sin falsos eventos del ESP32.
