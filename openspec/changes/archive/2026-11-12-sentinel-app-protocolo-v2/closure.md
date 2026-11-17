# Cierre de sentinel-app-protocolo-v2

Archivado el 2026-11-12 por instrucción del usuario, con sincronización de los tres requisitos de [app-module-protocol](../../../specs/app-module-protocol/spec.md). El usuario aprobó expresamente conservar la tarea física **2.1 pendiente**; no se marcó como ejecutada.

- Implementación integrada en `develop` mediante [PR #11](https://github.com/Jefry-P/ArtefactosProyecto/pull/11), fusionado el 2026-11-13 a las 04:56 UTC (2026-11-12 en Guatemala). La recuperación previa se integró mediante PR #10. Este archivo no realiza merges.
- Seguimiento preservado: **5/6 tareas** en [tasks.md](tasks.md), con **2.1 abierta** por faltar control real con ESP32/teléfono. La alternativa prevista en 2.2 permite compilar y documentar pendientes; permisos, pairing y BLE físicos siguen sin acreditar.
- [Evidencia del incremento](verification.md): 26 tests de app, 154 de contratos, build web, Chromium y build/sync Android aprobados. Fixtures/build no sustituyen hardware ni MQTT v2 real.
- La sincronización conserva Purpose, tres requisitos y sus escenarios: adaptación de versiones, control confirmado, historial y privacidad. Las demás specs principales permanecen intactas.
- Para ejecutar y registrar el pendiente, seguir la [guía de pruebas de Persona 1](../../../../docs/guia-pruebas-persona-1.md), especialmente instalación/pairing y T03; T14/T15 cubren reconexión e historial básico.
- Pantallas específicas y ACL v2 continúan con P4; Cebo y Sanitizador conservan sus autores y alcance. Archivar este adaptador común no declara terminado el MVP ni las pruebas físicas de esos componentes.
