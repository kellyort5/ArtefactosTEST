# Verificación MQTT del MVP

Fecha: 2026-11-17. Rama `feat/mqtt-mvp-lan`. El usuario autorizó retirar TLS de MQTT, conservando pruebas con Android real en red de confianza propia o autorizada, sin administrar el router ni instalar certificados. No se ejecutaron commits, push, merges ni carga USB.

## Resultado implementado

- Mosquitto oficial: MQTT5/TCP1883 y MQTT5/WS9001 publicados en LAN por defecto; sin TLS/CA. Conserva cuentas y volumen existentes. `manage.py up/restart` aplica ACL fuente a las identidades existentes, con permisos equivalentes node/operator/viewer en v1/v2. Notify y sus pruebas físicas siguen en su encargo.
- ESP32: `NetworkClient` oficial en el worker wolfMQTT2.2.0, sin ESP_SSLClient, verificador TLS ni fixtures/stubs de certificados obsoletos. MQTT no depende de UTC. Se conservan colas/cancelación, QoS, LWT, retained, validación/idempotencia y exclusión de modos.
- Configuración v1/v2: `mqtt://host[:puerto]`, SSID/password Wi-Fi y cuenta MQTT, sin `ca_certificate`. Schemas, tipos y validador C++ regenerados. Registro NVS versión2, rechazo visible del formato previo y reaprovisionamiento por BLE; sin migración ni borrado automático de identidad/bonds/historial.
- App/Android: WS con IP del PC; sin formulario CA ni instalación de certificados. Assets empaquetados desde `http://localhost`, `allowMixedContent:false`, tráfico claro permitido, confianza HTTPS del sistema conservada. BLE SC/MITM y HTTPS/DoH no se rebajan.
- Documentación operativa, R-04, roadmap, specs principales y encargos relacionados actualizados; informes anteriores identificados como históricos.

## Comprobaciones ejecutadas

| Comprobación | Resultado y alcance |
| --- | --- |
| `npm run check:contracts` | PASS: fixtures y codec v1/v2, límites, esquema sin CA, rechazo del formato anterior y contexto MQTT autenticado sin cifrado; BLE sigue exigiendo cifrado. Cuatro archivos de suites. |
| `npm run test:app` | PASS: cuatro archivos de suites; WS por IP, rechazo de endpoints/cuentas inválidos, operador/observador, retención, versiones, historial y privacidad. |
| `npm run build`, `npm run android:sync` | PASS: TypeScript/Vite y sincronización nativa. Aviso preexistente de tamaño del bundle, sin fallo. |
| `node firmware/test/check_contracts.mjs` | PASS: lógica del core, cierre/transiciones, NVS con fallos/rollback y conexión sin UTC, BLE storage; cruce de mensajes C++→AJV y 69 fixtures v2→C++/AJV. Dobles host explícitos, no radio física. |
| `node firmware/test/arduino_build_test.mjs` | PASS: flags/factories, guardas del core/particiones/BLE/HTTPS y MQTT5/TCP, sin exigir ESP_SSLClient. Preprocesador/enlace host, no placa. |
| `node firmware/test/check_transports.mjs --archives <cache-original>` | PASS: worker de producción y wolfMQTT original verificado por SHA256; TCP DNS/IP, fallo de conexión, credenciales/puerto ausentes, cancelación, retained, QoS1 y reenvío; cinco transcripciones MQTT5 decodificadas independientemente. Socket/DNS/RTOS dobles, no ESP32 real. |
| Preparación aislada con imagen Mosquitto real | PASS: seis contraseñas PBKDF2, cuentas privadas conservadas, ACL v1/v2 y cero archivos de certificados. Temporal eliminado; cuentas habituales sin rotación. Prueba local en `.cache/check-mqtt-prepare.py`. |
| `python3 broker/scripts/test_common.py` | PASS: seis regresiones de selección Docker. |
| `manage.py config/up/ps` | PASS: mismo servicio/volumen y cuentas, puertos `0.0.0.0:1883` y `0.0.0.0:9001`. No se modificó router ni firewall. |
| `node broker/scripts/smoke.mjs` | PASS con broker real: node→viewer, operator→node y node→operator por TCP y WS en v1/v2; rechazo anónimo/contraseña, permisos de escritura y filtrado de lectura entre nodos; retained después de reiniciar. Solo sondas descartables locales. |
| `npm run check:mqtt -w app` | PASS: tres casos del adaptador real sobre WS, comando no retenido hacia TCP, contraseña inválida/viewer y revisión de secretos en fuente/bundle. |
| `npm run check:browser -w app` | PASS Chromium: interfaz, BLE web no disponible, navegación/capacidades v2, rechazo sin éxito falso, responsive e IndexedDB/cursor/gap. Fixtures de prueba. |
| `npm run check:browser:mqtt -w app` | PASS Chromium a 390×844: WS real sin CA, estado v2 desde cliente TCP real del laboratorio, formulario sin CA, limpieza de contraseña y desconexión. Estado fixture explícito, no telemetría física. |
| `openspec validate sentinel-mqtt-mvp-lan --strict` y `sentinel-broker-lan --strict` | PASS; se conserva el alcance restante de notificaciones y comprobaciones físicas. |

