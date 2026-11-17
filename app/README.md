# Aplicación Sentinel Node

App real React19/Vite7/Tailwind4/Zustand5/TypeScript5, mqtt.js5 por WSS y Capacitor8/BLE8 para Android. Estado, comandos y eventos pasan por los contratos AJV de `@sentinel/contracts`. No contiene respuestas de nodo simuladas ni credenciales de laboratorio. El adaptador común reconoce base v1 y módulos v2, con navegación, estado, comandos e historial comunes. Los catálogos/controles específicos, gráficos, formularios Sanitizador y notificaciones siguen en sus encargos independientes; este cambio no los sustituye.

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

## Confianza CA de laboratorio

Preparar primero el [broker](../broker/README.md) con hostname/IP LAN en SAN, cuentas node/operator/viewer y CA pública compacta. `localhost` desde teléfono se refiere al teléfono; usar el endpoint de la LAN autorizada y el firewall del host. No exponer el router.

La app web confía en el almacén del navegador/sistema: instalar **solo** `broker/.secrets/ca.crt` como CA confiable mediante el procedimiento de su sistema. No copiar CA privada, claves de servidor ni clients.json al bundle. Navegador y Android deben tener fecha correcta y nombre coincidente con SAN.

Android debug incluye `network_security_config.xml`: confía en CA del sistema y, únicamente cuando debuggable=true, CA instaladas por el usuario. Cuando se use un teléfono autorizado, transferir solo la CA pública, instalarla en Ajustes → Seguridad → Cifrado y credenciales → Instalar certificado → CA (nombres varían según fabricante). Reiniciar la app y conectar WSS con hostname del certificado. Release conserva CA de sistema; requiere una cadena confiable o una decisión posterior explícita de distribución de la CA pública. No hay trust-all, bypass de fecha/hostname, tráfico claro ni mixed content.

Capacitor `loggingBehavior:none` evita registrar argumentos GATT privados incluso en debug; depuración WebView deshabilitada y backups Android deshabilitados. La app y el plugin no añaden logs de payloads ni campos de credenciales.

## Flujo BLE real (pendiente de verificación física)

1. Arrancar el ESP32 con OLED y botón configurados; mantener3 s para abrir ventana120 s. Sin OLED no debe permitirse pairing nuevo.
2. Pulsar **Buscar y conectar BLE**. La app pide permisos Android, comprueba Bluetooth, filtra el UUID de servicio compartido v1/v2 y crea bond si falta. Introducir el passkey OLED en el diálogo del sistema. Firmware exige LE Secure Connections/MITM: un bond por sí solo no demuestra seguridad; comprobar nivel real en firmware al hacer el recorrido.
3. La app lee metadatos protegidos, suscribe indicaciones y solicita get_state/read_events. Escribe con respuesta y framing MTU23; no requiere negociar MTU grande. Limpia ensamblados al desconectar/expirar10 s y serializa mensajes completos.
4. Consultar estado y sincronizar hora UTC mediante BLE. Enviar SSID/contraseña, mqtts URI/usuario/contraseña de nodo y CA pública en **Conectividad del dispositivo**. Todo el mensaje debe caber2048 bytes. Los campos de contraseña se limpian al enviar y todo el formulario se restablece al terminar, incluso si falla. El firmware actual persiste la configuración autorizada en NVS después de validarla; la app mantiene sus credenciales de sesión solo en RAM. Al reiniciar el nodo se requiere una referencia UTC vigente. Ver [política de persistencia y USB](../docs/persistence-policy.md).
5. Elegir solo un perfil anunciado como disponible: la base v1 prepara recursos sin implementar detectores ni HTTP/Telnet; un nodo v2 anuncia sus capacidades y estado funcional real. Observar ACK applied y estado/revisión; accepted no significa aplicado. Escudo detiene asociación/MQTT y conserva BLE de control; su captura y detectores dependen del firmware instalado, no del botón de cambio de perfil.
6. Ante permisos denegados/Bluetooth apagado, la app muestra instrucciones de permisos/ventana/OLED; habilitarlos desde ajustes y volver a conectar. Ante interrupción, consultar estado por el canal autorizado disponible; no reenviar automáticamente órdenes vencidas ni simular éxito.

## Flujo WSS

Introducir endpoint `wss://hostname:9001/mqtt`, ID de nodo, rol y su cuenta `operator-<id>` o `viewer-<id>`. La cuenta debe estar autorizada por ACL del broker; el ID de cliente no concede permisos. Credenciales quedan solo en RAM mientras dure la sesión y se eliminan al desconectar/cerrar. La app no usa localStorage, variables VITE ni IndexedDB para credenciales.

Un estado retenido aislado se muestra antiguo. Esperar el state vivo periódico del nodo (5 s) antes de ordenar; 15 s sin actualización o sin canal marca estado antiguo. Observador es solo lectura. Para configurar/red/hora se exige BLE. Al reconectar se obtiene snapshot y se puede **Recuperar por cursor**; el observador recibe eventos publicados pero no solicita páginas porque carece de permiso commands. Tras una desconexión WSS, volver a conectar suministrando la cuenta; no hay cola offline de órdenes.

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
# Con el broker local preparado y accesible, CA sin bypass TLS:
NODE_EXTRA_CA_CERTS="$PWD/broker/.secrets/ca.crt" npm run check:wss -w app
```

Los tests focalizados comprueban negociación, retención/boot/revisión, ACK rechazados o de tipo incorrecto, pérdida de transporte, páginas incompletas, historial v1/v2 de tres módulos y limpieza de credenciales de sesión. Chromium comprueba UI inicial, diagnóstico BLE web, navegación v2 y rechazo sin cambio optimista con fixtures de prueba, responsive e IndexedDB real tras recarga. WSS comprueba adaptador, operador/observador y rechazo de contraseña contra Mosquitto real; usa un comando de prueba sin ESP32 y no acredita aplicación en placa. [evidence.md](evidence.md) conserva la evidencia de la base; la [verificación de app v2](../openspec/changes/archive/2026-11-12-sentinel-app-protocolo-v2/verification.md) registra este incremento y sus pruebas físicas pendientes. API de transporte/formatos exactos en [contracts/README.md](../contracts/README.md). Los fixtures solo existen en tests; nunca alimentan la UI de producto.
