# Tasks

Responsable único de todas las casillas: **Persona 4**. Base de software ya entregada por P1; P4 verifica despliegue del equipo e integración física. Casillas conservadas pendientes de su evidencia.

## 1. LAN e integración anónima

- [ ] 1.1 Reutilizar Compose anónimo entregado y comprobar acceso LAN por IP TCP1883/WS9001; documentar firewall local y aislamiento de clientes en la red del equipo.
- [ ] 1.2 Verificar intercambio v1/v2 y suscripción anónima de P3; documentar que cualquier cliente con acceso al broker puede leer y publicar.
- [ ] 1.3 Documentar IP/topología, puertos y volumen en broker/README.md y docs/modules/broker-lan.md; comprobar el arranque directo con los comandos entregados.
- [ ] 1.4 Repetir configuración y smoke TCP/WS sobre la referencia integrada y registrar resultados con broker real.

## 2. Comprobación del incremento

- [ ] 2.1 Conectar teléfono y ESP32 al broker anónimo por la LAN autorizada; registrar nodo/topic y resultado físico sin confundirlo con fixtures.
- [ ] 2.2 Comprobar que no se publican HTTP/Telnet y entregar a P3 endpoints/topics de MQTT anónimo para sus alertas.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
