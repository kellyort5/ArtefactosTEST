# Aplicación Sentinel Node

App real React19/Vite7/Tailwind4/Zustand5/TypeScript5, mqtt.js5 por WSS y Capacitor8/BLE8 para Android. Estado, comandos y eventos pasan por los contratos AJV de `@sentinel/contracts`. No contiene respuestas de nodo simuladas ni credenciales de laboratorio. Módulos Escudo/Cebo, Sanitizador y canales externos siguen pendientes.

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
2. Pulsar **Buscar y conectar BLE**. La app pide permisos Android, comprueba Bluetooth, filtra servicio v1 y crea bond si falta. Introducir el passkey OLED en el diálogo del sistema. Firmware exige LE Secure Connections/MITM: un bond por sí solo no demuestra seguridad; comprobar nivel real en firmware al hacer el recorrido.
3. La app lee metadatos protegidos, suscribe indicaciones y solicita get_state/read_events. Escribe con respuesta y framing MTU23; no requiere negociar MTU grande. Limpia ensamblados al desconectar/expirar10 s y serializa mensajes completos.
4. Consultar estado y sincronizar hora UTC mediante BLE. Enviar SSID/contraseña, mqtts URI/usuario/contraseña de nodo y CA pública en **Conectividad del dispositivo**. Todo el mensaje debe caber2048 bytes. Los campos de contraseña se limpian al enviar y el nodo guarda secretos solo en RAM.
5. Elegir Cebo: inicia recursos STA/MQTT reales según firmware, sin HTTP/Telnet. Observar ack applied y estado/revisión; accepted no significa aplicado. Escudo detiene asociación/MQTT y conserva BLE de control, sin captura/detección.
6. Ante permisos denegados/Bluetooth apagado, la app muestra instrucciones de permisos/ventana/OLED; habilitarlos desde ajustes y volver a conectar. Ante interrupción, consultar estado por el canal autorizado disponible; no reenviar automáticamente órdenes vencidas ni simular éxito.

## Flujo WSS

Introducir endpoint `wss://hostname:9001/mqtt`, ID de nodo, rol y su cuenta `operator-<id>` o `viewer-<id>`. La cuenta debe estar autorizada por ACL del broker; el ID de cliente no concede permisos. Credenciales quedan solo en RAM mientras dure la sesión y se eliminan al desconectar/cerrar. La app no usa localStorage, variables VITE ni IndexedDB para credenciales.

Un estado retenido aislado se muestra antiguo. Esperar el state vivo periódico del nodo (5 s) antes de ordenar; 15 s sin actualización o sin canal marca estado antiguo. Observador es solo lectura. Para configurar/red/hora se exige BLE. Al reconectar se obtiene snapshot y se puede **Recuperar por cursor**; el observador recibe eventos publicados pero no solicita páginas porque carece de permiso commands. Tras una desconexión WSS, volver a conectar suministrando la cuenta; no hay cola offline de órdenes.

No confirmar aplicación por PUBACK o por resolución de publishAsync. El store conserva resultados sanitizados de32 órdenes y reconcilia por ack o state/last_command_id/revisión, incluso si el canal cambia. Un comando de boot anterior permanece sin confirmar tras reboot. Configurar un perfil preparado no implica detector/honeypot disponible.

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

Chromium comprueba UI inicial, diagnóstico BLE web, responsive y IndexedDB real tras recarga. WSS comprueba adaptador, operador/observador y rechazo de contraseña contra Mosquitto real; usa un comando de prueba sin ESP32 y no acredita aplicación en placa. [evidence.md](evidence.md) identifica resultados y tareas diferidas. API de transporte/formatos exactos en [contracts/README.md](../contracts/README.md). Los fixtures solo existen en tests; nunca alimentan la UI de producto.
