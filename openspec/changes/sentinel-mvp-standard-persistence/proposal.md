# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## Why

El usuario redefine el MVP: identidad, vinculos BLE, configuracion e historial persisten durante operacion, reinicios y cortes de alimentacion, pero pueden reinicializarse al programar por USB. Ya no hace falta preservar datos entre versiones ni mantener recetas especiales para esa compatibilidad.

## What Changes

- Adoptar partitions.csv compatible con las direcciones estandar de Arduino3.3.12, con aplicacion4MiB, NVS24KiB e historial256KiB.
- Eliminar recetas propias de carga/exportacion; conservar solamente selector/limite4MiB y wrapper NVS justificado por la proteccion durante arranque normal.
- Hacer persistente la seleccion de objetivo Escudo y la configuracion Wi-Fi/MQTT autorizada; restaurarla sin activar modos ni confiar en un reloj UTC anterior.
- Actualizar instalador para retirar claves obsoletas propias conservando configuracion ajena, y preparar carga limpia inicial sin migracion.
- Extender la politica a arquitectura, decisiones, guias, encargos y planes OpenSpec activos; registrar la regla de soluciones estandar primero.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `device-connectivity`: configuracion validada persistente con privacidad, restauracion y errores visibles.
- `escudo-discovery`: conservar la seleccion aceptada sin guardar catalogos ni reanudar captura automaticamente.
- `event-journal`: mantener recuperacion durante operacion normal y explicitar excepcion de reinstalacion USB.
- `project-workspace`: preparacion/carga/exportacion estandar y politica de persistencia diferenciada.

## Impact

Firmware, soporte Arduino, pruebas focalizadas y documentos/encargos. Sin nuevos protocolos, dependencias, SDK, cambios de seguridad MQTT/BLE ni reduccion de modulos. La configuracion autorizada se guarda en NVS estandar: cambia la decision anterior de mantener esas credenciales solo en RAM; no se promete cifrado fisico de flash. No guardar secretos en Git, logs o eventos. La distribucion nueva requiere primera programacion limpia; no se migran datos anteriores. No se programa hardware no identificado ni se hacen commits/push/merge.
