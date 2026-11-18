# Tasks

Responsable único de todas las casillas: **Persona 4**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. LAN y ACL

- [ ] 1.1 Verificar acceso LAN por IP a TCP1883/WS9001 sin certificados; conservar cuentas y documentar firewall local y aislamiento de clientes.
- [ ] 1.2 Añadir ACL v2 y rol notify preservando replay v1/events; probar cada permiso y rechazo cross-node con cuentas temporales.
- [ ] 1.3 Documentar Compose, host/firewall y cuentas en broker/README.md y docs/modules/broker-lan.md; verificar comandos sin secretos reales.
- [ ] 1.4 Ejecutar comprobación de configuración y smoke TCP/WS; registrar resultados con broker real.

## 2. Comprobación del incremento

- [ ] 2.1 Conectar teléfono y ESP32 por LAN autorizada por IP sin certificados; registrar identidad/topic permitido sin credenciales.
- [ ] 2.2 Comprobar que el broker no publica HTTP/Telnet y entregar a P3 la configuración local de rol por canal privado fuera de Git.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
