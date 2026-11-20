# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

El usuario necesita seleccionar redes y ver todos los detectores/cobertura del Escudo real, no solo tarjetas de diagnóstico.

## What Changes

- Responsable único: **Persona 1, por autorización expresa para implementar este encargo originalmente de P4**. Los demás revisan y consumen su entrega, sin compartir autoría del cambio.
- Retirar por petición del usuario el panel temporal Diagnóstico P1, su flag y sus APIs; las pruebas negativas continúan en software y la salida serie de firmware se conserva.
- Entregar app: escudo completo conforme a R-03, R-06 y H-08.
- Cierre observable: Pantalla Escudo completa integrada por BLE, con control, gráficos, incidentes y recuperación reales.
- Dependencias de integración: `sentinel-app-protocolo-v2`, `sentinel-escudo-descubrimiento`, `sentinel-escudo-detectores`, `sentinel-escudo-recuperacion`.
- Esta propuesta solo planifica. No implementa funcionalidades ajenas a la capacidad ni modifica la evidencia de la base.

## Capabilities

### New Capabilities

- `app-escudo`: App: Escudo completo para completar la entrega integrada de Sentinel Node.

### Modified Capabilities

Se retira de `mobile-control` el requisito de diagnóstico P1 optativo, sustituido por la pantalla normal. Las capacidades consolidadas están en openspec/specs/; los cierres archivados conservan sus comprobaciones pendientes aceptadas. Este cambio añade una capacidad complementaria, sin reducir R-03/R-08/R-10.

## Impact

Archivos de futura implementación bajo propiedad de este autor:

- `app/src/features/escudo/`
- `app/src/ (registro de ruta y store Escudo)`
- `app/test/escudo*`
- `docs/modules/app-escudo.md`

No editar carpetas de otros integrantes. Riesgos: UI puede adelantar firmware → distinguir fixture de dato real y no marcar cierre por fixture. Alertas ambiguas → evidencia/indicio y SSID duplicado sin etiqueta malicioso.
