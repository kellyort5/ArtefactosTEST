# Evidencia de grupos 1, 2 y 7

**Evidencia histórica:** las pruebas TLS/WSS de este documento corresponden a la configuración anterior. El MVP actual usa MQTT/TCP1883 y WS9001 sin certificados; ver [verificación vigente](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md).


Cambio `sentinel-core-foundation`, rama `feat/sentinel-core-foundation`; 2026-11-08 America/El_Salvador. Ámbito exclusivo: raíz npm/.nvmrc, `contracts/`, `app/`, README raíz, `docs/development.md`, `simulator/README.md` y casillas propias. Sin commits, push, merge, cambios de rama, borrado de fuentes ni cambios de proposal/design/specs; se preservó trabajo compartido. No se modificó `.gitignore` raíz: ya separaba artefactos generados de configuración fuente. El `.gitignore` nativo de Capacitor se conserva dentro de app.

## Implementación

Workspaces npm privados app/contratos, un lockfile y versiones exactas del stack aprobado. Contratos JSON Schema2020-12 state/metrics/event/command/ack y metadata; tipos TypeScript, AJV8, límites UTF-8/2048, campos públicos cerrados, comandos boot/revisión/vigencia30 s. Framing BLE MTU23, cabecera LE8, ensamblado acotado/timeout10 s y limpieza de desconexión. `contracts/README.md` se publicó primero y se coordinó con firmware; conserva payloads exactos/UUIDs/topics/API.

App con adaptadores reales separados BLE comunitario8 y mqtt.js5 WSS, permisos/bond/metadatos/GATT protegido y operaciones de base. El nivel LE SC/MITM lo impone firmware; no se afirma que createBond lo demuestre. UI funcional de conexión, configuración solo BLE, hora, perfiles/capacidades reales, errores, resultados de comandos e historial. No se cargan fixtures en la UI de producto. Detector/captura/HTTP/Telnet/notificaciones/Sanitizador se muestran pendientes.

Store Zustand: publicación/accepted/pending separados de applied/rejected, reconciliación entre canales por resultado o last_command_id/revisión, boot vigente y estados antiguos15 s; retained aislado no acredita presencia. Correcciones coordinadas: lookup AJV solo propiedades propias; retained MQTT de otro boot no sustituye un snapshot vivo BLE. Historial IndexedDB256/nodo, deduplicación por nodo/message_id, cursor solo tras página completa, validación de registros recibidos y gap persistente.

Credenciales en RAM y campos de contraseña limpiados al enviar; nunca en Zustand/IndexedDB/localStorage/variables VITE. Capacitor logging deshabilitado y WebView debug/backup deshabilitados. Android no permite tráfico claro/mixed content; CA de usuario solo en debug, verificación normal de hostname/fecha. Proyecto Android fuente, wrapper y configuración nativa conservados; assets sincronizados/APK/caches ignorados.

## Entorno y comandos comprobados

WSL Linux; Node24.13.1/npm11.8.0 con PATH explícito `/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH`. JDK21.0.12.1, Android SDK `/home/jefry/Android/Sdk`, Platform36/Build Tools36. Gradle8.14.3/AGP8.13.0; Capacitor8.6.0 y BLE8.3.0. Dependencias resueltas exactas en package.json/lockfile; Playwright1.65.0 para Chromium real.

| Comando/comprobación | Resultado |
| --- | --- |
| `npm ci` | PASS: instalación limpia desde package-lock;229 paquetes instalados,238 auditados,0 vulnerabilidades reportadas por npm. |
| `npm run check:contracts` | PASS:24 casos (fixtures válidos/incorrectos, JSON/UTF8/version/tamaño/campos privados, tipos heredados, TTL/metadata, MTU23/límite/timeout/desconexión y URI/puerto/NUL/UTC canónico). |
| `npm run test:app` | PASS:9 casos de comandos/reconciliación/stale/boot/privacidad y journal256/dedup/gap externo e interno/página interrumpida/reapertura. |
| `npm run build` | PASS: TypeScript contratos/app y Vite7 producción; warning de chunk>500k, sin error. |
| `npm run dev -w app -- --host 127.0.0.1` | PASS: servidor Vite real en localhost5173; se comprobó página desde Chromium. |
| `npm run check:browser -w app` | PASS: Chromium real, UI inicial sin datos ficticios, error BLE web accionable, ancho390 sin overflow y IndexedDB real256/dedup/cursor/gap tras recarga. Fixtures de test, no evidencia de placa. |
| `npm run android:sync -w app` | PASS: detecta plugin BLE8.3.0 y copia bundle a proyecto nativo. |
| `ANDROID_HOME=… ANDROID_SDK_ROOT=… npm run android:assemble` | PASS: BUILD SUCCESSFUL,124 tareas, APK debug generado ignorado. Avisos de flatDir/API deprecadas del plugin/XML SDK, sin error. |
| `NODE_EXTRA_CA_CERTS="$PWD/broker/.secrets/ca.crt" npm run check:wss -w app` | PASS:3 casos, operador/observador WSS reales, comando v1 no retenido recibido por TLS, credenciales incorrectas rechazadas y fuente/bundle sin contraseñas privadas. |
| `git check-ignore` | PASS: dist, APK y capacitor.config.json generado ignorados; wrapper JAR y network_security_config.xml no ignorados. |

