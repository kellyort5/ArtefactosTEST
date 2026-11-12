# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado: 6/7 tareas verificadas por software; 2.2 pendiente por falta de placa accesible. Ver [evidencia](verification.md).

## 1. Interfaz integrable

- [x] 1.1 Crear tipos y hooks de sentinel_api y documentar propiedad/lifetime; verificar compilación de un consumidor mínimo de prueba.
- [x] 1.2 Extender supervisor y despacho v2 con registro opcional; verificar pruebas de exclusión, autorización y capacidad ausente.
- [x] 1.3 Adaptar gestor Wi-Fi/BLE, préstamo de netifs y flags globales IP_FORWARD/IPV4_NAPT para tres perfiles; compilar y comprobar que solo el gestor inicia/detiene drivers y que NAPT permanece inactivo fuera de Sanitizador.
- [x] 1.4 Conectar hooks de evento/métrica al journal y serializador; verificar rechazo de detalle inválido y límite de cola.
- [x] 1.5 Documentar hooks y registro en docs/development.md con ejemplo de ciclo de vida; verificar la ruta de integración de P2/P3.

## 2. Integración del núcleo

- [x] 2.1 Verificar parada con timeout, error visible y botón/OLED para perfiles registrados mediante prueba focalizada y compilación pio run -d firmware.
- [ ] 2.2 En ESP32, comprobar Idle y una transición de base por BLE sin servicios residuales; registrar evidencia o mantener pendiente si falta hardware.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
