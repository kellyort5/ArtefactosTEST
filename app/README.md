# Aplicación Sentinel Node

App real React19/Vite7/Tailwind4/Zustand5/TypeScript5, mqtt.js5 por WS y Capacitor8/BLE8 para Android. Estado, comandos y eventos pasan por los contratos AJV de `@sentinel/contracts`. No contiene respuestas de nodo simuladas ni credenciales de laboratorio. El adaptador común reconoce base v1 y módulos v2, con navegación, estado, comandos e historial comunes. Los catálogos/controles específicos, gráficos, formularios Sanitizador y notificaciones siguen en sus encargos independientes; este cambio no los sustituye.

## Compilar sin dispositivo

Desde la raíz, Node24.13.1/npm11.8.0:

```sh
npm ci
npm run check:contracts
npm run test:app
npm run build
npm run dev
```

Android requiere JDK21 y Android SDK36 con Build Tools36:

```sh
export ANDROID_HOME=/home/jefry/Android/Sdk
export ANDROID_SDK_ROOT="$ANDROID_HOME"
npm run android:sync
npm run android:assemble
```

En Windows usar variables/rutas de su instalación y `app\android\gradlew.bat assembleDebug` desde `app\android`. El wrapper fuente usa Gradle8.14.3; AGP8.13.0. El APK generado es `app/android/app/build/outputs/apk/debug/app-debug.apk`, ignorado; no versionarlo. También están ignorados assets web copiados por Capacitor, cachés y plugins Cordova regenerados. Versionar el resto del proyecto nativo, incluido wrapper JAR y configuración de seguridad.

**Uso de Android autorizado por el usuario.** Copiar el APK de debug al teléfono e instalarlo desde su gestor de archivos autorizando esa fuente, o usar ADB con el dispositivo seleccionado expresamente. Revisar permisos de dispositivos cercanos y abrir la app. El SDK Windows indicado por el usuario es `C:\Users\Jefry\AppData\Local\Android\Sdk`. La [guía de prueba](../docs/core-foundation-test-guide.md) describe el recorrido y el [informe integral](../docs/core-foundation-check.md) registra los resultados del emulador y del equipo físico por separado.

## Conexión LAN sin certificados

Instalar el APK en el teléfono real. Preparar el [broker](../broker/README.md) con cuentas por nodo y usar la IP LAN del PC: `ws://192.168.1.10:9001/mqtt` (ejemplo). No usar localhost desde teléfono; no instalar CA ni configurar SAN/DNS/router. PC, teléfono y ESP32 deben poder comunicarse en la red de confianza autorizada. El firewall local debe permitir Docker/puertos1883 y9001. Una Wi-Fi con aislamiento de clientes requiere otra red o un punto de acceso disponible.

Capacitor sirve los assets empaquetados desde `http://localhost`; no carga un sitio remoto. Android permite tráfico sin cifrar para el broker configurado y mantiene CA del sistema para HTTPS. `allowMixedContent` permanece deshabilitado: el origen local HTTP evita contenido mixto para WS. Los controles BLE nativos siguen protegidos. Web de desarrollo: abrir Vite por HTTP; una página alojada por HTTPS puede bloquear WS y no es el despliegue de este MVP.

## Flujo BLE real (pendiente de verificación física)

