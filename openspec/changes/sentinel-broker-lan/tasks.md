# Tasks

Responsable único de todas las casillas: **Persona 4**. Base de software ya entregada por P1; P4 verifica despliegue del equipo e integración física. Casillas conservadas pendientes de su evidencia.

## 1. LAN e integración anónima

- [ ] 1.1 Reutilizar Compose anónimo entregado y comprobar acceso LAN por IP TCP1883/WS9001; documentar firewall local y aislamiento de clientes en la red del equipo.
- [ ] 1.2 Verificar intercambio v1/v2 y suscripción de notificaciones de P3 sin cuentas ni ACL; documentar que no hay aislamiento entre nodos.
- [ ] 1.3 Documentar IP/topología, puertos y volumen en broker/README.md y docs/modules/broker-lan.md; comprobar comandos sin preparación de cuentas/CA.
- [ ] 1.4 Repetir configuración y smoke TCP/WS sobre la referencia integrada y registrar resultados con broker real.

## 2. Comprobación del incremento

- [ ] 2.1 Conectar teléfono y ESP32 por LAN autorizada sin TLS ni cuentas; registrar nodo/topic y resultado físico sin confundirlo con fixtures.
- [ ] 2.2 Comprobar que no se publican HTTP/Telnet y entregar a P3 endpoints/topics de MQTT anónimo para sus alertas; sin rol notify.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
