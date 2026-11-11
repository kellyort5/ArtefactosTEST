# Desarrollo de la base compartida

Trabajar desde la raíz del repositorio en la rama correspondiente a cada cambio; la base se publicó desde `feat/sentinel-core-foundation`. Leer [brief](brief.md), [CONTRIBUTING](../CONTRIBUTING.md) y el [diseño de la base archivada](../openspec/changes/archive/2026-11-10-sentinel-core-foundation/design.md). No se necesitan servicios de monorepo ni un backend adicional.

## Entorno y dependencias

- Node24 LTS: `.nvmrc` fija **24.13.1**; npm **11.8.0**.
- App: React19, Vite7, TypeScript5, Tailwind4/plugin Vite, Zustand5, mqtt.js5, Capacitor8 y BLE comunitario8. Versiones exactas en `app/package.json` y lockfile raíz. AJV8 y ajv-formats3 en `contracts/`.
- Android: JDK21, SDK Platform36 y Build Tools36; Gradle8.14.3 y AGP8.13.0 provistos por Capacitor8.6.0. Proyecto nativo y wrapper versionados; APK/caches/assets web regenerados ignorados.
- Firmware: PlatformIO/ESP-IDF/FreeRTOS, herramientas y configuración en [firmware](firmware.md).
- Broker: Docker Desktop/Engine y Compose, OpenSSL/Python3 para material local; [preparación](../broker/README.md).

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
| `NODE_EXTRA_CA_CERTS="$PWD/broker/.secrets/ca.crt" npm run check:wss -w app` | Adaptador mqtt.js real contra broker local preparado; cuentas solo desde archivo privado. |

Para la comprobación Chromium, `npm exec -w app -- playwright install chromium` instala su navegador si falta; no es un emulador Android. Los fixtures utilizados en tests no acreditan funcionalidades del ESP32.

## Firmware, broker y simulador

```sh
pio run -d firmware -e sentinel_s3
pio run -d firmware -e sentinel_s3 -t upload --upload-port PUERTO_REAL
pio device monitor --port PUERTO_REAL --baud 115200
```

Los comandos upload/monitor requieren placa y puerto comprobados; nunca inventar `/dev/ttyUSB0`. Detalles de [cableado y memoria](firmware.md). Sin placa, solo build y pruebas focalizadas de host son evidencia válida.

Preparar y ejecutar broker conforme a [broker/README.md](../broker/README.md). Sus scripts son independientes de npm; CA/usuarios/ACL deben existir antes del arranque. Mantener loopback por defecto. Usar una IP/SAN y firewall explícitos para LAN autorizada, sin abrir puertos del router. Un broker accesible desde WSL no demuestra acceso desde teléfono/ESP32.

[simulator/README.md](../simulator/README.md) delimita un futuro apoyo ligero. No hay simulador de comportamiento de producto en esta base; las pruebas pequeñas de contratos son auxiliares y no sustituyen controles/periféricos reales.

## Seguimiento y comprobación

[App](../app/README.md) describe instalación, confianza CA y recorrido funcional BLE/WSS. El usuario autorizó continuar con el teléfono, la ESP32 y el emulador del SDK Windows. Seguir la [guía de prueba](core-foundation-test-guide.md) y registrar las comprobaciones ejecutadas en la [evidencia integral](core-foundation-check.md). Las tareas de teléfono y radio requieren su recorrido físico completo para cerrar las casillas correspondientes.

No versionar node_modules, dist, .gradle, build, APK, configuración local privada ni credenciales. Mantener native Android, wrapper JAR, configuración fuente, esquemas, fixtures y lockfile. Las actualizaciones de casillas deben releer/contrastar la línea original bajo lock exclusivo `/tmp/sentinel-core-tasks.lock`; solo completar comportamiento y verificación realmente cumplidos.

## API de módulos de firmware (Persona 1, 2 y 3)

El cambio `sentinel-firmware-module-api` publica [module_api.hpp](../firmware/components/sentinel_api/include/module_api.hpp). Persona 2 implementa Sanitizador y Persona 3 Cebo como componentes independientes; dependen de **`sentinel_api`**, no de `sentinel`. El núcleo registra las implementaciones, conserva un único supervisor y administra Wi-Fi, BLE, identidad, serialización y el journal NVS de 64 eventos. Los detectores, servicios HTTP/Telnet y lógica NAT/DNS/DoH siguen en sus respectivos cambios.

El firmware de esta rama anuncia **metadata v2** y recibe órdenes v2 conforme a [module-protocol.md](../contracts/module-protocol.md). Las órdenes v1 reciben `unsupported_version`; `start_capture` y `start_honeypot` conservan su significado reservado de v1. Los eventos históricos v1 se recuperan sin convertirlos y se publican en `sentinel/v1/.../events`; los nuevos mensajes usan `sentinel/v2/...`. La integración del adaptador v2 de la app pertenece a su encargo; una app que solo envía v1 no puede controlar este firmware.

### Crear y registrar un componente

| Integrante | Directorio/componente ESP-IDF | Factory con enlace C | Opción de configuración |
| --- | --- | --- | --- |
| Persona 1 | `firmware/components/escudo/` | `sentinel_escudo_module()` | `CONFIG_SENTINEL_MODULE_ESCUDO=y` |
| Persona 2 | `firmware/components/sanitizador/` | `sentinel_sanitizador_module()` | `CONFIG_SENTINEL_MODULE_SANITIZADOR=y` |
| Persona 3 | `firmware/components/cebo/` | `sentinel_cebo_module()` | `CONFIG_SENTINEL_MODULE_CEBO=y` |