1. Arrancar el ESP32 con OLED y botón configurados; mantener3 s para abrir ventana120 s. Sin OLED no debe permitirse pairing nuevo salvo el modo serie temporal explícito de pruebas P1 documentado al final.
2. Pulsar **Buscar y conectar BLE**. La app pide permisos Android, comprueba Bluetooth, filtra el UUID de servicio compartido v1/v2 y crea bond si falta. Introducir el passkey OLED en el diálogo del sistema. Firmware exige LE Secure Connections/MITM: un bond por sí solo no demuestra seguridad; comprobar nivel real en firmware al hacer el recorrido.
3. La app lee metadatos protegidos, suscribe indicaciones y solicita get_state/read_events. Escribe con respuesta y framing MTU23; no requiere negociar MTU grande. Limpia ensamblados al desconectar/expirar10 s y serializa mensajes completos.
4. Consultar estado y sincronizar hora UTC por BLE si se desean fechas UTC. Enviar SSID/contraseña, URI `mqtt://IP-DEL-PC:1883` y usuario/contraseña del nodo, sin CA en **Conectividad del dispositivo**. Todo el mensaje debe caber2048 bytes. Los campos de contraseña se limpian al enviar y todo el formulario se restablece al terminar, incluso si falla. El firmware actual persiste la configuración autorizada en NVS después de validarla; la app mantiene sus credenciales de sesión solo en RAM. MQTT conecta también con UTC desconocido; la app puede sincronizar hora para las fechas de eventos. Ver [política de persistencia y USB](../docs/persistence-policy.md).
5. Elegir solo un perfil anunciado como disponible: la base v1 prepara recursos sin implementar detectores ni HTTP/Telnet; un nodo v2 anuncia sus capacidades y estado funcional real. Observar ACK applied y estado/revisión; accepted no significa aplicado. Escudo detiene asociación/MQTT y conserva BLE de control; su captura y detectores dependen del firmware instalado, no del botón de cambio de perfil.
6. Ante permisos denegados/Bluetooth apagado, la app muestra instrucciones de permisos/ventana/OLED; habilitarlos desde ajustes y volver a conectar. Ante interrupción, consultar estado por el canal autorizado disponible; no reenviar automáticamente órdenes vencidas ni simular éxito.

## Flujo WS

Introducir endpoint `ws://IP-DEL-PC:9001/mqtt`, ID de nodo, rol y su cuenta `operator-<id>` o `viewer-<id>`. La cuenta debe estar autorizada por ACL del broker; el ID de cliente no concede permisos. Credenciales quedan solo en RAM mientras dure la sesión y se eliminan al desconectar/cerrar. La app no usa localStorage, variables VITE ni IndexedDB para credenciales.

Un estado retenido aislado se muestra antiguo. Esperar el state vivo periódico del nodo (5 s) antes de ordenar; 15 s sin actualización o sin canal marca estado antiguo. Observador es solo lectura. Para configurar/red/hora se exige BLE. Al reconectar se obtiene snapshot y se puede **Recuperar por cursor**; el observador recibe eventos publicados pero no solicita páginas porque carece de permiso commands. Tras una desconexión WS, volver a conectar suministrando la cuenta; no hay cola offline de órdenes.

No confirmar aplicación por PUBACK o por resolución de publishAsync. El store conserva resultados sanitizados de32 órdenes y reconcilia por ack o state/last_command_id/revisión, incluso si el canal cambia. Un comando de boot anterior permanece sin confirmar tras reboot. Configurar un perfil preparado no implica detector/honeypot disponible.

## Versiones, capacidades y uso por las pantallas

`protocol.ts` reutiliza `decodeAnyMetadata`/`decodeAnyMessage`, `makeCommand` (v1), `makeModuleCommand` (v2) y `checkModuleAckForCommand`. BLE mantiene UUID y framing compartidos. La versión de salida procede de Metadata protegida o state vivo; un retained nunca negocia versión ni confirma un comando. Se rechazan versiones desconocidas, topics/envelopes incoherentes y respuestas de otro nodo/arranque. No se cambia un ID histórico ni se convierte un evento v1 a v2.

MQTT suscribe `sentinel/v{1,2}/nodes/<id>/{state,availability,metrics,events,acks}` y publica commands en la versión negociada. Conserva compatibilidad si una ACL solo permite v1, siempre que permita su state. **Dependencia de integración:** la plantilla actual del broker solo concede v1; Persona 4 debe completar ACL v2/roles en `sentinel-broker-lan`. Suscribirse a v2 no demuestra autorización ni entrega real. Escudo sigue solo por BLE; MQTT de control requiere estado vivo de Cebo y rol operador. No hay asociación Wi-Fi adicional desde el Escudo.

