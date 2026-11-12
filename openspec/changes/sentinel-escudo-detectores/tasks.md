# Tasks

Responsable único de todas las casillas: **Persona 1**. Estado: **5/6 tareas verificadas en software**; la comprobación física **2.1 sigue pendiente**, conservada por instrucción del usuario. Ver [evidencia](verification.md).

## 1. Heurísticas y evidencia

- [x] 1.1 Implementar ventanas y detectores Deauth/desconexión; probar umbral, expiración y enfriamiento con reloj controlado.
- [x] 1.2 Implementar comparación Evil Twin; probar SSID duplicado benigno y diferencias de seguridad/canal/RSSI.
- [x] 1.3 Implementar correlación Karma pasiva y caso inconcluso; verificar fixtures positivos/negativos sin inyección de tramas.
- [x] 1.4 Conectar los cuatro detectores a capture/parser y eventos/métricas v2; verificar que cada familia tiene fixture y salida válida.
- [x] 1.5 Documentar umbrales, evidencia y límites en docs/modules/escudo-detectors.md; revisar que no se presentan atribuciones concluyentes.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar pio run -d firmware y comprobar observaciones benignas reales recorriendo los detectores; adjuntar pruebas de lógica y recepción BLE.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
