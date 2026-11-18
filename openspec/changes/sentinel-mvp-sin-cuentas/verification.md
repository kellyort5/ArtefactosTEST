# Verificación integrada del MVP sin cuentas

Fecha local: 2026-11-17 (Guatemala/UTC−6); builds nativos terminados el 2026-11-18 UTC. Rama de entrega: feat/sentinel-mvp-integration. Base origin/develop bb2ee4a; incluye P1 b63772a, checkpoint MQTT01f6050 y resolución explícita de conflictos de documentación/app. No se archivan cambios ni se cierran pruebas físicas por integrar.

## Cambios revisados

- Conservadas la reubicación de caché32 en PSRAM, comprobaciones de asignación/tareas y correcciones de pila BLE de P1. Su tarea4.3 de imagen final sigue pendiente; código visible en la imagen intermedia no prueba el arreglo final.
- Mosquitto usa imagen/entrypoint oficiales y TCP1883/WS9001 anónimos, sin TLS, password_file, ACL o preparación de cuentas. El volumen se conserva; secretos locales anteriores no se borran ni se montan.
- App/firmware/contratos carecen de campos/cuentas/roles MQTT. configure_connectivity recibe ssid/wifi_password/mqtt_uri y NVS versión3; falla visible en formato anterior sin borrar identidad/bonds/historial. BLE y HTTPS/DoH conservan seguridad y permisos necesarios.
- Consumer gates mantienen nodo/topic, modo, boot/revisión/TTL, retención y dedup. El contexto contractual identifica el receptor con deviceId; en MQTT los indicadores de autenticación no se exigen. Los paquetes CONNECT de wolfMQTT no llevan usuario ni contraseña. Nada de esto autentica al emisor anónimo.
- Reparto y planificación activos actualizados en docs/mvp-policy.md, guías personales y fichas. P4 consume diagnóstico P1 y software broker, verifica LAN e implementa UX final; P3 no pide rol notify/ACL. P2 mantiene Sanitizador completo. Informes antiguos se identifican como evidencia histórica.

## Pruebas de software

| Comprobación | Resultado y límite |
| --- | --- |
| npm run check:contracts | PASS,156 tests. Configuración sin cuenta/CA, rechazo de campos antiguos, MQTT anónimo y BLE con autenticación/cifrado conservados. |
| npm run test:app | PASS,30 tests. Sin roles; canal/estado vivo, privacidad Wi-Fi, P1 y ACK/páginas/dedup. Se corrigió el reloj ficticio del test al comprobar envío MQTT permitido. |
| npm run build -w app | PASS sin opción diagnóstico; advertencia de chunk grande preexistente. |
| npm run test:firmware | PASS. C++ real de núcleo/contratos/journal/bonds, fallos de asignación y digest P1, persistencia/rollback/UTF8/errores NVS y69 fixtures v2. Dobles host, sin placa. |
| npm run test:escudo | PASS. Descubrimiento/captura/cuatro detectores/recuperación; seis eventos únicos/seis repetidos. Sin radio real. |
| node firmware/test/arduino_build_test.mjs | PASS. Guardas, opciones de módulos/display0/1 y compilación host. |
| node firmware/test/check_transports.mjs --archives /tmp/sentinel-anon-archives | PASS. wolfMQTT2.2.0 original verificado por SHA256, producción worker con dobles de socket/FreeRTOS. Decoder mqtt-packet independiente confirma cinco trazas sin credenciales CONNECT, MQTT5/expiry0/LWT/QoS1/RAP/retained/cierre/replay idéntico. |
| python3 broker/scripts/manage.py config/up | PASS con Docker Desktop, configuración nueva sin secretos/cuentas; preserva volumen. |
| node broker/scripts/smoke.mjs --restart | PASS broker real: TCP/WS anónimos, entrega cruzada, retained y persistencia tras reiniciar. Topic temporal limpiado. |
| npm run check:mqtt -w app | PASS adaptador WS real publica comando validado al receptor TCP anónimo. No ejecuta firmware. |
| checks/browser.mjs | PASS Chromium: navegación, permisos BLE web, rechazo sin éxito falso, historial IndexedDB, responsive y formularios sin login. Fixtures explícitos. |
| checks/p1-browser.mjs | PASS a390px: catálogo/ACK por controller real con transporte BLE sustituido solo en test, desconexión, reenvío explícito e invalidación privada. No radio. |
| checks/browser-broker.mjs | PASS Chromium a390px con broker TCP/WS real, estado v2 fixture y ausencia de campos CA/cuentas/roles. No ESP32. |
| openspec validate --all --strict --json | PASS,41/41 elementos:27 cambios y14 specs principales. Deltas activos alineados sin retirar escenarios recientes. |

