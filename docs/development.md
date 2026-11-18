# Desarrollo de la base compartida

**MQTT vigente (2026-11-17):** TCP1883/WS9001 autenticados en LAN de confianza, sin certificados ni ESP_SSLClient. [Cambio y verificación](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md). Los builds anteriores con TLS son históricos.


**Política aprobada vigente:** [persistencia operativa y USB](persistence-policy.md), definida en [sentinel-mvp-standard-persistence](../openspec/changes/sentinel-mvp-standard-persistence/design.md). Conservar identidad, bonds, configuración e historial confirmado en uso normal/reinicio/corte; USB permite reinicialización sin migración. Arduino oficial primero; justificar cada excepción mínima. La tabla nueva/cuatro claves/configuración persistente se verifican en la [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); los builds anteriores son históricos y no cierran pruebas físicas.

Trabajar desde la raíz del repositorio en la rama correspondiente a cada cambio; la base se publicó desde `feat/sentinel-core-foundation`. Leer [brief](brief.md), [CONTRIBUTING](../CONTRIBUTING.md) y el [diseño de la base archivada](../openspec/changes/archive/2026-11-10-sentinel-core-foundation/design.md). No se necesitan servicios de monorepo ni un backend adicional.

## Entorno y dependencias

- Node24 LTS: `.nvmrc` fija **24.13.1**; npm **11.8.0**.
- App: React19, Vite7, TypeScript5, Tailwind4/plugin Vite, Zustand5, mqtt.js5, Capacitor8 y BLE comunitario8. Versiones exactas en `app/package.json` y lockfile raíz. AJV8 y ajv-formats3 en `contracts/`.
- Android: JDK21, SDK Platform36 y Build Tools36; Gradle8.14.3 y AGP8.13.0 provistos por Capacitor8.6.0. Proyecto nativo y wrapper versionados; APK/caches/assets web regenerados ignorados.
- Firmware: Arduino IDE Windows, core oficial ESP32 3.3.12, APIs ESP-IDF y FreeRTOS. El firmware completo tiene build Windows PASS con wolfMQTT2.2.0 y ESP_SSLClient3.1.3; [firmware](firmware.md) documenta instalación inicial/opciones y subida física pendiente. El [build del SDK propio](../openspec/changes/sentinel-arduino-firmware-port/integration-verification.md) es evidencia histórica. Node no es requisito para compilar/subir desde el IDE.
- Broker: Docker Desktop/Engine y Compose, Python3 para cuentas locales; [preparación](../broker/README.md).

Entorno WSL de esta implementación (todas las órdenes desde la raíz):

```sh
export PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH
export ANDROID_HOME=/home/jefry/Android/Sdk
export ANDROID_SDK_ROOT="$ANDROID_HOME"
node --version
npm --version
java -version
npm ci
```

`nvm use` es alternativa a fijar PATH si nvm está inicializado. No guardar rutas privadas de SDK en archivos versionados; usar entorno o `app/android/local.properties` ignorado. PowerShell puede usar su Node24/JDK21/SDK nativos y `app/android/gradlew.bat`.

## Comandos de app y contratos

| Comando raíz | Resultado |
| --- | --- |
| `npm ci` | Instalación reproducible de ambos workspaces desde lockfile único. |
| `npm run dev` | Vite; localhost5173, puerto alterno si ocupado. Por defecto escucha0.0.0.0; `npm run dev -w app -- --host 127.0.0.1` limita a equipo local. |
| `npm run build` | Compila contratos y después web a `app/dist/`. |
| `npm run build -w app` | Compila la app después de construir contratos con `npm run build -w contracts`. |
| `npm run check:contracts` | Build TypeScript, fixtures válidos/inválidos y framing/codec. |
| `npm run test:app` | Casos de reconciliación y registro usando IndexedDB de prueba en Node. |
| `npm run android:sync` | Build de contratos/web y sincronización nativa Android. |
| `npm run android:assemble` | `assembleDebug` usando wrapper del proyecto Android; sin dispositivo. |
| `npm run check:browser -w app` | Chromium real: servidor Vite activo, UI, IndexedDB256/cursor/gap tras recarga. |
| `npm run check:mqtt -w app` | Adaptador mqtt.js real contra broker local preparado; cuentas solo desde archivo privado. |