La navegación General/Escudo/Sanitizador/Cebo usa `modeAvailable`; cada módulo muestra el estado confirmado del mismo arranque y sus capacidades. Un nodo v1 conserva los controles de recursos de base, sin presentar módulos v2 como funcionales. Un módulo ausente queda no disponible. La sección común no implementa catálogo Escudo, reglas DNS, gráficos ni controles específicos: corresponden a `sentinel-app-escudo`, `sentinel-app-sanitizador` y `sentinel-app-cebo` de P4.

Para esas pantallas, reutilizar `send(NodeRequest, canal?)`, `recover`, `nodeStore` y `eventSummary`; no crear transportes o formatos nuevos. `send` selecciona codec y revisa capacidades/canal; devolver un ID indica entrega pendiente, nunca éxito. Las operaciones `escudo.*`, `sanitizador.*`, `configure_connectivity` y `set_time` requieren BLE protegido. El estado mantiene solo correlación pública; las configuraciones privadas no conservan argumentos en el historial. Los resultados ACK tipados se correlacionan con la operación original: solo `kind:events` completa una página. Métricas de arranques anteriores no reemplazan las actuales.

## Historial y pruebas

IndexedDB conserva256 eventos/nodo, deduplica `(device_id,message_id)`, guarda cursor de páginas completadas y discontinuidad persistente. Eventos en vivo no adelantan cursor; una página incompleta conserva cursor anterior y puede repetirse con ID nuevo. Replay se pagina8; las páginas siguientes solo se piden después de escribir los eventos recibidos. Un gap reportado admite huecos internos de slots inválidos; se exige recibir el evento de cursor final antes de avanzar. Sin gap se comprueba continuidad completa. Dispositivo conserva64 registros; no hay historial ilimitado.

```sh
npm run check:contracts
npm run test:app
# Con Vite activo en otra terminal:
npm exec -w app -- playwright install chromium
npm run check:browser -w app
# Con el broker local preparado y accesible:
npm run check:mqtt -w app
npm run check:browser:mqtt -w app
```

Los tests focalizados comprueban negociación, retención/boot/revisión, ACK rechazados o de tipo incorrecto, pérdida de transporte, páginas incompletas, historial v1/v2 de tres módulos y limpieza de credenciales de sesión. Chromium comprueba UI inicial, diagnóstico BLE web, navegación v2 y rechazo sin cambio optimista con fixtures de prueba, responsive e IndexedDB real tras recarga. WS comprueba adaptador, operador/observador y rechazo de contraseña contra Mosquitto real; usa un comando de prueba sin ESP32 y no acredita aplicación en placa. [evidence.md](evidence.md) conserva la evidencia de la base; la [verificación de app v2](../openspec/changes/archive/2026-11-12-sentinel-app-protocolo-v2/verification.md) registra este incremento y sus pruebas físicas pendientes. API de transporte/formatos exactos en [contracts/README.md](../contracts/README.md). Los fixtures solo existen en tests; nunca alimentan la UI de producto.

## Diagnóstico optativo de Persona 1

Para probar firmware Escudo antes de las pantallas finales: `VITE_SENTINEL_P1_TOOLS=1 npm run android:sync` desde la raíz y después `npm run android:assemble`. El panel usa el mismo BLE protegido, metadatos frescos, contratos v2 y journal; permite catálogo paginado, objetivo, captura, estado, historial y negativos públicos explícitos. Sin BLE permanece deshabilitado. Enviar no significa aplicado: revisar ACK correlacionado. No introducir credenciales en el panel. Su memoria de respuestas es acotada y temporal.

El valor por defecto está desactivado; para un APK de producto ejecutar `npm run android:sync` sin esa variable y ensamblar de nuevo. El flag se incorpora durante build web, no es una opción que pueda activarse en el teléfono. La [guía P1](../docs/guia-pruebas-persona-1.md) explica salida serie temporal sin OLED y el recorrido completo; no acredita OLED, ataques en aire ni módulos ausentes.
