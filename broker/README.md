# Mosquitto del MVP: LAN sin TLS ni cuentas

MQTT5/TCP **1883**, WebSocket **9001**. Conecta nodo, app y notificaciones sin usuario, contraseña, roles, ACL, certificados ni preparación de secretos. Usa la IP LAN del PC desde el teléfono; localhost en el teléfono no es el PC.

```sh
python3 broker/scripts/manage.py config
python3 broker/scripts/manage.py up
python3 broker/scripts/manage.py ps
node broker/scripts/smoke.mjs --restart
```

Con Docker nativo también: `docker compose -f broker/compose.yaml up -d`. El helper selecciona Docker Desktop Windows cuando el socket WSL no está disponible; no instala otro motor. La imagen y el entrypoint son oficiales. El volumen `mqtt-data` se conserva; no ejecutar down -v para reiniciar. Configuración fuente en config/mosquitto.conf.

Publicación LAN por defecto 0.0.0.0; `SENTINEL_BIND_IP` permite acotarla. El PC/teléfono/ESP32 deben poder comunicarse en la red propia/autorizada. Permitir los puertos pertinentes en el firewall local si hace falta. Wi-Fi con aislamiento requiere otra red/punto de acceso disponible; no se elimina aislamiento por software ni hace falta administrar router o DNS.

**Límite aprobado:** cualquier cliente que alcance el broker puede leer/publicar en cualquier topic, incluidos comandos y telemetría. El ID/topic del nodo enruta y no autentica. No hay confidencialidad, autenticidad del emisor ni aislamiento entre nodos. Usar LAN de confianza y no exponer estos puertos a Internet. Boot/revisión/TTL, deduplicación y ACK siguen verificándose en los consumidores; no sustituyen autenticación.

No se usan ni montan broker/.secrets, clients.json, password_file, ACL o CA antiguos. Se conservan fuera de Git si ya existen; no son un paso de instalación. Los scripts de generación/gestión de cuentas fueron retirados. El usuario Linux del proceso Mosquitto sigue existiendo por ser parte de la imagen, no una cuenta MQTT.

Endpoints: `mqtt://IP-DEL-PC:1883` (ESP32) y `ws://IP-DEL-PC:9001/mqtt` (app). Contratos v1/v2 en [contracts](../contracts/module-protocol.md). Configuración Wi-Fi y URI se suministra al ESP32 por BLE protegido. BLE, HTTPS/DoH y claves de proveedores externos conservan sus requisitos.

P4 verifica acceso físico LAN y suscripciones de P3; no implementa cuentas/ACL. La entrega P1 y sus pendientes están en [la guía](../docs/guia-pruebas-persona-1.md). [Política vigente](../docs/mvp-policy.md).
