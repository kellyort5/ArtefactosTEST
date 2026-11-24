# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado: 4/6 tareas verificadas por software; pruebas físicas 2.1 y 2.2 pendientes por instrucción del usuario. Ver [evidencia](verification.md).

## 1. Catálogo y selección

- [x] 1.1 Registrar hooks Escudo y scan pasivo mediante gestor; compilar y verificar callback de fin sin asociación.
- [x] 1.2 Implementar catálogo limitado y páginas v2; comprobar casos de SSID oculto, duplicado BSSID y overflow con fixtures focalizados.
- [x] 1.3 Implementar selección/cambio y ACK; verificar rechazo de BSSID/canal inconsistente sin mutar objetivo.
- [x] 1.4 Documentar operaciones y datos no observables en docs/modules/escudo-discovery.md; cotejar ejemplos con contratos.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar pio run -d firmware y comprobar catálogo/selección por BLE en ESP32 con LAN propia; registrar datos y ausencia STA/MQTT.
- [ ] 2.2 Comprobar salida a Idle durante scan y repetir un scan; registrar comportamiento y errores corregidos.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