Se reejecutaron contratos/app/build/sync/assembleDebug tras `npm ci` limpio con éxito; la última compilación incremental completó124 tareas (27 ejecutadas,97 actualizadas; build final13 s). El primer intento WSS encontró TCP timeout hacia localhost8883/9001 y daemon Docker WSL inaccesible; se escaló al coordinador y el responsable del broker recuperó Docker Desktop. El reintento del adaptador completó3/3 pruebas reales, sin modificar broker desde este ámbito. En una repetición final posterior el broker volvió a quedar inaccesible:2 pruebas de conexión fallaron por connack timeout y la revisión de secretos volvió a pasar (1/3); las sondas TCP a ambos listeners también dieron timeout y manage.py ps informó daemon inaccesible. Se escaló de nuevo y se conserva tanto la evidencia PASS real como ese fallo externo; no se atribuye estabilidad continuada del broker a la app. El check usa cuentas desde `broker/.secrets/clients.json` solo en RAM, nunca args/logs; CA pública confiada mediante NODE_EXTRA_CA_CERTS con ruta absoluta. Ningún fixture o publicación de prueba significa orden aplicada por ESP32.

## Seguimiento por tarea

| ID | Implementación y verificación / pendiente |
| --- | --- |
| 1.1 | Completa: cinco componentes separados, firmware/broker independientes; sin Nx/Turbo/backend adicional. |
| 1.2 | Completa: workspace/versiones exactas y `npm ci` limpio PASS, un lockfile; `npm ls --depth=0` sin dependencias incumplidas. |
| 1.3 | Completa: stack inicializado, vista sin datos ficticios, build web y servidor Vite/Chromium comprobados. |
| 1.4 | Completa: README/development/scripts/prerrequisitos y simulador futuro documentados conforme al diseño. |
| 2.1 | Completa: esquemas/fixtures validados, campos y tamaño restringidos. |
| 2.2 | Completa: tipos/codec/AJV/check:contracts y rechazos especificados comprobados. |
| 2.3 | Completa: UUID/metadata/framing/fragmentos documentados y pruebas ejecutables MTU23,2048,timeout/desconexión. |
| 2.4 | Completa: comandos/errores/idempotencia/vigencia/topics/QoS documentados; operaciones de módulos siguen pendientes. |
| 7.1 | Completa: build web/sync/assembleDebug PASS, también tras npm ci; configuración logging:none/debug:false confirmada dentro del APK. Fuentes nativas/wrapper conservados y artefactos ignorados. |
| 7.2 | Implementada; **casilla pendiente**. Usuario difirió expresamente uso de cualquier dispositivo Android; no se ejecutó adb ni emulador. Faltan permisos denegados en teléfono, pairing LE SC/MITM negociado y orden/respuesta real ESP32. No hay puerto ttyUSB/ttyACM visible. |
| 7.3 | Completa por ejecución real3/3: adaptador WSS contra Mosquitto pasó operador/observador, comando v1 recibido por TLS, rechazo de contraseña y ausencia de secretos en fuente/bundle. Repetición final1/3 por broker/daemon inaccesible otra vez, escalada al coordinador; no hubo cambio del adaptador entre ambos resultados. CA/hostname normales con CA pública confiada; no evidencia de nodo físico. |
| 7.4 | Completa: pruebas de pending/accepted/applied y reconciliación/cambio de canal/stale/retained/reboot; sin confundir publicación con aplicación. |
| 7.5 | Implementada; **casilla pendiente**. UI completa y smoke Chromium PASS, pero no se ha mostrado telemetría de ESP32 real; depende de placa y canal reales. No usa estados/indicadores de seguridad fabricados. |
| 7.6 | Completa: tests y Chromium/IndexedDB real comprobaron repetición,256,recarga de app,cursor y gap sin duplicar. Replay individual/página cableado a transportes; recorrido físico queda en7.2/7.5. |
| 7.7 | Documentada; **casilla pendiente**. Build/sync/assemble comprobados; instalación/recorrido y confianza CA Android diferidos por instrucción del usuario. No se reclaman con compilación. |

## Archivos y límites de entrega