Para la comprobación Chromium, `npm exec -w app -- playwright install chromium` instala su navegador si falta; no es un emulador Android. Los fixtures utilizados en tests no acreditan funcionalidades del ESP32.

## Firmware, broker y simulador

Abrir `firmware/SentinelNode/SentinelNode.ino` en Arduino IDE Windows. Usar **Verificar**, **Subir** y **Monitor serie** a 115200 con el core oficial. [Firmware](firmware.md#preparar-arduino-ide) concentra la instalación de las dos bibliotecas mantenidas, la opción local de protección de datos y los valores N16R8; no reutilizar el FQBN del SDK propio para acreditar ese flujo.

Upload/monitor requieren placa y COM identificados; los puertos Bluetooth no sustituyen la conexión USB de la ESP32. Sin placa, solo build y pruebas focalizadas de host son evidencia válida. No usar PlatformIO ni generar una segunda copia de las fuentes para desarrollar un módulo.

Preparar y ejecutar broker conforme a [broker/README.md](../broker/README.md). Sus scripts son independientes de npm; usuarios/ACL deben existir antes del arranque. Publica TCP1883/WS9001 en LAN por defecto; usar IP del PC y permitir Docker en el firewall local. Sin certificados ni cambios de router. Un broker accesible desde WSL no demuestra acceso desde teléfono/ESP32.

[simulator/README.md](../simulator/README.md) delimita un futuro apoyo ligero. No hay simulador de comportamiento de producto en esta base; las pruebas pequeñas de contratos son auxiliares y no sustituyen controles/periféricos reales.

## Seguimiento y comprobación

[App](../app/README.md) describe instalación, acceso LAN y recorrido funcional BLE/WS. El usuario autorizó continuar con el teléfono, la ESP32 y el emulador del SDK Windows. Seguir la [guía de prueba](core-foundation-test-guide.md) y registrar las comprobaciones ejecutadas en la [evidencia integral](core-foundation-check.md). Las tareas de teléfono y radio requieren su recorrido físico completo para cerrar las casillas correspondientes.

No versionar node_modules, dist, .gradle, build, APK, configuración local privada ni credenciales. Mantener native Android, wrapper JAR, configuración fuente, esquemas, fixtures y lockfile. Las actualizaciones de casillas deben releer/contrastar la línea original bajo lock exclusivo `/tmp/sentinel-core-tasks.lock`; solo completar comportamiento y verificación realmente cumplidos.

## API de módulos de firmware (Persona 1, 2 y 3)

El cambio `sentinel-firmware-module-api` publica [module_api.hpp](../firmware/SentinelNode/src/sentinel_api/include/module_api.hpp). Persona 2 implementa Sanitizador y Persona 3 Cebo como módulos C/C++ independientes dentro del sketch; dependen de **`sentinel_api`**, no de `sentinel`. El núcleo registra las implementaciones, conserva un único supervisor y administra Wi-Fi, BLE, identidad, serialización y el journal NVS de 64 eventos. Los detectores, servicios HTTP/Telnet y lógica NAT/DNS/DoH siguen en sus respectivos cambios.

El firmware de esta rama anuncia **metadata v2** y recibe órdenes v2 conforme a [module-protocol.md](../contracts/module-protocol.md). Las órdenes v1 reciben `unsupported_version`; `start_capture` y `start_honeypot` conservan su significado reservado de v1. Los eventos históricos v1 se recuperan sin convertirlos y se publican en `sentinel/v1/.../events`; los nuevos mensajes usan `sentinel/v2/...`. La integración del adaptador v2 de la app pertenece a su encargo; una app que solo envía v1 no puede controlar este firmware.

### Mapa de lectura del núcleo

| Archivo existente | Responsabilidad |
| --- | --- |
| `runtime.cpp` | Componer servicios y encadenar el ciclo del supervisor; delega operaciones concretas. |
| `runtime_storage.h` | Adaptador NVS del journal y persistencia; conserva errores y evita borrados silenciosos. |
| `runtime_time.h` | Convertir UTC ya validado a la referencia temporal; no obtiene hora mediante Wi-Fi en Escudo. |
| `module_runtime.cpp` / `core.cpp` | Registro/ciclo de módulos, exclusión, admisión de comandos y estado. |
| `network.cpp` / `ble.cpp` | Recursos de red compartidos y canal BLE protegido; los módulos no crean otro gestor. |
| `mqtt_transport.cpp` | Adaptar wolfMQTT al worker/outbox acotados y confirmar su cierre; no implementa otro protocolo MQTT. |
| `mqtt_dns.cpp` | Resolver DNS de manera cancelable mediante lwIP, fuera del ciclo del supervisor. |
| `mqtt_transport.cpp` | MQTT5 sobre NetworkClient TCP oficial, worker y colas acotados; sin certificados. |
| `ble_storage.cpp` | Revisar vínculos anteriores mediante NVS de solo lectura; detener BLE ante incompatibilidad sin borrar datos. |
| `arduino_environment.cpp` / `arduino_requirements.cpp` | Conservar el resultado inicial de NVS y verificar la configuración oficial soportada. |
| `wire.cpp` / `protocol_v2.cpp` / `v2_validator.h` | Sobres, validación y contratos existentes; no deciden modos por separado. |
| `src/escudo/` | Descubrimiento, captura, parser, recorrido de canales, detectores y salida del módulo. |

Este mapa describe la simplificación existente; no añade requisitos de clases o archivos a las specs funcionales.

### Crear y registrar un componente

| Integrante | Directorio del sketch | Factory con enlace C | Flag en SentinelConfig.h |
| --- | --- | --- | --- |
| Persona 1 | `firmware/SentinelNode/src/escudo/` | `sentinel_escudo_module()` | `SENTINEL_MODULE_ESCUDO=1` |
| Persona 2 | `firmware/SentinelNode/src/sanitizador/` | `sentinel_sanitizador_module()` | `SENTINEL_MODULE_SANITIZADOR=1` |
| Persona 3 | `firmware/SentinelNode/src/cebo/` | `sentinel_cebo_module()` | `SENTINEL_MODULE_CEBO=1` |

Cada factory devuelve `sentinel::Module*` con vida de todo el arranque; puede devolver `nullptr` si la implementación no está disponible. Arduino descubre los `.cpp` existentes recursivamente bajo `src/`; sus fuentes incluyen la API pública y headers privados mediante rutas relativas. No añadir dependencias CMake, copiar fuentes a `libraries/` ni incluir `.cpp` desde otros archivos. El núcleo llama la factory al habilitar el flag de aplicación; no hace falta editar `runtime.cpp`, crear otro supervisor ni modificar `SentinelNode.ino`. Activar un flag sin su implementación debe producir error de enlace. Escudo integra descubrimiento, captura, detectores y recuperación; Sanitizador/Cebo permanecen deshabilitados hasta incorporar sus servicios reales.

Ejemplo de punto de entrada para un componente futuro (adaptar el nombre; no es una implementación del módulo):

```cpp
#include "../sentinel_api/include/module_api.hpp"
// MiModulo implementa los siete hooks de sentinel::Module.
extern "C" sentinel::Module *sentinel_sanitizador_module() {
  static MiModulo module;
  return &module;
}
```

La [prueba de consumidor](../firmware/test/module_consumer.cpp) compila incluyendo únicamente la API pública. Es un fixture de host y no se enlaza ni anuncia en el firmware. La revisión del PR por otro integrante, en particular los consumidores P2/P3, sigue siendo necesaria antes de integrar.

### Ciclo de vida y propiedad

1. El núcleo llama `init(ModuleContext&)` una vez, con el dispositivo en Idle. Guardar la referencia al contexto, preparar las estructuras acotadas y devolver `false` si falla. No adquirir radio ni arrancar servicios aquí.
2. Una orden confirmada `set_mode` solicita la parada del módulo anterior mediante `request_stop()`. Este hook retorna inmediatamente. `tick(now_ms)` permite completar la limpieza y `stopped()` solo devuelve `true` cuando hayan terminado callbacks, tareas, sockets y servicios propios.
3. El supervisor espera la parada durante un máximo inicial de **5000 ms**, libera los recursos de red y concede la radio al destino antes de llamar `start()`. Este devuelve pronto; el módulo informa su subestado mediante `snapshot()`.
4. El supervisor llama `handle(ModuleCommand, ModuleReply)` y `tick()` sin bloqueo. El módulo copia los argumentos que necesite **antes de retornar**; no conserva punteros a `operation`, `args` ni `command_id`. Las tareas largas usan una cola propia limitada; no crean trabajo ilimitado ni invocan hooks del supervisor desde otras tareas.
5. Si no termina la parada o falla la liberación, el destino no arranca. OLED/estado muestran `error` y `timeout` o `resource_error`. Resolver la causa y solicitar Idle para reintentar la limpieza; no ocultar el fallo iniciando otro modo.

`ModuleSnapshot.state` contiene el objeto JSON exacto `V2EscudoModule`, `V2SanitizadorModule` o `V2CeboModule`, incluido `kind`. `capabilities` describe servicios realmente implementados, incluso si están detenidos. `target_selected` permite al núcleo rechazar `escudo.capture_start` sin objetivo. Un servicio no implementado declara `available:false,status:"unavailable"`; uno implementado y detenido usa `available:true,status:"stopped"`. No anunciar detecciones, HTTP, Telnet o pasarela solo por registrar el componente.

`ModuleReply.error` es un código estático del contrato, nunca texto recibido del cliente. Para resultados síncronos, escribir el JSON tipado en `result` (incluido `kind`), o dejarlo vacío cuando corresponde `null`. Para `escudo.scan` y `sanitizador.audit`, establecer `job=true`: el núcleo genera `job_id=command_id`, confirma el trabajo encolado y el módulo completa luego con su evento/consulta. El núcleo comprueba el tipo y correlación del resultado, y conserva la respuesta para reintentos. La aceptación de una orden no acredita la finalización de un trabajo asíncrono.

El gate mantiene autorización, cifrado del canal, nodo, boot y caducidad antes de deduplicar; luego comprueba revisión y contexto para órdenes nuevas. MQTT solo controla en Cebo. Las operaciones de Escudo, Sanitizador, configuración de conectividad y reloj requieren BLE autorizado. Las colas llenas devuelven `busy` cuando hay capacidad de respuesta, sin ejecutar dos veces la orden.

### Radio y recursos compartidos

Usar `context.resources()` desde los hooks del supervisor; no llamar `esp_wifi_init/start/stop` en los módulos. Cada contexto recibe una concesión que rechaza peticiones de módulos inactivos. No conservar ni destruir los netifs prestados.

| Modo | Recursos disponibles y restricciones |
| --- | --- |
| Idle | Sin Wi-Fi/MQTT ni servicios de módulo; BLE de control y emparejamiento de la base permanece disponible. |
| Escudo | Driver STA **sin asociación**, scan pasivo, cambio de canal, captura promiscua y BLE. Nunca `esp_wifi_connect` ni MQTT. |
| Cebo | STA con reconexión y MQTT seguro de la base; BLE para configuración/control. Los sockets HTTP/Telnet pertenecen al módulo y se cierran antes de confirmar parada. |
| Sanitizador | `configure_upstream`, `configure_ap`, netifs STA/AP prestados y petición explícita `napt(true/false)`. Sin MQTT directo. El módulo administra auditoría, reglas, DNS/DoH y decide cuándo solicitar la pasarela. |

`ApConfig.ipv4/netmask` usan orden de red. El gestor conserva los netifs durante todo el boot. El destino conserva forwarding IPv4 y NAPT; la configuración efectiva del core oficial se verifica en el cambio de dependencias. Habilitar compilación no activa NAPT. El gestor solo permite NAPT en Sanitizador con AP y dirección de upstream, y lo desactiva antes de liberar la red. Esto prepara la integración de Persona 2; no acredita el funcionamiento de su pasarela.

`RadioEvent::ScanComplete` conserva `scan_status` (0 si ESP-IDF completó el scan) y `scan_count`. `read_scan_results(out,capacity,count)` distingue error de lectura de un catálogo vacío; `scan_results()` se conserva para consumidores previos.

`resources.poll(RadioEvent&)` entrega copias desde una cola de **8** entradas sin bloqueo: final de scan, conexión/desconexión STA, dirección IPv4 obtenida, clientes AP y metadatos/tramas. Los frames conservan hasta **512 bytes**, longitud original, canal, RSSI y tiempo; comprobar truncamiento antes de parsear. `scan_results()` entrega como máximo **32** registros por consulta; las enumeraciones de seguridad son las oficiales del ESP-IDF fijado. El módulo procesa estas copias en `tick()` y mantiene sus propios límites. `resources.dropped()` informa saturación. No interpretar una pausa, pérdida de cola o tramo truncado como evidencia concluyente de ataque.

### Eventos, métricas y persistencia

`context.emit_event(name,severity,details,simulated)` y `publish_metrics(interval_ms,values)` copian JSON de hasta **1536 bytes** antes de retornar. El contrato completo sigue limitado a **2048 bytes**; usar páginas de 1–4 elementos y reducirlas si el sobre no cabe. `details` y `values` contienen exclusivamente las estructuras v2 de [schemas](../contracts/schemas/v2/common.schema.json). El include local `src/sentinel/v2_validator.h` conserva los validadores de esos schemas; el firmware no necesita Node ni Python para compilar ese archivo. El mantenimiento usa `node firmware/tools/generate_validator.mjs` y `node firmware/test/validator_test.mjs` para regenerarlo/contrastarlo; no modificarlo a mano ni crear otro contrato.

Las llamadas no escriben flash ni transportes: validan y encolan en una cola común de **8** entradas. Un retorno `false` indica datos inválidos, contexto inactivo o saturación; no reintentar en un bucle bloqueante. La pérdida se contabiliza y los servicios continúan. Los callbacks de captura solo copian a la cola de radio del gestor.

El supervisor asigna identidad, secuencia, cursor del journal y marca temporal/calidad al procesar la salida. Valida el mensaje completo, persiste los eventos aceptados en el journal existente y luego los entrega por BLE/MQTT según el canal disponible. Las métricas son efímeras. Los eventos reales usan `origin:"real"`; fixtures y simulación usan `"simulated"`, preservado al recuperar/republicar. No incluir contraseñas, tokens, cuerpos ni detalles adicionales: los objetos cerrados del schema los rechazan. No crear almacenamiento paralelo desde los módulos.

El botón corto rota únicamente los modos registrados (Escudo → Sanitizador → Cebo); sin módulos, permanece Idle. Mantener pulsación de 3 s abre los 120 s de emparejamiento y no dispara además la acción corta. OLED usa el perfil/subestado/error del supervisor y mantiene el código de emparejamiento. Las alertas recientes en OLED corresponden al cambio de integración de los tres modos.

### Verificar este cambio

La [evidencia de esta entrega](../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/verification.md) distingue software verificado y prueba física pendiente.

Mantenimiento opcional con Node 24, C/C++ y OpenSSL, desde Linux/WSL:

```sh
SENTINEL_SANITIZE=1 npm run test:firmware
SENTINEL_SANITIZE=1 npm run test:escudo
node firmware/test/arduino_build_test.mjs
```

Los dos primeros comandos ya incluyen compilación C++ y contraste con el codec; no repetir cada `run_host.mjs`. `arduino_build_test.mjs` contrasta factories/flags y guardas de configuración del core oficial y de las bibliotecas; no sustituye el build completo. `check_config.mjs --sdkconfig <ruta-al-sdkconfig.h-efectivo>` inspecciona el header de la variante oficial, memoria configurada, capacidades y particiones. Los defaults del SDK para SC-only/nivel no acreditan el enlace negociado: BLE exige SC-only/nivel4 en runtime. Las suites incluyen parada asíncrona y preservación de vínculos anteriores; los resultados de software no acreditan el hardware.

Los runners de firmware usan Node en lugar de Python. La app/contratos mantienen sus dependencias npm y el firmware no las requiere para **Verificar/Subir** en Arduino.

La regresión opcional del transporte compila el worker activo contra wolfMQTT original, con dobles declarados de socket, DNS y FreeRTOS:

```sh
SENTINEL_SANITIZE=1 node firmware/test/check_transports.mjs --archives /ruta/a/archivos-upstream-en-cache
```

La carpeta contiene `wolfMQTT.tar.gz` de la URL/versión de [dependencies.json](../firmware/arduino/dependencies.json); el runner comprueba su hash antes de usarlo. Requiere Node24, compiladores C/C++, `tar` y dependencias npm existentes. No requiere ESP_SSLClient/BearSSL ni certificados. Cubre TCP, MQTT5, QoS, retained, cancelación y reenvío con cinco transcripciones MQTT independientes. Es mantenimiento opcional: no se ejecuta para compilar/subir con Arduino ni acredita radio física. Ver [evidencia vigente](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md).

El host prueba un consumidor mínimo, exclusión y timeout, gate, capacidades ausentes, concesiones de radio, validación, saturación y recuperación mixta v1/v2 del journal. `check_contracts.mjs` contrasta fixtures v1/v2 y salidas C++ con el codec AJV real. Son comprobaciones de lógica; no acreditan radio, OLED ni flash reales.

Para cerrar la tarea física 2.2, conectar la placa, registrar puerto/build, arrancar y leer metadata v2 por BLE autorizado. Comprobar Idle, rechazos de modos ausentes y, con un consumidor de prueba registrado que no anuncie servicios completos (o un módulo real), transición de ida/vuelta por `set_mode` sin sockets, MQTT, captura ni NAPT residuales. Registrar ACK, perfil, revisión y pantalla; verificar botón corto y pulsación larga. Las pruebas de host o un puerto Bluetooth genérico de Windows no sustituyen esta comprobación. Un consumidor de prueba en la placa solo verifica el ciclo de vida de la API, no acredita detecciones ni servicios del producto.

El incremento de descubrimiento se documenta en [Escudo: descubrimiento y selección](modules/escudo-discovery.md), con órdenes v2, límites y prueba física pendiente.

La [guía de captura](modules/escudo-capture.md) detalla control BLE, parser, generaciones y cobertura. La API añade `capture_generation()`, `capture_dropped()` y `allowed_channels()`; cada contexto conserva la concesión del modo. `RadioEvent::generation` identifica la captura que produjo la copia, sin modificar mensajes v2 ni introducir otro driver.

La [guía de detectores](modules/escudo-detectors.md) documenta las cuatro heurísticas conectadas al parser, sus umbrales/límites, enfriamiento, incidentes y contadores v2. La capacidad implementada no acredita recepción BLE física; la tarea 2.1 sigue pendiente hasta registrar el recorrido real.

La [guía de recuperación](modules/escudo-recovery.md) describe el journal compartido, read_events por cursor, identidad entre boots, gap/cobertura y las comprobaciones físicas pendientes. `persist_module_event` es la rutina interna de serialización/commit usada por el supervisor y las pruebas; el éxito precede al intento de envío en vivo. No se llama de nuevo al reenviar: se leen los bytes originales del journal. El adaptador de archivos de las pruebas no sustituye `NvsStore` del dispositivo.