Cada factory devuelve `sentinel::Module*` con vida de todo el arranque; puede devolver `nullptr` si la implementación no está disponible. El `CMakeLists.txt` del componente declara `REQUIRES sentinel_api` y sus dependencias oficiales necesarias. El núcleo añade la dependencia del componente y llama la factory al habilitar la opción; no hace falta editar `runtime.cpp`, crear otro supervisor ni modificar `firmware/src/main.cpp`. Activar una opción sin incluir su componente debe producir un error de build. Las tres opciones están deshabilitadas por defecto: esta entrega no anuncia módulos funcionales ficticios.

Ejemplo de punto de entrada para un componente futuro (adaptar el nombre; no es una implementación del módulo):

```cpp
#include "module_api.hpp"
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

`ApConfig.ipv4/netmask` usan orden de red. El gestor conserva los netifs durante todo el boot. `CONFIG_LWIP_IP_FORWARD` y `CONFIG_LWIP_IPV4_NAPT` están habilitados en defaults y configuración mantenida; habilitar compilación no activa NAPT. El gestor solo permite NAPT en Sanitizador con AP y dirección de upstream, y lo desactiva antes de liberar la red. Esto prepara la integración de Persona 2; no acredita el funcionamiento de su pasarela.

`resources.poll(RadioEvent&)` entrega copias desde una cola de **8** entradas sin bloqueo: final de scan, conexión/desconexión STA, dirección IPv4 obtenida, clientes AP y metadatos/tramas. Los frames conservan hasta **512 bytes**, longitud original, canal, RSSI y tiempo; comprobar truncamiento antes de parsear. `scan_results()` entrega como máximo **32** registros por consulta; las enumeraciones de seguridad son las oficiales del ESP-IDF fijado. El módulo procesa estas copias en `tick()` y mantiene sus propios límites. `resources.dropped()` informa saturación. No interpretar una pausa, pérdida de cola o tramo truncado como evidencia concluyente de ataque.

### Eventos, métricas y persistencia

`context.emit_event(name,severity,details,simulated)` y `publish_metrics(interval_ms,values)` copian JSON de hasta **1536 bytes** antes de retornar. El contrato completo sigue limitado a **2048 bytes**; usar páginas de 1–4 elementos y reducirlas si el sobre no cabe. `details` y `values` contienen exclusivamente las estructuras v2 de [schemas](../contracts/schemas/v2/common.schema.json). El núcleo compila sus validadores desde estos schemas durante CMake, sin otro contrato ni dependencia de validación en el dispositivo.

Las llamadas no escriben flash ni transportes: validan y encolan en una cola común de **8** entradas. Un retorno `false` indica datos inválidos, contexto inactivo o saturación; no reintentar en un bucle bloqueante. La pérdida se contabiliza y los servicios continúan. Los callbacks de captura solo copian a la cola de radio del gestor.

El supervisor asigna identidad, secuencia, cursor del journal y marca temporal/calidad al procesar la salida. Valida el mensaje completo, persiste los eventos aceptados en el journal existente y luego los entrega por BLE/MQTT según el canal disponible. Las métricas son efímeras. Los eventos reales usan `origin:"real"`; fixtures y simulación usan `"simulated"`, preservado al recuperar/republicar. No incluir contraseñas, tokens, cuerpos ni detalles adicionales: los objetos cerrados del schema los rechazan. No crear almacenamiento paralelo desde los módulos.

El botón corto rota únicamente los modos registrados (Escudo → Sanitizador → Cebo); sin módulos, permanece Idle. Mantener pulsación de 3 s abre los 120 s de emparejamiento y no dispara además la acción corta. OLED usa el perfil/subestado/error del supervisor y mantiene el código de emparejamiento. Las alertas recientes en OLED corresponden al cambio de integración de los tres modos.

### Verificar este cambio

La [evidencia de esta entrega](../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/verification.md) distingue software verificado y prueba física pendiente.

```sh
python3 firmware/test/run_host.py
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH python3 firmware/test/check_contracts.py
pio run -d firmware
python3 firmware/test/check_config.py
```

El host prueba un consumidor mínimo, exclusión y timeout, gate, capacidades ausentes, concesiones de radio, validación, saturación y recuperación mixta v1/v2 del journal. `check_contracts.py` contrasta fixtures v1/v2 y salidas C++ con el codec AJV real. Son comprobaciones de lógica; no acreditan radio, OLED ni flash reales.

Para cerrar la tarea física 2.2, conectar la placa, registrar puerto/build, arrancar y leer metadata v2 por BLE autorizado. Comprobar Idle, rechazos de modos ausentes y, con un consumidor de prueba registrado que no anuncie servicios completos (o un módulo real), transición de ida/vuelta por `set_mode` sin sockets, MQTT, captura ni NAPT residuales. Registrar ACK, perfil, revisión y pantalla; verificar botón corto y pulsación larga. Las pruebas de host o un puerto Bluetooth genérico de Windows no sustituyen esta comprobación. Un consumidor de prueba en la placa solo verifica el ciclo de vida de la API, no acredita detecciones ni servicios del producto.
