# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Captura y parser

- [ ] 1.1 Implementar callback/cola limitada y generación de captura; verificar descartes y ausencia de trabajo bloqueante en callback.
- [ ] 1.2 Implementar parsers Beacon/Deauth/Disassoc y metadatos Probe; verificar fixtures de longitud/IE/RSN válidos y truncados.
- [ ] 1.3 Integrar scheduler de canales y cobertura; verificar prioridad/rotación con reloj controlado y documentar límites en docs/modules/escudo-capture.md.
- [ ] 1.4 Conectar capture_start/stop y cambio de objetivo; comprobar que no se procesa una generación cerrada.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar pio run -d firmware y observar Beacons reales del objetivo y otra red propia por BLE; registrar canales y contadores.
- [ ] 2.2 Detener/reanudar desde teléfono y comprobar ausencia de asociación/probes enviados por el nodo.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
