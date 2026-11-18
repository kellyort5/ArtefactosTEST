# Comprobación de sentinel-core-foundation

**Registro histórico de la base:** TLS/WSS, confianza CA y ACL solo v1 de las ejecuciones anteriores fueron sustituidos por MQTT/TCP1883 y WS9001 con ACL v1/v2 el 2026-11-17. Consultar la [verificación vigente](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md) y la [guía de pruebas actual](core-foundation-test-guide.md). No usar estos resultados antiguos como instrucciones de configuración.


**Aviso de vigencia:** este informe conserva resultados, autorizaciones y pendientes históricos. La [política aprobada posterior](persistence-policy.md) sustituye RAM-only y continuidad USB: NVS explícita para configuración autorizada, conservación en reinicio/corte normal y reinstalación USB con reinicialización permitida sin migración. Sus builds/logs no acreditan la nueva tabla ni cierran pendientes físicos.

**Evidencia histórica de la base:** los comandos PlatformIO y rutas originales de este informe describen las ejecuciones registradas entonces. El flujo vigente de firmware es [Arduino IDE Windows](firmware.md); el [mapa de traslado](firmware-arduino-migration.md#7-árbol-único-y-mapa-del-movimiento-realizado) localiza las fuentes actuales. No se reinterpretan estos resultados ni sus pendientes como pruebas del port Arduino.


Registro de implementación y evidencia del cambio OpenSpec `sentinel-core-foundation`. La ejecución inicial corresponde al 2026-11-08 en America/El_Salvador (2026-11-09 UTC); su cierre administrativo autorizado se registra el 2026-11-10 en America/Guatemala.
Las casillas del [cambio archivado](../openspec/changes/archive/2026-11-10-sentinel-core-foundation/tasks.md) solo se completan cuando se cumple toda la comprobación de la tarea. Este documento no sustituye esas condiciones.

## Cierre autorizado del 2026-11-10

El usuario autorizó expresamente archivar esta base aceptando las validaciones físicas pendientes **bajo la suposición de que el hardware funciona**. También autorizó realizar commits, publicar la rama de esta entrega y abrir una PR hacia `develop` conforme a las reglas del equipo. No se autoriza por ello el merge ni la eliminación de ramas.

**Estado de cierre: archivado con validaciones pendientes aceptadas; 26/45 tareas verificadas y 19 casillas abiertas.** La aceptación permite continuar los siguientes cambios. No demuestra que funcionen la placa, el enlace BLE, los periféricos, la persistencia física o el recorrido LAN, ni convierte los resultados asumidos en PASS.

| Clase de resultado | Evidencia o tratamiento |
| --- | --- |
| Pruebas ejecutadas | Builds y pruebas de contratos, lógica host del firmware, app, Android y broker documentados en este informe y en los informes de cada componente. Las revalidaciones de publicación se registran por separado. |
| Comprobaciones no ejecutadas, aceptadas como supuesto | Memoria y arranque en ESP32, OLED/botón, NVS real tras reinicio, BLE/GATT y seguridad negociada, STA/MQTT del nodo, transiciones en placa, recorrido físico app–ESP32 y acceso LAN/firewall. Se conservan sin marcar. |
| Riesgo aceptado | Un fallo físico o de integración puede requerir una corrección posterior; no se presenta la aceptación como garantía técnica o de seguridad. La guía de prueba sigue disponible para realizar esas comprobaciones. |

Tareas afectadas: **3.2, 3.3, 3.6, 3.7, 4.1, 5.5, 6.1–6.8, 7.2, 7.5, 7.7, 8.2 y 8.3**. Su implementación y evidencia parcial se conservan; no se cambia ninguna casilla pendiente a completada por esta autorización.

Las siete specs de la base pasan al inventario principal como contratos de comportamiento; su presencia allí no constituye una certificación del hardware. Este cierre sustituye la condición histórica de mantener el cambio activo hasta verificar todas sus casillas. Los apartados anteriores de ejecución se conservan como registro de lo observado en cada fecha.

Archivo: [cierre del cambio](../openspec/changes/archive/2026-11-10-sentinel-core-foundation/closure.md). Alcance de publicación: base compartida, sus specs y evidencias. Las propuestas y encargos de los siguientes módulos se mantienen como trabajo pendiente separado.

### Revalidación para publicación del cierre

Ejecutada el 2026-11-10 con Node24.13.1, SDK Android local y las versiones fijadas del proyecto:

| Comando o revisión | Resultado ejecutado |
| --- | --- |
| `npm run check:contracts` | PASS: 24 pruebas. |
| `npm run test:app` | PASS: 9 pruebas. |
| `npm run build` | PASS: contratos TypeScript y app TypeScript/Vite; aviso conocido de tamaño de bundle. |
| `python3 firmware/test/check_contracts.py` | PASS: núcleo host y paridad C++/AJV con fixtures compartidos. |
| `python3 firmware/test/check_config.py` | PASS: configuración efectiva y límites de particiones. |
| `pio run -d firmware -e sentinel_s3` | SUCCESS en 213.49 s: RAM estática 77368/327680 bytes e imagen 1183525/4194304 bytes. No se grabó una placa. |
| `npm run android:assemble`, con `ANDROID_HOME` y `ANDROID_SDK_ROOT` configurados según la guía | BUILD SUCCESSFUL: 124 tareas actualizadas. Avisos conocidos de flatDir y versión XML del SDK. No prueba instalación ni BLE físico. |
| `python3 -m unittest discover -s broker/scripts -p 'test_*.py'` | PASS: 6 pruebas. |
| `python3 broker/scripts/manage.py ps` | Broker real activo en Docker; TLS8883/WSS9001 enlazados solo a 127.0.0.1. No acredita acceso LAN. |
| `npm run check:wss -w app`, con la CA local mediante `NODE_EXTRA_CA_CERTS` | PASS: 3 casos contra el broker real: orden WSS→TLS, credenciales incorrectas/observador y revisión de secretos. No implica ejecución de la orden por ESP32. |
| `openspec validate --all --strict --json` | PASS local: siete specs principales y 23 propuestas futuras. Para esta publicación solo se incluyen las siete specs de base y su archivo; los 31 requisitos sincronizados coinciden con los deltas archivados. |
| Revisión del índice Git | 184 archivos de la base; enlaces locales válidos y sin claves privadas ni copias de las seis contraseñas locales revisadas. Las propuestas futuras, dependencias, compilaciones y material privado quedan fuera. |
| `git diff --cached --check`, reconociendo CRLF del wrapper Windows con `core.whitespace=blank-at-eol,blank-at-eof,space-before-tab,cr-at-eol` | PASS; se corrigieron espacios de final de línea en brief y configuración Gradle sin alterar comportamiento. |

La primera ejecución PlatformIO no pudo escribir su caché por el sandbox; se repitió autorizadamente y compiló. Android falló inicialmente por SDK no configurado y pasó al usar las variables documentadas. WSS falló dentro del sandbox y sus tres casos pasaron al repetir con acceso local autorizado. No se presentan esos intentos iniciales como PASS.

Los scripts npm mostraron resultados agregados por archivo en este entorno. Para comprobar los casos individuales se ejecutaron además `node --test --test-isolation=none contracts/test/contracts.test.mjs` (24/24 PASS) y `node --import tsx --test --test-isolation=none app/test/journal.test.ts app/test/store.test.ts` (9/9 PASS), usando Node24.13.1.

Se verificó antes de publicar que el cambio ya no está activo, el archivo conserva `.openspec.yaml` y las casillas siguen exactamente en **26 verificadas / 19 pendientes**. `develop`, ausente inicialmente del remoto, se creó desde el commit inicial de `origin/main`, sin reescribir historial. La revisión de otro integrante sigue pendiente de la PR.

## Seguimiento del 2026-11-10

El usuario autorizó utilizar teléfono, ESP32 y el emulador del SDK Windows `C:\Users\Jefry\AppData\Local\Android\Sdk`. Esta autorización sustituye el aplazamiento Android registrado en la ejecución inicial del 8 de noviembre. Se conserva el registro histórico de aquella ejecución.

La [guía de prueba](core-foundation-test-guide.md) reúne cableado, instalación, BLE, MQTT y recuperación. Al retomar, WSL carecía de puertos `ttyUSB`/`ttyACM` visibles y ADB Windows carecía de dispositivos conectados. El SDK contiene una imagen API36 Google Play x86_64 y WHPX está disponible. Se preparó un AVD local llamado `Sentinel_Foundation_API36` en `.cache/android-avd`, con RAM de 2048 MB y pantalla 1080×2400. Los archivos del AVD y sus logs quedan ignorados por Git.

El AVD arrancó con `sys.boot_completed=1`. `adb -s emulator-5554 install -r` instaló el APK debug y `am start -W -n org.sentinelnode.app/.MainActivity` abrió el dashboard. Se comprobó la pantalla inicial en Android y el recorrido hasta los formularios BLE y WSS. La captura de arranque está en `.cache/sentinel-android-initial.png`.

Al pulsar Buscar y conectar BLE, Android solicitó el permiso Dispositivos cercanos. Al denegarlo, la app mostró el diagnóstico de permisos, Bluetooth y ventana física. El resultado aporta evidencia parcial a 7.2 y 7.7. Para cerrarlas aún hacen falta el teléfono físico, la placa y el intercambio BLE real.

Mosquitto arrancó con el material privado existente y el adaptador WSS pasó 3/3 comprobaciones desde WSL. Se configuró `adb -s emulator-5554 reverse tcp:9001 tcp:9001` y se instaló la CA pública de laboratorio mediante Ajustes del emulador. El APK abrió una sesión WSS autenticada en `wss://localhost:9001/mqtt` con la cuenta de operador de `lab-node`. La app mostró Canal conectado y Esperando un dispositivo. La captura está en `.cache/sentinel-android-wss.png`. La contraseña se suministró al campo enmascarado mediante entrada virtual por stdin, sin argumentos ni registros con su valor.

La sesión del emulador aporta evidencia parcial a 7.7. La telemetría real de 7.5, el intercambio BLE y los controles físicos siguen pendientes. El acceso LAN de 5.5 requiere comprobar el broker desde un cliente en la red autorizada con su enlace LAN explícito.

El seguimiento OpenSpec continúa en 26/45 hasta completar todos los criterios de una tarea pendiente.

## Condiciones de la ejecución inicial del 2026-11-08

- Rama de trabajo: `feat/sentinel-core-foundation`; sin commits, push ni merges autorizados.
- Desarrollo en WSL; Node 24, JDK 21 y SDK Android están disponibles en el entorno.
- Se conservan las specs y el diseño aprobados. La base no implementa captura/detectores, HTTP/Telnet, NAT o notificaciones externas.
- **Por indicación posterior del usuario, no se utilizan dispositivos Android en esta fase.** Las comprobaciones de teléfono, bonding efectivo, BLE con el nodo e instalación/recorrido en teléfono permanecen pendientes, sin marcarse realizadas. La compilación web y Android no requiere conectar un teléfono y continúa dentro del alcance.
- No se observaron puertos serie ESP32 en la inspección inicial de `/dev`. Grabar, leer memoria real, OLED/botón y reinicios del equipo requieren hardware accesible y permanecen sin verificar hasta disponer de él.

## Estado de las comprobaciones

Solo los resultados anotados con un comando ejecutado y su resultado constituyen evidencia de esta ejecución. Se completó la implementación comprobable en este entorno; las validaciones físicas pendientes conservan sus casillas abiertas.

| Área | Comprobación prevista | Resultado |
| --- | --- | --- |
| OpenSpec | Validación estricta y seguimiento de tareas | Validación estricta PASS; artefactos aprobados sin cambios; solo tareas enteramente verificadas marcadas. |
| Contratos | Compilación, fixtures de esquemas y framing limitado | PASS: 24 pruebas finales, incluida paridad UTC, UTF-8/NUL, versiones, tipos heredados y BLE limitado. |
| Firmware | Compilación PlatformIO del entorno `sentinel_s3` y comprobaciones focalizadas | SUCCESS final; lógica host, sanitizers, paridad C++/AJV y configuración efectiva PASS. [Evidencia](../firmware/EVIDENCE.md). |
| Registro local | Pruebas de límites/recuperación y comprobación NVS real tras reinicio | Lógica de ring/paginación/recuperación PASS en host; NVS física tras reinicio pendiente de placa. |
| Broker | Configuración Compose, MQTT/TLS, WSS, negativos de autenticación/ACL y persistencia | Verificado en Docker real; evidencia en [broker/evidence.md](../broker/evidence.md). Acceso LAN/firewall pendiente. |
| Aplicación | Build web, pruebas de estado/historial y compilación Android | PASS: instalación limpia npm, nueve casos de app, Chromium/IndexedDB, build web, sync, assembleDebug y adaptador WSS real. |
| Teléfono Android | Pairing, permisos, BLE real e instalación/recorrido | Diferido por instrucción expresa del usuario. |
| Integración física | ESP32, OLED, botón, radio y reconexión real | Pendiente de hardware accesible; no sustituible por mocks. |

## Criterio de revisión final

1. Revisar cada requisito de las siete specs frente a archivos, comandos y evidencia correspondiente.
2. No usar un build web para justificar radio física, seguridad negociada BLE o funcionamiento OLED.
3. No usar una prueba simulada para afirmar entrega real MQTT/TLS/WSS ni persistencia del ESP32.
4. Mantener sin completar las tareas con verificaciones físicas pendientes o bloqueos reales, aun cuando exista su implementación compilable.
5. Registrar aquí los comandos finales, resultados y pendientes concretos antes de cerrar el objetivo de trabajo.

## Correcciones verificadas durante la integración

- `decodeMessage` selecciona validadores únicamente entre propiedades propias; las regresiones rechazan `constructor`, `__proto__`, `toString` y tipos no string.
- Contratos y cJSON se alinearon para UTF-8, NUL decodificado, límites ATT, formato UTC y URI/puerto. Las comprobaciones cubren entradas válidas e inválidas.
- Se corrigió y probó que un estado MQTT retenido de otro arranque no reemplace un estado BLE vivo.
- Las pruebas reales del broker acreditan por separado TLS, WSS, autenticación, ACL y persistencia; la CA compacta permite aprovisionar dentro del límite del contrato.
- Durante la integración de app se reprodujo pérdida de acceso local desde WSL: ambos engines Docker estaban inaccesibles. El responsable recuperó Docker Desktop y volvió a ejecutar satisfactoriamente los flujos TLS/WSS, negativos y persistencia. La selección de CLI ahora comprueba el daemon, con seis regresiones focalizadas. Véase la revalidación en [broker/evidence.md](../broker/evidence.md). La app debe acreditar separadamente su adaptador real; la prueba del broker no lo sustituye.
- Se corrigió y probó la recuperación de páginas con discontinuidades internas del registro: una pérdida declarada no produce un bucle de reconsulta y el cursor final requiere haber recibido su evento. Sin pérdida declarada se conserva la comprobación de continuidad.

## Resultados y pendientes finales

### Matriz de revisión contra las specs

Cada fila agrupa los requisitos de una spec; las comprobaciones de host cubren lógica y formatos, nunca el funcionamiento de radio o periféricos.

| Spec | Implementación revisada | Evidencia y límites |
| --- | --- | --- |
| `project-workspace` | Componentes separados; workspace privado app/contratos, versiones y lockfile; configuración privada ignorada; instrucciones de preparación; capacidades futuras explícitas. | README, `docs/development.md`, configuración raíz y documentación de cada componente. Instalación limpia npm, builds web/Android y compilación final PlatformIO acreditados. El recorrido físico sigue pendiente. |
| `message-contracts` | Esquemas y codec compartido; validación cJSON; órdenes con nodo/boot/revisión/vencimiento; caché acotada e identificadores; acuses accepted/applied/rejected; framing y limpieza por sesión. | `contracts/src`, `contracts/schemas/v1`, `contracts/test`, `firmware/components/sentinel/{core,wire,digest}.cpp` y pruebas de firmware. Revisados rechazo de tipos heredados, límites UTF-8 y paridad del formato. GATT efectivo requiere placa. |
| `device-runtime` | Identidad NVS y boot ID; supervisor serializado; liberación antes de activar otro perfil; restricciones de recursos; operaciones futuras rechazadas; OLED y botón. | `runtime.cpp`, `core.cpp`, `peripherals.cpp`, `platformio.ini` y pruebas focalizadas. Memoria detectada, arranque y controles físicos no se acreditan sin placa/OLED/botón. |
| `event-journal` | Ring NVS limitado, recuperación de slots, páginas/cursor/gap e IDs estables; errores sin borrado automático; replay y deduplicación en IndexedDB. | `core.cpp`, `runtime.cpp`, `app/src/journal.ts` y sus pruebas. Se corrigió incompatibilidad de gap interno. Persistencia NVS tras reinicio y recuperación por radio requieren hardware. |
| `device-connectivity` | BLE protegido con ventana física y passkey, SC/MITM/bonds; configuración privada RAM; STA/MQTT TLS en Cebo; Escudo sin asociación/MQTT; limpieza de sesiones y reconexión. | `ble.cpp`, `network.cpp`, `runtime.cpp` y configuración ESP-IDF. Revisión de fuente y compilación no prueban seguridad negociada, coexistencia de radios ni exclusión efectiva en placa; esas comprobaciones permanecen pendientes. |
| `secure-mqtt-broker` | Listeners TLS/WSS sin anónimo; roles y ACL por nodo; secretos separados; volumen persistente. | [Pruebas reales del broker](../broker/evidence.md), incluidos rechazos, ausencia de entregas prohibidas y persistencia. LAN/firewall desde cliente autorizado pendiente. |
| `mobile-control` | React/Capacitor y proyecto Android; adaptadores BLE/WSS reales; estado y acuses reconciliados; historial/cursor/gap; capacidades futuras no disponibles. | [Evidencia app/contratos](../app/evidence.md): builds, Chromium/IndexedDB, pruebas de estado y adaptador WSS contra broker real. Permisos, pairing, instalación y recorrido Android quedan diferidos por el usuario. UI con telemetría real de ESP32 pendiente de placa. |

La revisión global de Git examinó 175 archivos no ignorados, sin encontrar valores de las contraseñas generadas del laboratorio ni claves privadas. Los diez archivos privados del broker estaban ignorados y sin seguimiento; no se imprimieron sus valores. Arquitectura, decisiones técnicas y roadmap conservaron sus hashes aprobados. Esta revisión se consolida después de las entregas finales para cubrir archivos posteriores.

### Broker: entrega revisada

El trabajador del grupo 5 entregó [evidencia y comandos reproducibles](../broker/evidence.md). Se revisaron Compose, configuración, ACL, preparación privada y script de comprobación real. Las pruebas cubren ambos listeners TLS/WSS, identidades de nodo/operador/observador, denegaciones con ausencia de entrega no autorizada, CA/hostname, persistencia entre reinicios y comando de aprovisionamiento compatible con el contrato.

Tareas 5.1, 5.2, 5.3, 5.4 y 5.6 completadas con esa evidencia. **5.5 permanece sin marcar:** volumen, loopback y parámetro LAN están implementados; no hay IP/LAN autorizada concreta y cliente en esa red para acreditar alcance y firewall. Renderizar una IP documental no sustituye esa prueba. La configuración publica únicamente en loopback. Tras la entrega se reprodujo otra pérdida del daemon Docker, incluso en una comprobación del coordinador fuera del sandbox; no se afirma disponibilidad continua del laboratorio. La app conserva evidencia de una ejecución WSS real de tres casos satisfactoria y una repetición posterior fallida por timeout del broker, sin cambiar su adaptador.

En la revisión global inicial de archivos no ignorados no se encontraron copias de las credenciales generadas del laboratorio; los archivos privados están ignorados. Repetir la revisión al cerrar cuando hayan terminado de cambiar los demás componentes. No se imprimieron secretos.

### Comprobaciones adicionales del coordinador

Desde la raíz con Node24.13.1 en PATH:

| Comando | Resultado |
| --- | --- |
| `npm run check:contracts` | PASS: 23 casos. El primer intento no pudo escribir `contracts/dist` por la ruta canónica protegida del sandbox; la repetición autorizada fuera del sandbox pasó. |
| `npm run test:app` | PASS: ambos archivos de pruebas, incluidos reconciliación e historial con gap interno. |
| `npm run build` | PASS: contratos TypeScript y app TypeScript/Vite; advertencia informativa por tamaño de bundle, sin error. |
| `git diff --check` | PASS; la revisión de archivos nuevos se realiza por separado porque todavía no están añadidos a Git. |
| SHA256 de artefactos aprobados | PASS: proposal, design y las siete specs conservan los hashes anteriores a implementar. |
| Comprobación de enlaces locales | PASS: README y documentos de firmware, conectividad, desarrollo, app, broker, contratos y simulador resuelven sus archivos enlazados. |
| `python3 broker/scripts/manage.py ps` tras iniciar Docker Desktop | PASS: Mosquitto activo y puertos 8883/9001 publicados solo en 127.0.0.1. Se conservó el contenedor y su volumen. |
| `NODE_EXTRA_CA_CERTS=…/broker/.secrets/ca.crt npm run check:wss -w app` | PASS: tres casos reales del adaptador tras la recuperación; comando validado no retenido, credenciales incorrectas/observador y ausencia de secretos. La CA se suministró mediante ruta absoluta y no se deshabilitó TLS. |
| `python3 firmware/test/check_config.py` | PASS sobre la configuración efectiva final y los límites de particiones; no implica detección física. |
| `python3 firmware/test/check_contracts.py` con Node24 en PATH | PASS final: lógica portable del firmware, salida C++ validada por AJV y fixtures de comandos compartidos validados por cJSON después del ajuste UTC. |

La inspección de `/dev/ttyUSB*` y `/dev/ttyACM*` no encontró puertos candidatos. No se ejecutaron upload/monitor ni comandos de dispositivos Android. Se revisaron también manifiesto Android, configuración de confianza y Capacitor: tráfico claro/mixed content deshabilitados, CA de usuario solo para debug, sin logging de argumentos privados. No se añadieron servidores HTTP/Telnet, captura, escaneos activos o NAT a esta base.

Estas ejecuciones del coordinador preceden al último ajuste de paridad de fecha UTC entre firmware y contratos. El responsable de contratos acreditó después **24/24 comprobaciones de contratos, nueve casos de app, build web, sync y assembleDebug** satisfactorios; el calendario real y el formato UTC quedan alineados con firmware. Véase [app/evidence.md](../app/evidence.md).

La revisión de cierre examinó **180 archivos no ignorados**, incluido el informe final del firmware, sin passwords privados del laboratorio ni bloques de claves privadas. Los directorios generados quedaron excluidos y se conservan lockfile, Gradle Wrapper y configuración fuente. `git diff --check` y `openspec validate sentinel-core-foundation --strict` pasaron también después de actualizar las casillas finales. Esta evidencia, los hashes aprobados y la revisión de fuentes/contratos/enlaces completan **8.4**; no autoriza commits, publicación ni integración de ramas.

### Firmware: entrega final revisada

Se revisaron [firmware/EVIDENCE.md](../firmware/EVIDENCE.md), [firmware.md](firmware.md), [connectivity.md](connectivity.md), configuración efectiva y código de producción. La compilación final `pio run -d firmware -e sentinel_s3` terminó **SUCCESS en 116.29 s**, sin errores ni warnings, con RAM estática 77368/327680 bytes e imagen 1183525/4194304 bytes. Estas cifras de enlace no acreditan memoria física detectada ni consumo o autonomía.

El responsable ejecutó las pruebas host normales y con AddressSanitizer/UBSan, el cruce serializador C++→AJV/fixtures→cJSON y `check_config.py`, todos PASS. El coordinador verificó también la configuración generada: particiones dentro de 16 MiB, PSRAM octal, SC-only/nivel4, bonds y validación de fecha TLS obligatorios. La configuración no sustituye la lectura de memoria ni la seguridad BLE negociada en placa.

Se completaron 3.1, 3.4, 3.5, 4.2, 4.3 y 4.4. Los adaptadores ESP-IDF/NimBLE/NVS/SSD1306/esp-mqtt están implementados y compilados; no se sustituyeron por dobles de tests. El supervisor, la admisión de comandos pendientes, la limpieza de buffers privados y la recuperación limitada mantienen los contratos revisados.

### Estado histórico antes de la aceptación de archivo

**8.1** dispone de compilaciones finales de firmware, contratos, web y Android, instalación npm limpia desde el lockfile y configuración/paquetes PlatformIO fijados. Las fuentes necesarias, incluido el wrapper Android, permanecen fuera de las exclusiones; ninguna compilación exige credenciales reales incorporadas al repositorio. **8.5** conserva aquí entorno, comandos, resultados y limitaciones; la integración física no se da por realizada.

Quedan **19 tareas sin marcar**, todas implementadas pero con estas comprobaciones pendientes:

| IDs | Bloqueo real y comprobación restante |
| --- | --- |
| 3.2, 3.3, 3.6, 3.7 | Placa/OLED/botón no accesibles: grabación, consola, memoria física, identidad tras reinicio y controles/pantalla reales. |
| 4.1 | Placa no accesible: escritura NVS de evento real y lectura con mismo ID después de reiniciar. |
| 5.5 | Falta cliente e IP/LAN autorizada concreta para verificar alcance/firewall. Volumen y persistencia local ya comprobados. |
| 6.1–6.8 | Faltan placa y recorrido real de radio: anuncio, pairing/SC/MITM, GATT, aprovisionamiento/reloj, TLS/MQTT, órdenes y exclusión Cebo–Escudo. El uso de Android está expresamente diferido. |
| 7.2, 7.5, 7.7 | Uso de teléfono diferido y placa no accesible: permisos/BLE, telemetría real ESP32 en UI, instalación, confianza CA y recorrido Android. Build/sync/assemble y UI/IndexedDB en Chromium sí están comprobados. |
| 8.2, 8.3 | Recorrido integral ESP32/OLED/botón/teléfono y recuperación tras desconexión/reinicio: pendiente de esos equipos y canales reales. |

La ejecución inicial terminó con **26/45 tareas verificadas** y esos bloqueos documentados. Entonces el cambio permaneció activo; esta condición quedó sustituida por el cierre autorizado del 2026-11-10, que conserva abiertas las comprobaciones asumidas. El archivo no se presenta como íntegramente verificado. No se declara completo el MVP: detección, honeypot, Sanitizador y notificaciones siguen correspondiendo a sus cambios independientes, sin reducir su alcance obligatorio.

La ejecución inicial no realizó commits, push, merges ni borrado de ramas. La autorización posterior de cierre permite publicar esta base y abrir la PR hacia develop; el merge y la revisión de otro integrante siguen pendientes conforme a CONTRIBUTING.
