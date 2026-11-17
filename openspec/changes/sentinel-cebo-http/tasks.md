# Tasks

Responsable único de todas las casillas: **Persona 3**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Servicio y privacidad

- [ ] 1.1 Crear módulo y bus interno de interacciones Cebo; compilar con hooks sin editar núcleo ni controladores de red.
- [ ] 1.2 Implementar panel/rutas/formulario con esp_http_server; verificar accesos y auth_attempt desde cliente LAN propio.
- [ ] 1.3 Añadir límites, sanitización y descarte de datos; probar marcador ficticio de secreto ausente en todas las salidas.
- [ ] 1.4 Documentar rutas, límites y datos registrados en docs/modules/cebo-http.md; cotejar evento con v2.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar `firmware/SentinelNode` desde Arduino IDE Windows con la placa `Sentinel ESP32-S3 N16R8 (secure SDK)` y comprobar HTTP real y eventos desde teléfono/PC en LAN autorizada.
- [ ] 2.2 Detener Cebo/cambiar modo y comprobar puerto 80 cerrado, sin ejecución ni exposición a Internet.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
