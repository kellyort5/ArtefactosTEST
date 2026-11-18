# Verificación — pruebas P1 sin OLED

**Corrección posterior comprobada por software:** ver [fallos físicos de memoria/pila y versión final](ble-stack-verification.md). Los builds iniciales de este documento se conservan como antecedente; la última corrección sigue pendiente de cargar porque el usuario ya no dispone de la placa.

Fecha: 2026-11-17. Rama `feat/sentinel-p1-serial`, base `b1f8729`. Evidencia generada antes de publicar la rama; integración mediante PR y revisión pendiente. Checkout aislado `/home/jefry/orca/workspaces/ArtefactosProyecto/sentinel-p1-serial`; el directorio original sigue reservado al trabajo MQTT LAN concurrente.

## Resultado de software

| Comprobación | Resultado y límite |
| --- | --- |
| `npm run test:firmware` | PASS. Núcleo, framing, supervisor, journal, bonds, conectividad, 69 fixtures v2 y prueba del display local real con writer de test. Sin hardware. |
| `npm run test:escudo` | PASS. Descubrimiento, parser/captura, detectores, persistencia y recuperación de seis eventos únicos/seis repetidos. Sin radio real. |
| `node firmware/test/arduino_build_test.mjs` | PASS. Flags 0/1, rechazo 2, contador temporal en ambos perfiles, guards del core y transportes. No sustituye compilación nativa. |
| `npm run test:app` | PASS, 29 tests. Diagnóstico deshabilitado, privacidad/JSON, BLE desconectado, metadatos frescos, negativos, reenvío explícito idéntico y aislamiento de observers. |
| `npm run build` | PASS sin opción de pruebas. Chromium sobre bundle de producción confirma panel ausente y ausencia de errores JS. Warning de tamaño de chunk preexistente; sin optimización prematura. |
| `VITE_SENTINEL_P1_TOOLS=1 npm run android:sync` | PASS: build web optativo y copia de assets a Android. |
| `npm run android:assemble` con JDK21/SDK36 | BUILD SUCCESSFUL. APK diagnóstico generado, sin instalarlo ni dar BLE por probado. |
| `SENTINEL_APP_URL=http://127.0.0.1:5184 node app/checks/p1-browser.mjs` con Vite y opción 1 | PASS. Chromium ancho 390: controles deshabilitados sin BLE, catálogo/ACK mediante controller real con frontera BLE falsa solo en runner, reenvío explícito, rechazo JSON privado y ancho móvil. No evidencia de radio. |
| `openspec validate sentinel-p1-pruebas-sin-oled --strict` | PASS. |

La prueba de Chromium usa Vite con `VITE_SENTINEL_P1_TOOLS=1 npm exec -w app -- vite --host 127.0.0.1 --port 5184 --strictPort`. La app de producto no incorpora mocks. Los resultados de ACK/estado del panel en uso normal vienen exclusivamente del nodo.

## Artefactos locales listos para la sesión

APK: `C:\Users\Jefry\AppData\Local\Temp\sentinel-p1-serial-check\sentinel-p1-diagnostic.apk`, 4499134 bytes, SHA-256 `402b7e437e0d0cc621857283e929d4e9fe97018fcfcac5b673e0b799fe1194d4`. Código fuente por defecto sin panel; el APK fue construido con `VITE_SENTINEL_P1_TOOLS=1` y sus assets de diagnóstico se comprobaron dentro del ZIP.

Sketch de prueba: `C:\Users\Jefry\AppData\Local\Temp\sentinel-p1-serial-check\input\SentinelNode\SentinelNode.ino`. La copia preparada usa `SENTINEL_SERIAL_TEST_DISPLAY=1`; la fuente Git conserva `0`. Mantener carpeta completa. UART0, USB CDC On Boot Disabled, monitor a 115200 antes de pairing. El driver UART no puede comprobar si una persona está leyendo. No guardar el passkey en logs/capturas públicas.

## Compilación nativa Windows

Perfil serie (`SENTINEL_SERIAL_TEST_DISPLAY=1`): **PASS**, exit 0, terminado `2026-11-17T23:25:34Z`. Imagen 1394858/4194304 bytes; RAM global 160452/327680 bytes. Son cifras del enlazador, no memoria física ni heap/PSRAM observados. Perfil normal (`SENTINEL_SERIAL_TEST_DISPLAY=0`): **PASS**, exit 0, terminado `2026-11-17T23:26:23Z`. Imagen 1381722/4194304 bytes; RAM global 160428/327680 bytes. Ambos perfiles usan las mismas bibliotecas y recetas oficiales. Core oficial ESP32 3.3.12; wolfMQTT 2.2.0 y ESP_SSLClient 3.1.3 ya instalados. FQBN `esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel`. Se usa copia temporal Windows y recetas oficiales, sin cambios al core, carga, exportación, particiones o bibliotecas.

Se comprobó que los inputs compilables del sketch temporal coinciden con la fuente del checkout, salvo la activación explícita de la opción 1 para la copia de pruebas. El script terminó restaurando esa opción 1 en la copia; Git conserva 0. SHA-256 de `SentinelNode.ino.bin`: serie `dbade1179d9c94788fab6e1a1bc32f7d409f91ff86184adbf67c482afd1b50ef`; normal `31f392ebcaab3ee71bd91165b5211d0dac25e94fdf8c658319a4f7f154211ce7`.

Los logs, manifest fuente y script de comprobación quedan fuera de Git en `%TEMP%\sentinel-p1-serial-check`. Una primera invocación heredó cwd UNC: PowerShell trató stderr como error aunque el compilador seguía activo. Se canceló únicamente la compilación de esta prueba y se relanzó desde una ruta local, con captura stderr sin abortar el proceso. Esa invocación no acredita un build válido.

## Pendientes físicos conservados

- Ejecutar [la guía P1](../../../docs/guia-pruebas-persona-1.md) con ESP32 y teléfono: arranque, memoria real, GPIO4, ventana/passkey/SC/MITM, catálogo, captura, detectores, reconexión, journal y persistencia ante RESET/corte tras commit.
- Pantalla OLED real y futuras alertas recientes: no quedan aprobadas por usar UART. La fuente vuelve a opción 0 al probar el producto.
- Patrones de ataques en radio y ausencia de probes: laboratorio/red propia o autorizada, con instrumentación y responsable asignado. No se fabrican incidentes reales desde fixtures.
- Sanitizador, Cebo, pantallas finales y notificaciones: sus entregas e integración final siguen pendientes de los compañeros. Este cambio no las simula ni las elimina.

No se modifican casillas físicas de specs archivadas. No se graba placa ni se archiva este cambio automáticamente.