## Build oficial Arduino Windows

Arduino CLI instalado en Arduino IDE, core oficial ESP32 **3.3.12**/IDF5.5.5, FQBN `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`. Compile limpio **exit0**, sin modificar core/SDK/bibliotecas ni grabar placa. Script reproducible local `.cache/mqtt-mvp-build.ps1`; salida en `%LOCALAPPDATA%/Temp/sentinel-mqtt-mvp-build/`.

Imagen **1255438 bytes**, variables globales **157684 bytes**. Son cifras de enlace, no memoria física medida. Bibliotecas efectivas: wolfMQTT2.2.0 y Networking3.3.12; ningún depfile referencia ESP_SSLClient o NetworkClientSecure. BIN SHA256 `5b2847d02cb54666df7f46d687a87d06b7847a07874c08f54b2900810f2b3f63`.

## APK Android

`assembleDebug` **BUILD SUCCESSFUL**, 124 tareas. SDK36/JDK21/Gradle8.14.3 existentes, sin instalar dependencias nuevas. El build convencional encontró recursos incrementales inválidos y un clean detectó escrituras concurrentes; se verificó el mismo proyecto con directorios de build/cache temporales aislados mediante un init script de Gradle, sin cambiar fuentes ni interferir con otros builds.

Comando de esta verificación desde `app/android`:

```sh
./gradlew --init-script /tmp/sentinel-mqtt-android.init.gradle --project-cache-dir /tmp/sentinel-mqtt-android-project-cache --no-daemon --max-workers=2 assembleDebug --stacktrace
```

El init script asignó `layout.buildDirectory` por proyecto bajo `/tmp/sentinel-mqtt-android-build`. No es un requisito adicional de instalación o uso; el flujo normal sigue documentado en app/README.md.

APK disponible en `.cache/sentinel-mqtt-mvp-debug.apk` (ignorado por Git), SHA256 `87829ee125fb5751f183c26fd90751df08fb7b9529414daf4695366697f76665`. Inspección con aapt2 del **APK compilado**: permiso INTERNET, usesCleartextTraffic=true, Network Security Config cleartextTrafficPermitted=true y trust-anchors system sin CA de usuario. `assets/capacitor.config.json` empaqueta origen Android HTTP local, mixed content deshabilitado y logging deshabilitado. No incluye clients.json ni CA del laboratorio.

## Revisión final

`git diff --check` PASS. Enlaces relativos de los Markdown modificados/nuevos: sin destinos ausentes. Inspección de109fuentes/documentos/artefactos nuevos o modificados: no contienen contraseñas privadas del laboratorio ni claves privadas. APK/binarios/builds, proveedores y material `.secrets` permanecen ignorados y fuera de la entrega Git.

## Límites y prueba física

ADB Windows no encontró dispositivos; no hay AVD configurado ni se identificó placa para grabación. Por eso **no se acredita conexión por radio desde teléfono/ESP32 ni una LAN ajena concreta**, y esas tareas físicas conservan su estado pendiente. La compilación del APK, su política de red y WS real en Chromium acreditan las piezas de software, no sustituyen ese recorrido.

El Cebo real y sus servicios siguen en su encargo; no se habilitan capacidades ficticias para probar MQTT. Para comprobar hardware cuando esté disponible: instalar el APK, conectar PC/teléfono/ESP32 a una red autorizada de confianza, usar IP LAN del PC con WS9001/TCP1883, configurar el nodo por BLE y probar Cebo al estar implementado. No instalar CA, configurar SAN/DNS ni cambiar el router. Puede ser necesario permitir Docker/puertos en el firewall del PC. Si la Wi-Fi aísla clientes, usar otra red/punto de acceso disponible que permita comunicación; el software no puede anular ese aislamiento.

El riesgo de tráfico MQTT sin cifrar está aceptado para este MVP; cuentas exclusivas del laboratorio, sin exponer Internet. La sesión app conserva credenciales solo en RAM; el nodo persiste configuración autorizada conforme a la política operativa. Los secretos permanecen fuera de Git, logs y evidencia.
