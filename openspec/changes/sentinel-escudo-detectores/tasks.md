# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Heurísticas y evidencia

- [ ] 1.1 Implementar ventanas y detectores Deauth/desconexión; probar umbral, expiración y enfriamiento con reloj controlado.
- [ ] 1.2 Implementar comparación Evil Twin; probar SSID duplicado benigno y diferencias de seguridad/canal/RSSI.
- [ ] 1.3 Implementar correlación Karma pasiva y caso inconcluso; verificar fixtures positivos/negativos sin inyección de tramas.
- [ ] 1.4 Conectar los cuatro detectores a capture/parser y eventos/métricas v2; verificar que cada familia tiene fixture y salida válida.
- [ ] 1.5 Documentar umbrales, evidencia y límites en docs/modules/escudo-detectors.md; revisar que no se presentan atribuciones concluyentes.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar pio run -d firmware y comprobar observaciones benignas reales recorriendo los detectores; adjuntar pruebas de lógica y recepción BLE.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
