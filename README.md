# Sentinel Node

**Acuerdo vigente (2026-11-17):** [MQTT anónimo, entrega P1 y reparto actualizado](docs/mvp-policy.md). Sin TLS/cuentas/roles/ACL MQTT; BLE y HTTPS/DoH conservan sus protecciones. Evidencia histórica no sustituye las pruebas físicas pendientes.

Sistema modular de seguridad de redes para **ESP32-S3-DevKitC-1 N16R8**, broker Eclipse Mosquitto en Docker y aplicación React/Capacitor Android. Un supervisor permite ejecutar un único modo a la vez: Escudo Pasivo, Auditor/Sanitizador o Cebo de Red Local. Solo se utiliza en redes propias o autorizadas.

**Para empezar:** [mini setup Windows: Arduino IDE, Mosquitto y app](docs/mini-setup.md).

## Estado actual

Estado de esta entrega: **2026-11-17**. La base y el Escudo están implementados en firmware; el MVP completo de tres módulos todavía requiere las entregas del equipo y la comprobación física.

| Componente | Implementado y comprobado por software | Pendiente |
| --- | --- | --- |
| Firmware compartido | Supervisor, gestión de radio, BLE protegido, transporte MQTT anónimo, OLED/botón, contratos v2 y journal persistente. Compilación completa Windows con core oficial. | Arranque, memoria, periféricos, BLE/MQTT y persistencia en la placa real. |
| Escudo — Persona 1 | Descubrimiento pasivo, selección de objetivo, captura, cuatro detectores y recuperación. Configuración de conectividad y objetivo persistentes. | Pruebas reales de captura/coexistencia, reinicio y corte de alimentación; integración final de los tres modos y alertas recientes en OLED. |
| Sanitizador — Persona 2 | Contratos, API y encargos preparados. | Auditoría, pasarela/NAT, DNS y DoH; su flag permanece deshabilitado hasta integrar la implementación real. |
| Cebo — Persona 3 | Contratos, API y encargos preparados. | Servicios HTTP/Telnet, detección, historial y alertas; su flag permanece deshabilitado hasta integrar la implementación real. |
| Aplicación — Persona 4 | Base Android/web v2: conexión BLE/WS, estado, comandos e historial. | Controles específicos y gráficos de los módulos, pantallas completas y notificaciones push. |
| Broker e integraciones | Mosquitto MQTT/TCP y WS anónimos, v1/v2 para LAN; sin TLS/cuentas/roles/ACL. | Pruebas con ESP32/teléfono; webhooks y push en sus encargos. |

Subir el firmware y arrancar la app y Docker permite comenzar las pruebas de la base; no completa los módulos ni las pantallas pendientes. **Escudo funciona por BLE y no utiliza MQTT directo ni se asocia a la red observada.** La [pantalla Escudo](docs/modules/app-escudo.md) ya incluye scan, catálogo, objetivo, captura, gráficos, incidentes e historial en el APK normal; su aceptación física sigue pendiente. El panel temporal Diagnóstico P1 fue retirado; las comprobaciones negativas de protocolo permanecen en pruebas de software. Las demás pantallas conservan sus encargos. Sin OLED puede usarse la salida serie temporal (`SENTINEL_SERIAL_TEST_DISPLAY=1`) con el mismo botón y pairing protegido, según la guía P1. La salida serie está desactivada por defecto.

La [verificación integrada](openspec/changes/sentinel-mvp-sin-cuentas/verification.md) y el [informe del build nativo](openspec/changes/sentinel-mvp-standard-persistence/native-build-verification.md) registran compilación Windows PASS, pruebas host con ASan/UBSan y nueve tareas de software completadas. Esto no acredita pruebas físicas ni cierra el MVP completo. Las evidencias y pendientes históricos se conservan en sus respectivos cierres OpenSpec.

## Preparar y probar el firmware

El flujo diario utiliza **Arduino IDE Windows**, core oficial **ESP32 3.3.12**, C/C++ y FreeRTOS. Biblioteca externa: **wolfMQTT 2.2.0**; TCP usa NetworkClient del core oficial, sin ESP_SSLClient. No requiere reconstruir un SDK, PlatformIO ni ejecutar generadores del proyecto antes de cada compilación.

