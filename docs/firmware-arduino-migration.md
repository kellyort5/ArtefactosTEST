# Migración y simplificación del firmware para Arduino IDE Windows

> Registro histórico de su entrega. Para instalación y alcance actual consultar [acuerdos vigentes](mvp-policy.md). TLS, cuentas/ACL o dependencias anteriores aquí descritas no se reinstalan para el MVP actual; se conservan las evidencias y pendientes originales.

**MQTT vigente (2026-11-17):** TCP1883/WS9001 anónimos en LAN de confianza, sin certificados ni ESP_SSLClient. [Cambio y verificación](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md). Los builds anteriores con TLS son históricos.


**Política actual:** [persistencia operativa y carga USB](persistence-policy.md). Sustituye la conservación entre versiones y las recetas especiales anteriores. El [build final de tabla nueva/cuatro claves/configuración persistente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md) terminó PASS en Windows; los hashes citados como antecedentes abajo pertenecen al build anterior.

Fecha de preparación: **2026-11-13**. Responsable humano: **Persona 1 — Jefry**. Rama compartida: `feat/sentinel-arduino-migration`, baseline `af7200a` más trabajo local preservado.

**Destino confirmado:** Arduino IDE Windows con el core oficial ESP32 **3.3.12**, C/C++ y FreeRTOS, reutilizando APIs ESP-IDF y dependencias mantenidas. La nueva instrucción sustituye la fabricación/distribución de un SDK propio como objetivo. Node es mantenimiento opcional: no se exige para compilar, subir o monitorizar firmware.

**Evidencia histórica anterior a la tabla nueva:** build completo Windows **PASS exit0** con core oficial3.3.12, wolfMQTT2.2.0 y ESP_SSLClient3.1.3. Imagen1376726bytes, globalRAM160228bytes;428entradas inmutables y730depfiles sin SDK Sentinel. ELF SHA256 `60d6e73cc23ac17520f0652d9995ea8dc20e63bd89ab2d4bd9c4f3dc5c2eb6ff`. Ver [evidencia final](../openspec/changes/sentinel-arduino-dependencies/verification.md). El build del SDK reconstruido se conserva únicamente como antecedente. Upload/monitor, memoria física, OLED/botón, BLE, radio e historial NVS siguen pendientes. Solo se identificaron COM3/COM4 Bluetooth y ningún teléfono en ADB; no se inventó puerto ni prueba física.

## 1. Flujo del equipo y relevo mínimo

