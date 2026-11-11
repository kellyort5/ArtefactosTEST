# Proposal

## Why

Sentinel Node tiene arquitectura y alcance funcional definidos, pero el repositorio aún no contiene firmware, broker ni aplicación ejecutables. Se necesita una base compartida real y sencilla para desarrollar en paralelo Escudo Pasivo y Cebo sin duplicar contratos, conectividad, control de modos o periféricos.

## What Changes

- Crear un monorepo con firmware, broker, app, contratos y simulador separados, conservando documentación, reglas Git y artefactos OpenSpec existentes.
- Inicializar ESP32-S3-DevKitC-1 N16R8 con PlatformIO, ESP-IDF, C/C++ y FreeRTOS; configurar flash/PSRAM y versiones reproducibles.
- Implementar supervisor central, perfiles de recursos Escudo/Cebo, transiciones seguras y extensión para futuros módulos. El perfil de base no anuncia detección ni honeypot como disponibles.
- Preparar BLE autenticado para control/configuración y Wi-Fi STA/MQTT para el perfil Cebo; impedir asociación Wi-Fi y MQTT directo en el perfil Escudo.
- Ejecutar Mosquitto mediante Docker Compose con MQTT/TLS, WSS, identidades separadas, ACL y material privado local no versionado.
- Definir contratos v1 de estado, métricas, eventos, comandos y confirmaciones, con validación, identificación, vigencia y deduplicación.
- Incorporar un registro persistente circular mínimo de eventos del sistema, compartido por futuros módulos, con recuperación por cursor.
- Crear app React/Vite/Tailwind/Zustand con Capacitor Android, adaptadores BLE/MQTT reales, configuración de conectividad y vista de diagnóstico/control de la base.
- Integrar OLED SSD1306 y botón con el supervisor, y documentar configuración, compilación, ejecución y comprobaciones básicas.

**Objetivo de cierre de este cambio:** compilar firmware y app, levantar el broker protegido y demostrar estado/comandos/periféricos con un ESP32 y teléfono reales. No equivale a terminar el MVP completo de R-11.

**Exclusiones:** descubrimiento/captura/detectores del Escudo; HTTP/Telnet y clasificación Cebo; integración webhook/Telegram/push; NAT, DNS/DoH, infraestructura histórica central, OTA, CI compleja, benchmarks y campañas prolongadas. Las funcionalidades completas de módulos 1 y 3 permanecen obligatorias en R-11 y se implementarán en cambios independientes; este cambio prepara sus puntos de integración y no las sustituye con mocks.

## Capabilities

### New Capabilities

- `project-workspace`: organización separada, configuración reproducible e instrucciones de compilación/ejecución.
- `device-runtime`: arranque N16R8, supervisor, exclusión de perfiles, gestión de recursos y controles físicos.
- `message-contracts`: mensajes versionados, validación, identidad, confirmación y vigencia de comandos independientes del transporte.
- `device-connectivity`: control BLE autenticado, configuración local y conectividad STA/MQTT condicionada al perfil.
- `secure-mqtt-broker`: broker Docker con listeners cifrados, identidades, permisos y configuración privada local.
- `event-journal`: registro persistente acotado, recuperación y deduplicación de eventos del sistema.
- `mobile-control`: app Android con canales BLE/MQTT, estado real y control diagnóstico del dispositivo.

### Modified Capabilities

Ninguna: el inventario actual de OpenSpec está vacío.

## Impact

- Nuevos archivos de implementación futuros en `firmware/`, `broker/`, `app/`, `contracts/` y estructura mínima `simulator/`; documentación de ejecución en `docs/` y enlaces desde README. No introducir servicios backend adicionales.
- Dependencias: toolchain PlatformIO/ESP-IDF, componentes oficiales de radio/MQTT/NVS/LCD, Node/npm, stack frontend aprobado, plugin BLE de referencia y herramientas Android/Docker. Versiones concretas se fijan según el diseño y lockfiles; no añadir frameworks de monorepo.
- Contratos v1 y GATT son interfaces nuevas propuestas para revisión; los acuerdos de R-03/R-04/R-05/R-06/R-07/R-10/R-11 se conservan. Configuración de pines, certificados y credenciales de laboratorio se comprueba al integrar, sin secretos en Git.
- Riesgos: configuración N16R8 distinta del perfil genérico de placa; contención RF; pairing Android; confianza TLS local; cambio de canal durante transiciones y disponibilidad de hardware. Resolver fallos funcionales puntuales según H-01/H-02/H-07/H-08; H-03 queda preparado y H-04/H-05/H-06 se atienden en sus módulos.
- El brief/configuración inicial priorizan simulación; la instrucción posterior y documentación vigente priorizan hardware real. El simulador se conserva separado como apoyo, sin convertirlo en prerrequisito.