`package.json`, `package-lock.json`, `.nvmrc`, `contracts/{package.json,tsconfig.json,README.md,schemas/v1,src,fixtures,test}`, `app/{package.json,tsconfig.json,vite.config.ts,capacitor.config.ts,index.html,src,test,checks,android,README.md,evidence.md}`, `README.md`, `docs/development.md`, `simulator/README.md`; casillas propias en tasks.md bajo flock exclusivo `/tmp/sentinel-core-tasks.lock`, comprobando descripción original y progreso OpenSpec después de cada casilla.

Captura de revisión visual temporal `/tmp/sentinel-app-mobile.png` (no versionada). Pruebas físicas aplazadas sin cambiar specs aprobadas ni reducir alcance. No se implementaron funcionalidades excluidas y no se presenta el MVP como completo.

Revisión final:107 archivos propios revisables sin contraseñas reales del laboratorio ni bloques de claves privadas; assets dentro del APK revisados también sin esas contraseñas. Un solo lockfile npm y dependencias directas exactas confirmados; `git diff --check` PASS. README anterior solo tenía el título ArtefactosProyecto y se reemplazó por documentación de arranque dentro del ámbito autorizado. Enlaces a docs/firmware.md y docs/connectivity.md quedan bajo responsabilidad del trabajador firmware; estaban aún ausentes durante la revisión propia, sin editar ese ámbito. Casillas propias completas:1.1–1.4,2.1–2.4,7.1,7.3,7.4,7.6 (12/15); pendientes7.2,7.5,7.7 por las verificaciones reales indicadas.

## Corrección de paridad UTC — task_e8891e982a83

Se alineó el codec AJV y los cinco esquemas de mensajes con `utc_valid` de firmware, incluido `set_time.args.timestamp`, sin nuevos campos wire ni cambios de specs. Los once usos de date-time ahora exigen formato canónico con T/Z mayúsculas, años0001..9999, horas00..23, minutos/segundos00..59 y fracción decimal opcional, conservando máximo32 caracteres. Ese límite total admite hasta11 dígitos de fracción;12 dígitos exceden32 y se rechazan como en firmware. AJV usa explícitamente ajv-formats en modo full para comprobar días reales del calendario gregoriano y reglas bisiestas.

La regresión focalizada comprueba cinco fechas válidas (extremos0001/9999, año2000 bisiesto y fracciones de1/11 dígitos) y19 inválidas (t/z minúsculas, espacio,0000/10000, leap second,24h, minuto60, offset, fracción vacía/excesiva, newline, meses/días imposibles y febrero inválido en1900/2025/2100). Los casos se aplican tanto a timestamp público con app_sync/network_sync como al argumento set_time; unknown/null conserva su comportamiento.

Archivos de este ajuste: `contracts/src/index.ts`, `contracts/schemas/v1/{state,metrics,event,command,ack}.schema.json`, `contracts/test/contracts.test.mjs`, `contracts/README.md` y este informe. No se alteraron transportes, firmware, broker, package/lockfiles, specs ni casillas. No se repitió WSS por instrucción expresa y por el daemon inestable ya documentado; no se usó Android físico, adb ni emuladores. Resultados del ajuste: `npm run check:contracts` PASS24/24; `npm run test:app` PASS9/9; `npm run android:sync` PASS (incluye build de contratos y web/Vite); `ANDROID_HOME=/home/jefry/Android/Sdk ANDROID_SDK_ROOT=/home/jefry/Android/Sdk npm run android:assemble` PASS, BUILD SUCCESSFUL en16 s,124 tareas (27 ejecutadas/97 actualizadas). Se comprobó el patrón canónico en las11 ubicaciones y `git diff --check` PASS. Se conserva el warning conocido de chunk web>500k y flatDir Gradle, sin fallos. No quedan bloqueos de código para esta corrección; las verificaciones físicas anteriores continúan diferidas.

## Consolidación tras recuperación Docker — task_38abb9c56705

El coordinador informó que recuperó Docker Desktop y comprobó el mismo contenedor Mosquitto activo en loopback. Su ejecución real del adaptador con CA absoluta volvió a completar3/3 pruebas (comando v1 no retenido, rechazos de credenciales/observador y ausencia de secretos); se contrastó esa evidencia ya registrada en [docs/core-foundation-check.md](../docs/core-foundation-check.md), sección «Comprobaciones adicionales del coordinador». Este resultado posterior no elimina las indisponibilidades y timeouts históricos descritos arriba ni demuestra disponibilidad continua.

La paridad UTC y los builds afectados ya estaban terminados en task_e8891e982a83:24/24 contratos,9/9 app, build web/sync y assembleDebug PASS, incluidos los11 usos del patrón canónico y el calendario AJV full. Este despacho confirmó que esa corrección y su regresión siguen presentes; no introdujo cambios de código o bundle, por lo que no repitió las comprobaciones ya aprobadas. Por instrucción expresa no se repitió WSS ni se usó Android físico; este despacho solo actualiza la evidencia, conservando pendientes las verificaciones físicas anteriormente indicadas.
