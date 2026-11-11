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
