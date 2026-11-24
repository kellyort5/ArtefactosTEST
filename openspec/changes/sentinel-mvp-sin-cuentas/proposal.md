# Proposal

## Why

El usuario aprueba integrar la entrega P1, actualizar el reparto y simplificar el MVP eliminando usuarios, contraseñas, roles y ACL MQTT en todos sus consumidores. TLS MQTT ya fue retirado; se conserva la seguridad BLE y de HTTPS/DoH.

## What Changes

- **BREAKING**: Mosquitto anónimo en TCP1883/WS9001, sin password_file, ACL ni preparación de cuentas. Conservar volumen y flujo MQTT5.
- **BREAKING**: configure_connectivity v1/v2 solo recibe SSID, contraseña Wi-Fi y mqtt_uri. Retirar cuentas MQTT de firmware/NVS, app, contratos, tests y documentación operativa.
- Eliminar operador/observador de UI/store/transportes. Mantener validación de nodo/topic, boot/revisión/TTL, idempotencia, BLE protegido y exclusión de modos; estas comprobaciones no autentican al emisor MQTT.
- Integrar P1 y correcciones de memoria/pila BLE conservando su prueba física 4.3 pendiente; actualizar responsabilidades y encargos sin duplicar trabajo ya entregado ni eliminar los tres módulos.
- Actualizar las specs principales y la planificación activa. Conservar informes/archivos históricos identificados como antecedentes; no reescribir sus resultados.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `secure-mqtt-broker`: acceso anónimo sin TLS ni roles; nombre histórico de ruta conservado.
- `device-connectivity`: provisión y persistencia sin cuenta MQTT.
- `mobile-control`: conexión por endpoint/nodo sin login o selector de rol.
- `message-contracts`: argumentos mínimos y admisión MQTT sin identidad autenticada.

## Impact

Broker, firmware, app/Android, contratos, pruebas, fuentes OpenSpec y guías de las cuatro personas. Riesgo explícitamente aceptado: cualquier cliente que alcance el broker puede leer y publicar, incluidos comandos Cebo; device_id no es autenticación y no hay aislamiento entre nodos. Uso solo en LAN propia/autorizada de confianza, sin exposición Internet. No retirar passkey/bonds BLE, permisos Android indispensables, credenciales Wi-Fi, claves de proveedores push/webhook ni TLS de DoH/HTTPS. No borrar NVS, volumen o secretos locales antiguos automáticamente. El usuario autoriza commit, push e integración en develop mediante PR, sin modificar main ni forzar historial.