Pruebas Chromium: Vite con VITE_SENTINEL_P1_TOOLS=1 en http://127.0.0.1:5188; usar SENTINEL_APP_URL para ese puerto. Los fixtures o sustitutos solo están en runners de prueba, nunca en el producto.

## Compilación Arduino Windows real

Core oficial ESP32 **3.3.12**/IDF5.5.5, wolfMQTT **2.2.0**, NetworkClient oficial. Sin ESP_SSLClient o SDK propio. FQBN esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel. Ambas compilaciones exit0, sin grabar placa.

| Perfil | Imagen / límite | RAM global / límite | Terminado UTC |
| --- | --- | --- | --- |
| Serie1 |1267482 /4194304 bytes|98516 /327680 bytes|2026-11-18T03:10:13Z|
| Producto0 |1254326 /4194304 bytes|98484 /327680 bytes|2026-11-18T03:10:48Z|

Cifras del enlazador: no miden heap/PSRAM/radio físicos. BIN SHA256 serie:3ace989a3bdc6b934757d46e8cced5fc06554365c36d09c3312f5e3c6def825d; producto:2f976cd935e5f093fdfb438cb8237fcb196052e1938503d32dcac43c986b7467.

Logs, manifest de entradas y script local reproducible fuera de Git en %LOCALAPPDATA%/Temp/sentinel-mvp-integrated. Se compararon entradas compilables con las fuentes mantenidas: coinciden salvo opción serie1 de la copia preparada; Git conserva0. El .ino listo para pruebas está en input/SentinelNode/SentinelNode.ino, manteniendo toda la carpeta.

## APK integrado de diagnóstico

VITE_SENTINEL_P1_TOOLS=1 npm run android:sync PASS. La compilación incremental encontró caché de assets inválida; se resolvió con la operación oficial ./gradlew clean assembleDebug --no-daemon --max-workers=2, sin personalizar Gradle/core o instalar herramientas. **BUILD SUCCESSFUL**,129 tareas; JDK21/SDK36.

APK local ignorado: .cache/sentinel-mvp-integrated-debug.apk,4495683 bytes, SHA256 f810ac2e2cdc144cb378923271dc0b08fe083844a19962f7933037fd3625dfe2. La guía P1 utiliza esta referencia, no los APK anteriores con TLS/cuentas. Un APK de producto se construye sin VITE_SENTINEL_P1_TOOLS; la fuente mantiene la opción desactivada por defecto.

## Pendientes y límites

- P1 conserva4.3 y las pruebas físicas archivadas pendientes: cargar imagen final, pairing protegido, estado/ACK sin reinicios, captura/coexistencia y persistencia ante RESET/corte tras commit.
- OLED física/alertas recientes, red LAN desde teléfono/ESP32, escenarios RF autorizados, Cebo, Sanitizador, UX final y push con app cerrada requieren sus entregas/evidencia. No se sustituyen por compilación o fixture.
- Aceptación explícita del usuario: MQTT sin cifrado/autenticación/aislamiento. Cualquier cliente con acceso puede leer/publicar/suplantar/ordenar Cebo. Solo LAN de confianza propia/autorizada, sin exposición Internet; device_id no es credencial.
- No se borra NVS, volumen, ramas o secretos locales antiguos. La publicación solicitada usa PR hacia develop, sin modificar main ni forzar historial.

## Revisión previa a publicar

Enlaces locales de98 Markdown modificados/nuevos: PASS. Revisión de218 archivos afectados respecto de develop: sin secretos privados del laboratorio ni artefactos/dependencias/cache/builds versionados; valores secretos comparados sin imprimirlos. git diff --check PASS después de normalizar fin de archivo. Las cifras corresponden al alcance inspeccionado; se vuelve a comprobar la lista preparada antes del commit.
