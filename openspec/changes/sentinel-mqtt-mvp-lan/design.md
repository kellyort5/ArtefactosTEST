# Design

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## Context

Ver proposal.md. Mosquitto publica actualmente solo en loopback y exige TLS; app, schemas y firmware también lo imponen. El usuario autoriza sustituir esta arquitectura para el MVP. No se presupone administración de la Wi-Fi.

## Goals / Non-Goals

**Goals:** conexión por IP con Android real, mismo flujo de cuentas/ACL y MQTT5, sin certificados, DNS especial o reloj previo; conservar reconexión, límites, persistencia y cierre serializado.

**Non-Goals:** alterar BLE, HTTPS/DoH, modos, broker cloud, router o firewall automáticamente, ni acreditar hardware sin ejecutarlo.

## Decisions

- Mosquitto oficial: listeners TCP1883 y WS9001 y publicación 0.0.0.0 por defecto, acotable con SENTINEL_BIND_IP. Mantener password_file, ACL y volumen. Completar permisos equivalentes node/operator/viewer en v2 porque el nodo actual emite v2; mantener aislamiento y replay v1. El rol notify queda en su encargo. No crear certificados al preparar ni exigirlos al arrancar. Reutilizar cuentas previas sin borrarlas.
- NetworkClient oficial de Arduino para el worker wolfMQTT5 existente; retirar ESP_SSLClient y verificación de fechas exclusiva de MQTT. DNS y sockets siguen acotados y cancelables. Se prefiere esta API oficial a reemplazar SDK o protocolo.
- `configure_connectivity` v1/v2 elimina CA y usa mqtt://. Cambio coordinado del MVP: regenerar schemas/tipos/validador, rechazar formato anterior. Registro NVS con versión nueva, sin migración ni formato automático; USB puede reinicializar según política, o el usuario puede reconfigurar por BLE.
- Android: permitir cleartext en Network Security Config y origen local http://localhost de Capacitor para WS, sin servidor remoto ni contenido mixto innecesario. Mantener confianza HTTPS del sistema; sin CA de laboratorio. La app usa IP LAN ingresada por el operador, no localhost como dirección del PC desde un teléfono. Desarrollo web por HTTP permite WS; HTTPS alojado requerirá otro despliegue futuro.
- Mantener comprobación real TCP/WS, negativos de contraseña/ACL, retained/reinicio y pruebas worker/contratos. Automatizar browser y compilar APK y firmware con herramientas existentes cuando estén disponibles. Una prueba emulada no demuestra radio/teléfono físico.

## Risks / Trade-offs

- Tráfico observable → red de confianza y cuentas exclusivas de laboratorio, sin reutilizar credenciales personales; no exponer Internet.
- Wi-Fi con aislamiento/captive portal o firewall local → diagnóstico explícito; usar otra red/punto de acceso disponible. No prometer funcionamiento en cualquier Wi-Fi ni solicitar tocar router.
- Configuración persistida antigua → rechazo visible, reaprovisionar por BLE o primera carga limpia USB; nunca borrar NVS durante arranque normal.

## Migration Plan

Revisar specs, cambiar contratos y firmware/app/broker juntos, conservar cuentas/volumen, recrear servicio con configuración nueva y reprovisionar nodo. Registrar pruebas y pendientes físicos. Revertir fuentes coordinadamente si se decide volver a TLS; la configuración anterior no se migra automáticamente.

Los deltas de requisitos se alinean con la fuente vigente para no restaurar cuentas/TLS ni retirar escenarios P1 en una sincronización futura. Los resultados históricos y las fechas de esta entrega se conservan; el cambio posterior sentinel-mvp-sin-cuentas establece el alcance actual.