1. Seguir la [instalación inicial](docs/firmware.md#instalación-inicial): instalar el core oficial, cerrar el IDE y ejecutar una vez por equipo:

   ```powershell
   .\firmware\arduino\install-wolfmqtt.ps1
   .\firmware\arduino\install-board-config.ps1
   ```

2. Abrir [SentinelNode.ino](firmware/SentinelNode/SentinelNode.ino). Seleccionar **ESP32S3 Dev Module**, flash **16MB**, **QIO 80MHz**, **OPI PSRAM** y **Sentinel standard USB (4MiB factory, no OTA)**.
3. Seguir el [cableado y las pruebas de Persona 1](docs/guia-pruebas-persona-1.md). El botón físico sigue siendo necesario; la guía explica OLED o monitor serie temporal para mostrar el código BLE.
4. Para la **primera carga con la tabla nueva**, seleccionar **Erase All Flash Before Sketch Upload: Enabled**, identificar el COM real y subir. Se pierden los datos anteriores: emparejar y configurar de nuevo. Después volver a Disabled si no se solicita borrado completo; ninguna actualización USB garantiza conservar datos.

Identidad, vínculos BLE, configuración aceptada, objetivo del Escudo e historial confirmado deben conservarse durante el uso normal, reinicios y cortes. Se permite reinicializarlos al reinstalar/actualizar por USB, sin migración entre versiones. Ver [política de persistencia y protecciones restantes](docs/persistence-policy.md). No se almacenan secretos en Git, logs, eventos o estado público ni se promete cifrado físico de flash.

## Preparar la app, contratos y broker

Node **24.13.1**, npm **11.8.0**, JDK21 y Android SDK36 para la app. Arduino IDE es el flujo de firmware; Node y WSL no son requisitos para compilarlo o subirlo. Node también ofrece mantenimiento opcional del firmware mediante `npm run test:firmware` y `npm run test:escudo`. Para preparar app y contratos, desde la raíz:

```sh
nvm use
npm ci
npm run check:contracts
npm run test:app
npm run build
npm run dev
```

Workspaces npm privados: `app/` y `contracts/`, un solo `package-lock.json`. `firmware/` y `broker/` son independientes; `simulator/` conserva el límite de integración futuro, sin bloquear hardware.

`npm run dev` abre la app web. El flujo BLE de esta implementación se prueba con la app **Android**; consultar [compilación del APK, permisos y acceso LAN](app/README.md). Para Mosquitto, seguir [preparación y ejecución del broker](broker/README.md): arrancar Compose directamente, sin cuentas. Un teléfono o ESP32 usa la IP LAN del PC sin certificados ni cambios de router; `localhost` en el teléfono no apunta a la computadora. Las pruebas físicas LAN siguen en la integración de Persona 4.

## Documentación y reparto

- [Desarrollo y scripts](docs/development.md)
- [Contratos v1, ejemplos y API](contracts/README.md)
- [Aplicación, Android y acceso LAN](app/README.md)
- [Firmware, cableado, build/upload/monitor](docs/firmware.md)
- [Broker local y acceso LAN anónimo](broker/README.md)
- [Conectividad BLE y MQTT del nodo](docs/connectivity.md)
- [Evidencia de app y contratos](app/evidence.md)
- [Comprobación integral](docs/core-foundation-check.md)
- [Guía de prueba con ESP32 y Android](docs/core-foundation-test-guide.md)
- [Reparto para cuatro integrantes: 23 cambios y encargos individuales](docs/encargos/README.md)
- [Comparación con el PDF: alcance, adaptaciones y alertas recientes en OLED](docs/pdf-scope-comparison.md)
- [Brief](docs/brief.md), [arquitectura](docs/architecture.md), [decisiones](docs/technical-decisions.md), [roadmap](docs/roadmap.md) y [contribución](CONTRIBUTING.md)

Integrar cambios mediante PR hacia la rama de integración acordada (`develop` en esta entrega), con revisión de otro integrante. `main` conserva la versión estable. Versionar OpenSpec junto con el código y mantener pendientes las comprobaciones que no se hayan realizado; ver [CONTRIBUTING.md](CONTRIBUTING.md).