Abrir `firmware/SentinelNode/SentinelNode.ino` directamente desde el checkout completo. El recorrido previsto es seleccionar placa/opciones, **Verificar**, **Subir** y **Monitor serie** a 115200 en Arduino IDE Windows. [Firmware](firmware.md#preparar-arduino-ide) concentra la instalación/configuración inicial; [Persona 1](guia-pruebas-persona-1.md) conserva cableado y criterios de aceptación del Escudo/app v2.

| Elemento de la ruta oficial | Estado y evidencia |
| --- | --- |
| Bibliotecas y versiones definitivas | wolfMQTT2.2.0 con versión/hash fijados y NetworkClient del core oficial para TCP; sin ESP_SSLClient. Procedimiento en firmware.md. |
| Placa/opciones N16R8 y seguridad | Opciones N16R8/guardas comprobadas por el build previo oficial Windows PASS; nueva tabla y configuración persistente requieren build integrado nuevo. |
| NVS y subida | Wrapper NVS de protección de arranque normal, cuatro claves y recetas oficiales. Primera carga limpia; build integrado y persistencia física pendientes de evidencia nueva. |
| Instrucciones de uso | Pasos IDE/CLI publicados; build previo oficial PASS como antecedente. Build integrado nuevo y hardware pendientes de evidencia. |

wolfMQTT **2.2.0** y ESP_SSLClient **3.1.3** ya compilaron y enlazaron en una prueba aislada con el core oficial, según evidencia comunicada por el coordinador. Los adaptadores y la instalación previa están integrados; aquel firmware completo tiene build Windows PASS. No acredita tabla/configuración de la política posterior. Ni la prueba aislada ni ese build acreditan un enlace BLE físico.

**Decisión vigente:** core oficial, tres teléfonos, cuatro claves mínimas y carga/exportación estándar. El wrapper evita autoerase antes de setup en arranques normales; no garantiza compatibilidad entre versiones. Primera carga limpia desde IDE; después volver a Disabled si no se solicita borrado total. En reinicios sin upload, incompatibilidad NVS/bonds se informa sin borrar. USB puede reinicializar datos y requerir nuevo pairing/configuración, sin migración.

La preparación inicial se automatiza con `firmware/arduino/install-wolfmqtt.ps1` e `install-board-config.ps1`; ver [los dos comandos de instalación](firmware.md#instalación-inicial). Comprueban versión/hashes y estado, respaldan reemplazos y preservan las claves ajenas. Las recetas y los tres archivos personalizados siguen versionados; no se distribuye un ZIP modificado ni código del proveedor en Git.

Aplicación, broker, notificaciones, contratos y simulador conservan sus herramientas. No instalar frameworks adicionales ni modificar instalaciones globales ajenas. Las incompatibilidades se corrigen por su autor y se elevan si cambian arquitectura/contrato; no se eliminan garantías para compilar.

## 2. Alcance que debe conservarse

Las fuentes de verdad funcionales siguen siendo [arquitectura](architecture.md), [decisiones técnicas](technical-decisions.md), [roadmap](roadmap.md), [API de módulos](../firmware/SentinelNode/src/sentinel_api/include/module_api.hpp) y [protocolo v2](../contracts/module-protocol.md).

| Área | Funcionalidad que la migración conserva |
| --- | --- |
| Base | Un supervisor y un gestor de radio; Idle/Escudo/Sanitizador/Cebo excluyentes; parada confirmada, colas y recursos acotados, errores, reconexiones y control por el canal autorizado. |
| Escudo | Descubrimiento pasivo de 2.4 GHz, SSID/BSSID/canal/RSSI/seguridad, selección y cambio de objetivo; captura Beacon/Deauth/Disassoc y observaciones para Karma; prioridad del objetivo, recorrido de canales, cobertura y los cuatro detectores. Control, eventos, métricas y alertas por BLE; registro offline y recuperación. Sin asociación a la red auditada ni MQTT directo. |
| Sanitizador | Auditoría DNS/HTTPS/portal cautivo; STA+AP con DHCP, NAT IPv4 y subredes separadas; clientes, RSSI y tráfico de ambas direcciones; DNS local con reglas/excepciones y DoH con validación TLS, degradación y recuperación. Sin fallback automático a DNS plano. |
| Cebo | HTTP administrativo ficticio y Telnet simulado en puerto 23; conexiones/accesos/intentos de autenticación, metadatos, cantidad/frecuencia, clasificación y patrones repetitivos. Sin ejecución de comandos ni registro de credenciales, tokens o cuerpos completos. Límites de conexiones y uso exclusivo en LAN autorizada. |
| Eventos | Contratos y validadores existentes, IDs estables, boot, revisión, hora/calidad, origen real/simulado, persistencia circular, paginación, deduplicación y gap. Mantener la publicación MQTT que consumen historial/alertas/webhook/push. |
| Periféricos | SSD1306, botón corto/largo, código de pairing, estados y resumen de alertas recientes requerido en la integración final. |

El firmware base y Escudo tienen implementación en este checkout; Sanitizador/Cebo aún no están habilitados en los defaults. **La migración no convierte esos módulos futuros en mocks ni los da por terminados.** Sus encargos mantienen el alcance y los responsables originales. La pantalla específica de app y la infraestructura webhook/push permanecen en sus componentes externos; no se trasladan a la ESP32.

Invariantes de compatibilidad:

- Misma API pública de módulos y mismos hooks `init/start/request_stop/stopped/handle/tick/snapshot`.
- Mismos UUID GATT, framing, límite lógico de 2048 bytes y seguridad de lectura/escritura/indicaciones.
- Mismos contratos v2, operaciones, resultados, códigos y confirmaciones; eventos históricos v1 intactos. La versión de mensajes Sentinel y la versión del protocolo MQTT son conceptos distintos.
- MQTT 5/TCP sin TLS, identidad/ACL, QoS y retained conforme al contrato; PUBACK no equivale a comando aplicado ni a consumo por la app.
- Configuración Wi-Fi/MQTT por BLE autorizado persistente en NVS oficial: validación/commit antes de confirmar, restauración en Idle sin conexión ni reloj UTC antiguo. No se promete cifrado físico; no guardar secretos en Git/logs/eventos/estado ni credenciales de terceros.
- No modificar `app/`, `broker/`, `notifications/`, `simulator/` ni `contracts/` para ocultar una incompatibilidad del port. Una incompatibilidad real se corrige en firmware o se presenta para revisar la interfaz antes de cambiarla.

## 3. Evidencia preservada y alcance de cada resultado

| Etapa | Evidencia y límite |
| --- | --- |
| Base PlatformIO/ESP-IDF | [Fundación](core-foundation-check.md) y `firmware/EVIDENCE.md`; antecedentes, sin validar Arduino oficial. |
| SDK propio anterior | [SDK](../openspec/changes/sentinel-arduino-sdk/verification.md), [port](../openspec/changes/sentinel-arduino-firmware-port/verification.md), [build del coordinador](../openspec/changes/sentinel-arduino-firmware-port/coordinator-verification.md) e [integración/límites](../openspec/changes/sentinel-arduino-firmware-port/integration-verification.md). Build Windows completo PASS y apertura del sketch; Verificar visual y hardware pendientes. |
| Legibilidad actual | [Verificación](../openspec/changes/sentinel-firmware-readability/verification.md): supervisor de 23 líneas de coordinación, almacenamiento/tiempo extraídos, API y comportamiento conservados; suites host ASan/UBSan PASS. La unidad runtime se compiló con SDK previo, sin acreditar enlace oficial. |
| Limpieza actual | [Verificación](../openspec/changes/sentinel-firmware-cleanup/verification.md): runners Node24, validador local distribuido, recetas/scripts SDK anteriores retirados y un solo árbol Arduino. Suites y casos C++ conservados;17 casos opcionales de transporte reproducibles. |
| Integración oficial | [Verificación final de dependencias](../openspec/changes/sentinel-arduino-dependencies/verification.md): build completo oficial Windows PASS,17 casos focalizados de transporte y datos/recetas conservados por software; hardware pendiente. |

El port previo partió de `af7200ab1047f2d0dfc09b64047dfec6a3ce3309`, que integra Escudo/recuperación `dd0840c`. Su comparación identificó **36 archivos SHA256 idénticos** en aquel movimiento; los refactors posteriores se verifican por su propia evidencia, no se anuncian ahora como idénticos a esa referencia.

Para esta simplificación, el trabajo previo sin commit se conserva fuera del repo en `/tmp/sentinel-simplification-before.zip` y el mapa `/tmp/sentinel-simplification-before.json`. Baseline inmediato: `/tmp/sentinel-simplification-baseline-core.log` y `/tmp/sentinel-simplification-baseline-escudo.log`, ASan/UBSan PASS, **69 fixtures v2**, cuatro detectores y recuperación de **6 únicos/6 repetidos**. No modificar esos archivos protegidos. Son evidencia local de esta sesión, no requisitos diarios del equipo.

### Referencias del SDK propio anterior

Se preservan para identificar lo probado; no son instrucciones de instalación vigente:

| Componente | Referencia histórica |
| --- | --- |
| Arduino-ESP32 / ESP-IDF | 3.3.12 `94afccf35fb1e401facddbcf9e13bcf7c76a31d8` / 5.5.5 `b774170ff46c393eeb5e495ea37936038d3f4f4f`. |
| Builder / paquete / FQBN | `6671d0bd65cdb9d4cc1001b759e8610de945a8d5` / `3.3.12-sentinel.1` / `sentinel:esp32:sentinel_s3_n16r8`. |
| IDE / CLI | IDE 2.3.10 / CLI 1.5.1 `01f3d4f2b`. |
| Toolchain | GCC14.2.0 `esp-14.2.0_20260121`, Python Windows3.12.10, esptool5.3.1. |
| ZIP histórico | 576.518.681 bytes, SHA256 `6c3bdd904f62b01d7c64342c47d81f38aad22c018cc6d671578e46c82e2d24b0`, en `%LOCALAPPDATA%/Temp/sentinel-arduino-sdk/release/`. |
| Inventario distribuido | 8.174 archivos, manifiesto SHA256 `bfeb51cbbd2d287ed51cfc377eb526b25e1dd1931309cf4736a2d3924dfa1b8a`. |

Ese SDK enlazó ESP-MQTT5, mbedTLS con fechas y NimBLE SC-only/nivel4; su parche retenía el resultado de inicialización NVS sin formatear. El hook generaba validadores en el build y su receta omitía `boot_app0.bin` en `0xe000` por solapamiento con NVS. Esas soluciones pertenecen al antecedente; la integración oficial conserva las garantías de arranque normal sin fabricar aquel SDK; la continuidad USB queda sustituida por la política nueva. No borrar su instalación separada ni otras bibliotecas del equipo.

<a id="7-árbol-único-y-mapa-del-movimiento-realizado"></a>

## 4. Árbol único y responsabilidades

| Origen histórico (baseline) | Destino vigente |
| --- | --- |
| `firmware/src/main.cpp` | [SentinelNode.ino](../firmware/SentinelNode/SentinelNode.ino), setup/loop; Arduino aporta el único app_main. |
| `firmware/components/sentinel_api/` | [src/sentinel_api](../firmware/SentinelNode/src/sentinel_api/), API/hooks sin cambios. |
| `firmware/components/sentinel/` | [src/sentinel](../firmware/SentinelNode/src/sentinel/), núcleo, radio, BLE, MQTT, journal y periféricos. |
| `firmware/components/escudo/` | [src/escudo](../firmware/SentinelNode/src/escudo/), descubrimiento, captura, scheduler, parser y cuatro detectores. |
| Pines/módulos de Kconfig | [SentinelConfig.h](../firmware/SentinelNode/SentinelConfig.h); core oficial y bibliotecas fijadas. |
| `firmware/partitions.csv` | [SentinelNode/partitions.csv](../firmware/SentinelNode/partitions.csv), ahora con distribución compatible con recetas oficiales; el movimiento previo conservó bytes como evidencia histórica. |
| Módulos futuros P2/P3 | `firmware/SentinelNode/src/sanitizador/` y `src/cebo/`; todavía sin servicios reales. |

Arduino descubre recursivamente los `.cpp` existentes bajo `src/`. No se copian a `libraries/`, no se incluyen `.cpp` desde otros y no se generan nuevas unidades tarde en un hook. Las recetas CMake/Kconfig/PlatformIO ya se retiraron; sus antecedentes se consultan en Git. La versión anterior se consulta en Git sin mantener dos implementaciones editables.

P2/P3 usan la misma API `init/start/request_stop/stopped/handle/tick/snapshot`, factories `sentinel_sanitizador_module()` / `sentinel_cebo_module()` y flags de `SentinelConfig.h`. El núcleo conserva supervisor, leases, transportes y journal únicos. Los defaults habilitan Escudo y deshabilitan Sanitizador/Cebo; habilitarlos sin factory falla al enlazar. Ver [integración de módulos](development.md#api-de-módulos-de-firmware-persona-1-2-y-3) y [README del sketch](../firmware/SentinelNode/README.md).

El [mapa de lectura del núcleo](development.md#mapa-de-lectura-del-núcleo) explica `runtime.cpp`, `runtime_storage.h`, `runtime_time.h` y sus colaboradores. Describe archivos existentes; no impone nuevas clases o archivos como requisitos funcionales.

## 5. Validadores y mantenimiento opcional

Arduino consume el include local `src/sentinel/v2_validator.h`, de **193245 bytes** y SHA256 `9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68`, idéntico al generado anterior según la evidencia de limpieza. No se genera en un hook del IDE ni requiere Python/Node para compilar. Se conserva el contrato único de `contracts/schemas/v2`.

Mantenimiento desde Linux/WSL con Node24, C/C++ y OpenSSL:

```sh
SENTINEL_SANITIZE=1 npm run test:firmware
SENTINEL_SANITIZE=1 npm run test:escudo
node firmware/tools/generate_validator.mjs --check
node firmware/test/validator_test.mjs
```

Regenerar con `node firmware/tools/generate_validator.mjs` solo al cambiar los schemas y revisar el diff. Los runners host ya compilan el C++ y contrastan sus salidas con el codec real; no sustituyen placa/Android. `arduino_build_test.mjs` y el checker de configuración conservan comprobaciones del estado actual y esperan adaptación final al oficial; no prueban por sí solos un enlace completo.

## 6. Datos y seguridad que deben mantenerse

Tabla fuente en [partitions.csv](../firmware/SentinelNode/partitions.csv):

```csv
# Name, Type, SubType, Offset, Size, Flags
otadata, data, ota, 0xe000, 0x2000,
factory, app, factory, 0x10000, 0x400000,
nvs, data, nvs, 0x410000, 0x6000,
event_log, data, nvs, 0x416000, 0x40000,
```

Mantener journal de **64 registros de hasta 2048 bytes**, namespaces/slots, cursores, IDs y bytes históricos v1/v2. La imagen factory de 4 MiB no representa los 16 MiB físicos. El arranque debe informar errores de almacenamiento sin borrar/formatear NVS. La región `otadata` permite `boot_app0` estándar; no añade OTA funcional. No hay PHY separada (initdata en app, calibración en NVS). Primera carga de esta tabla con Erase All Flash Enabled desde el IDE; USB normal y merged de inicialización sin garantía de preservar datos. Las pruebas de reinicio/corte se hacen después sin upload ni borrado.

Conservar MQTT5/TCP sin TLS ni CA, BLE autenticado SC/nivel4 con bonds, mismos UUID/framing, autorización y colas acotadas. Forwarding/NAPT se concede solo al Sanitizador; su compilación no acredita la pasarela real. Escudo continúa sin asociación, probes activos o MQTT. OLED SSD1306 128×32/0x3C y GPIO4 mantienen estado, pulsación larga y prioridad del passkey; el resumen de alerta reciente conserva su tarea de integración pendiente.

## 7. Criterios de aceptación conservados

El build oficial completo debe identificar versiones/opciones/bibliotecas efectivas y mantener todas las pruebas críticas sin debilitar aserciones. No modificar contratos o módulos para obtener un verde. El coordinador contrasta evidencia antes de marcar tareas; esta documentación no cambia casillas ni archiva cambios.

### Hardware e integración breve

| Caso | Evidencia mínima |
| --- | --- |
| Arranque/periféricos | Flash física 16 MiB, PSRAM 8 MiB, device persistente y nuevo boot, Idle seguro, OLED/botón, ventana y passkey. |
| BLE | Metadata v2, UUID/framing originales, permisos protegidos, pairing SC/MITM y rechazo de enlaces no autorizados. Confirmación de estado/modo desde la app ya existente. |
| Escudo | Scan pasivo y selección real, captura/métricas/cobertura, control de canales y start/stop; sin asociación/MQTT. Heurísticas positivas/negativas en host y observaciones reales según la guía existente. |
| Offline | Evento persistido y recuperado tras desconexión/reinicio, mismos bytes/ID/tiempo, cursor/dedup/gap y capacidad de seguir operando. No borrar para hacer pasar la prueba. |
| MQTT/TCP | Conexión MQTT5/TCP al broker autorizado, QoS/retained y propiedades según contrato; rechazar credenciales y permisos incorrectos, conectar incluso sin UTC. Repetición conserva identidad y resultado de comando. |
| Sanitizador, al estar entregado | Cliente real STA+AP/DHCP/NAT, subredes, métricas, reglas DNS, DoH TLS y fallo/recuperación sin DNS plano; parada sin servicios residuales. |
| Cebo, al estar entregado | HTTP/Telnet ficticios, interacciones/eventos reales sin secretos ni ejecución, MQTT y recuperación offline; cierre al salir de modo. |
| Integración final | Un modo activo, cambios seguros, recursos liberados, OLED con alertas recientes y control físico; los demás componentes del sistema siguen sus specs propias. |

Wi-Fi y BLE continúan compartiendo radio: cambiar de IDE no elimina H-03. Registrar coexistencia o pausas reales sin retirar funciones; Espressif documenta [rendimiento inestable del sniffer concurrente con BLE](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/api-guides/coexist.html).

Si falta placa, teléfono, UI específica o un módulo, continuar con software integrable y registrar el pendiente. Un fixture, un estado `ready`, un paquete instalado o una casilla de archivo no acreditan esas comprobaciones. Las tareas físicas heredadas de las specs archivadas permanecen abiertas hasta tener evidencia y aceptación pertinente.

## 8. Coordinación y siguiente sesión

P1 mantiene sketch, núcleo/Escudo y API comunes; P2 implementa Sanitizador, P3 Cebo/notificaciones y P4 pantallas/broker/laboratorio según sus encargos. Los cuatro trabajos de simplificación actuales conservan propiedad por archivo y no reasignan módulos.

La instalación/configuración están documentadas; el build oficial anterior tiene evidencia histórica y el nuevo integrado tiene [build Windows PASS](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); la limpieza conserva un solo árbol de firmware y mantenimiento Node opcional. Con placa/teléfono identificados, se recorren Verificar/Subir/Monitor y T01/T02/T03/T14/T15 de Persona1; las pruebas específicas de Escudo esperan UI o cliente v2 revisado. Los servicios P2/P3 y la integración final conservan sus criterios completos.

No hacer commits, push, merge, archivo ni borrar ramas sin autorización. Conservar el trabajo previo; se permite reinicializar datos al programar por USB según el procedimiento limpio documentado. En reinicios ordinarios no borrar automáticamente; la falta de prueba física se documenta sin inventar detecciones o puertos.
