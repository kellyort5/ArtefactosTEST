# Sentinel Node

Base real de control para ESP32-S3 N16R8, broker Mosquitto protegido y aplicación React/Capacitor Android. Esta entrega prepara perfiles de recursos y contratos; captura, detectores, HTTP/Telnet, notificaciones externas y Sanitizador siguen pendientes de sus cambios. No declara terminado el MVP.

La [base archivada](openspec/changes/archive/2026-11-10-sentinel-core-foundation/closure.md) conserva **26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario** bajo la suposición de que el hardware funciona. Las pruebas físicas no ejecutadas siguen sin acreditarse; el [informe de cierre](docs/core-foundation-check.md) separa evidencia y aceptación.

## Preparación

Node **24.13.1**, npm **11.8.0**, JDK21 y Android SDK36 para la app. Firmware y broker usan sus propias herramientas. Desde la raíz:

```sh
nvm use
npm ci
npm run check:contracts
npm run test:app
npm run build
npm run dev
```

Workspaces npm privados: `app/` y `contracts/`, un solo `package-lock.json`. `firmware/` y `broker/` son independientes; `simulator/` conserva el límite de integración futuro, sin bloquear hardware.

- [Desarrollo y scripts](docs/development.md)
- [Contratos v1, ejemplos y API](contracts/README.md)
- [Aplicación, Android y confianza TLS](app/README.md)
- [Firmware, cableado, build/upload/monitor](docs/firmware.md)
- [Broker local, certificados y permisos](broker/README.md)
- [Conectividad BLE y MQTT del nodo](docs/connectivity.md)
- [Evidencia de app y contratos](app/evidence.md)
- [Comprobación integral](docs/core-foundation-check.md)
- [Guía de prueba con ESP32 y Android](docs/core-foundation-test-guide.md)
- [Brief](docs/brief.md), [arquitectura](docs/architecture.md), [decisiones](docs/technical-decisions.md), [roadmap](docs/roadmap.md) y [contribución](CONTRIBUTING.md)

Los secretos de laboratorio se suministran en ejecución y permanecen en rutas ignoradas y RAM según el diseño. Todas las pruebas de redes requieren un entorno propio/autorizado. El usuario autorizó continuar con teléfono, ESP32 y emulador Android. La guía de prueba indica las comprobaciones y el informe integral conserva sus resultados observados.
