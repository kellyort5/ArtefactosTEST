# Proposal

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## Why

El usuario autoriza retirar TLS del broker para reducir complejidad del MVP, conservando pruebas desde un teléfono Android real en una red de confianza, propia o autorizada, sin administrar el router ni instalar certificados.

## What Changes

- **BREAKING**: MQTT5/TCP en 1883 y MQTT5/WS en 9001 sustituyen MQTT/TLS y WSS. Autenticación, ACL por nodo/rol v1/v2 para el nodo actual, persistencia y contratos de entrega permanecen.
- **BREAKING**: configurar conectividad usa `mqtt://host[:puerto]` sin CA; retirar dependencia y validadores TLS exclusivos de MQTT. El reloj ya no bloquea la conexión, pero sigue usándose para fechas de producto.
- Permitir WS desde Capacitor Android y publicar el broker en LAN; conexión por IP del PC, sin DNS, CA ni cambios de router. Documentar firewall del PC y aislamiento de clientes como límites reales.
- Actualizar contratos, pruebas, guías vigentes y planificación relacionada. Conservar evidencias históricas identificadas como tales. BLE y HTTPS/DoH mantienen sus protecciones.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `secure-mqtt-broker`: transporte LAN sin cifrar con autenticación/ACL.
- `device-connectivity`: configuración sin CA y conexión MQTT independiente del reloj UTC.
- `mobile-control`: Android real con WS por IP sin certificados.
- `message-contracts`: parámetros de conectividad sin CA en v1/v2.

## Impact

Broker/Compose/scripts, firmware Arduino y su persistencia de configuración, biblioteca MQTT conservada, app/Android, contratos generados, pruebas y documentación. Las instalaciones USB pueden reinicializarse sin migración según la política aprobada; reprovisionar el formato nuevo. Riesgo aceptado: credenciales y mensajes visibles para quien observe el enlace. La red debe permitir comunicación entre clientes; el software no elimina aislamiento Wi-Fi. Sin cambios a router, exposición Internet, servicios cloud, BLE, DoH o funcionalidades de módulos.

Los deltas de requisitos se alinean con la fuente vigente para no restaurar cuentas/TLS ni retirar escenarios P1 en una sincronización futura. Los resultados históricos y las fechas de esta entrega se conservan; el cambio posterior sentinel-mvp-sin-cuentas establece el alcance actual.
