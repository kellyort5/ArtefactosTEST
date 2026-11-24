# Proposal

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Why

El equipo necesita comprobar acceso físico en su LAN para ESP32, Android e integración de notificaciones. P1 ya entrega Compose y transportes TCP/WS anónimos; no corresponde reimplementar la base o crear cuentas.

## What Changes

- Reutilizar TCP1883/WS9001 por IP y documentar red/firewall/aislamiento reales.
- Verificar tráfico v1/v2 y suscripción de P3, sin TLS/CA, usuarios, contraseñas MQTT, roles o ACL.
- Conservar volumen, eventos históricos y límite de no exponer HTTP/Telnet a Internet.
- Registrar evidencia física y distinguirla del smoke de software. Un responsable: P4.

## Capabilities

### New Capabilities

- `broker-module-access`: integración LAN anónima del equipo con clientes y notificaciones reales.

### Modified Capabilities

Ninguna.

## Impact

P4 modifica broker y documentación de su despliegue cuando sea necesario, sin tocar contratos/firmware de P1 o servidor de P3. La autenticación MQTT fue retirada por el usuario; cualquier cliente con acceso al broker puede leer/publicar. Solo LAN propia/autorizada de confianza, sin exposición Internet. No cerrar con fixtures las pruebas físicas pendientes.
