# Prueba de sentinel-core-foundation

Guía para comprobar la base con ESP32-S3 N16R8, OLED SSD1306, botón y aplicación Android. El usuario autorizó utilizar el teléfono y la placa. También está disponible el SDK Android de Windows en `C:\Users\Jefry\AppData\Local\Android\Sdk` para las comprobaciones del emulador.

Los resultados y tareas verificadas se registran en [core-foundation-check.md](core-foundation-check.md). Consultar [firmware.md](firmware.md), [connectivity.md](connectivity.md) y [app/README.md](../app/README.md) para los detalles de cada componente.

## 1. Conectar pantalla y botón

Realizar el cableado con la placa desconectada de la alimentación.

| Componente | ESP32 |
| --- | --- |
| OLED VCC | 3V3 |
| OLED GND | GND |
| OLED SDA | GPIO8 |
| OLED SCL | GPIO9 |
| Botón externo | Entre GPIO4 y GND, conectado solo al presionarlo |

El firmware configura pull-up interno para el botón. La pantalla prevista es SSD1306 I2C de 128×32, dirección `0x3C`. Confirmar modelo y resolución antes de probar otra pantalla. Los pines son los valores iniciales del cambio y están pendientes de comprobación física.

## 2. Preparar la terminal WSL

```sh
cd /mnt/c/Users/Jefry/Documents/Arduino/ArtefactosProyecto
export PATH="/home/jefry/.nvm/versions/node/v24.13.1/bin:/home/jefry/.platformio/penv/bin:$PATH"
node --version
pio --version
```

Se esperan Node 24.13.1 y PlatformIO Core 6.2.0.

## 3. Compilar y localizar la placa

```sh
pio run -d firmware -e sentinel_s3
pio device list
```

La compilación debe terminar en `SUCCESS`. Conectar la placa mediante un cable USB de datos al conector USB-UART para utilizar la consola UART0 configurada. Identificar su puerto real en la lista.

Si Windows detecta un COM y WSL carece del dispositivo, resolver el acceso USB entre ambos entornos antes de grabar. `PUERTO_REAL` en los comandos siguientes es un marcador que se debe sustituir.

## 4. Grabar y comprobar el arranque

```sh
pio run -d firmware -e sentinel_s3 -t upload --upload-port PUERTO_REAL
pio device monitor -d firmware -e sentinel_s3 --port PUERTO_REAL --baud 115200
```

Presionar RESET con el monitor abierto. Registrar el `device_id`, `boot_id`, perfil inicial y memoria detectada. Se esperan flash de 16777216 bytes, PSRAM de 8388608 bytes y perfil `idle`. La OLED debe mostrar perfil y conectividad.

Ante un diagnóstico de memoria o pantalla, resolverlo antes de continuar con el emparejamiento.

## 5. Probar los controles físicos

1. Pulsar brevemente el botón externo y soltarlo. Desde `idle` debe seleccionar `escudo`.
2. Pulsar brevemente otra vez. Debe seleccionar `cebo` y reflejarlo en OLED.
3. Mantener el botón tres segundos y soltarlo. Debe abrir la ventana de emparejamiento durante 120 segundos y conservar el perfil al soltar.

La conexión de Cebo permanece pendiente hasta aprovisionar la red y el broker.

## 6. Instalar la aplicación Android

El APK de debug se genera en `app/android/app/build/outputs/apk/debug/app-debug.apk`. Para reconstruirlo, usar Node24, JDK21 y un SDK con Platform36/Build Tools36 configurado en el entorno.

```sh
npm ci
npm run android:sync
npm run android:assemble
```

Copiar el APK al teléfono e instalarlo desde su gestor de archivos, autorizando esa fuente cuando Android lo solicite. Para un emulador iniciado, ADB permite instalar el mismo APK seleccionando expresamente su identificador.

Abrir Sentinel Node. Debe aparecer la pantalla Estado del nodo, los formularios de conexión, los controles y el historial. Al iniciar sin conexión aparece Esperando un dispositivo.

## 7. Emparejar por BLE con la placa

1. Activar Bluetooth en el teléfono.
2. Abrir la ventana física de 120 segundos manteniendo el botón de la placa tres segundos.
3. Pulsar Buscar y conectar BLE en Sentinel Node y conceder los permisos solicitados.
4. Seleccionar el nodo e introducir el código mostrado en OLED durante el emparejamiento.
5. Comprobar que la app muestra identidad, perfil, revisión y transportes reales.

Registrar el resultado físico y comprobar la seguridad negociada para cerrar las tareas correspondientes. Una comprobación de interfaz en emulador acredita únicamente lo que se haya ejecutado en ese entorno.

## 8. Probar órdenes y eventos por BLE

Ejecutar cada acción después de recibir la confirmación de la anterior.

1. Consultar estado.
2. Seleccionar Reposo, Escudo y Cebo, comprobando coincidencia con OLED.
3. Sincronizar hora por BLE.
4. Comprobar los resultados `applied` y los eventos de sistema.
5. Revisar las capacidades reportadas. Captura, detectores, HTTP/Telnet y notificaciones siguen pendientes de sus cambios.

## 9. Preparar MQTT para el equipo real

Obtener el `device_id` de la placa y la IPv4 de la computadora en la red autorizada. En Windows, `ipconfig` permite identificar la dirección del adaptador correspondiente.

El laboratorio de la implementación inicial utiliza `lab-node` y SAN `localhost`/`127.0.0.1`. La conexión desde la placa requiere cuentas para su ID real, certificado con la IP o DNS de la computadora en SAN, enlace LAN explícito y firewall para los clientes autorizados. Conservar el material privado existente mientras se prepara ese entorno siguiendo [broker/README.md](../broker/README.md).

Suministrar las cuentas mediante el archivo privado de laboratorio y los formularios de la app. Guardar contraseñas y claves en las rutas privadas ignoradas, y compartir únicamente la CA pública cuando se configure confianza Android.

## 10. Aprovisionar y comprobar Cebo

1. Sincronizar la hora mediante BLE y esperar el resultado aplicado.
2. En Conectividad del dispositivo, introducir SSID de 2.4 GHz, contraseña, URI `mqtts://HOST_REAL:8883`, cuenta de nodo y CA pública PEM.
3. Enviar configuración protegida y esperar `applied`.
4. Seleccionar Cebo. Esperar Wi-Fi y MQTT conectados.
5. Configurar en la app `wss://HOST_REAL:9001/mqtt`, el mismo ID de nodo y su cuenta operator o viewer. Android debe confiar en la CA pública del laboratorio.
6. Esperar un estado vivo y comprobar eventos y revisión.
7. Para probar órdenes MQTT, desconectar BLE en la app, usar la cuenta operator y consultar estado. Comprobar el ack aplicado.
8. Reconectar BLE autorizado antes de pedir Escudo. Al aplicar Escudo, STA/MQTT deben detenerse y el estado debe poder reconciliarse por BLE.

`HOST_REAL` es el hostname o la IP incluido en el certificado del laboratorio. En una placa o teléfono, `localhost` designa ese dispositivo.

## 11. Recuperar eventos después de desconexión y reinicio

1. Desconectar los canales de la app y producir una transición mediante el botón.
2. Reconectar BLE y pulsar Recuperar por cursor.
3. Comprobar que el evento aparece una vez.
4. Reiniciar la placa y reconectar BLE. Comprobar identidad estable, boot ID nuevo y recuperación de los eventos persistidos con sus IDs originales.
5. Repetir una recuperación y comprobar deduplicación.
6. Aprovisionar otra vez hora y conectividad antes de volver a probar MQTT, porque esa configuración se conserva en RAM.

## 12. Registrar resultados

Anotar fecha, entorno, placa, pantalla, puerto, versión Android, comandos ejecutados y resultados observados en [core-foundation-check.md](core-foundation-check.md). Mantener pendientes las verificaciones que requieran otro equipo o un canal todavía inaccesible. Las casillas OpenSpec se completan cuando se cumple toda la comprobación descrita en la tarea.

## 13. Repetir la comprobación en el emulador

El dispositivo virtual local **Sentinel_Foundation_API36** utiliza la imagen API36 Google Play x86_64 del SDK Windows. Su configuración y datos se guardan en la carpeta ignorada `.cache/android-avd` de este equipo. Si se reproduce la prueba en otro equipo, crear un dispositivo virtual con una imagen API compatible en Device Manager. [Administración de dispositivos virtuales](https://developer.android.com/studio/run/managing-avds.html)

Desde PowerShell en la raíz del proyecto, iniciar el dispositivo virtual si todavía está apagado.

```powershell
$env:ANDROID_AVD_HOME = "$PWD\.cache\android-avd"
& "$env:LOCALAPPDATA\Android\Sdk\emulator\emulator.exe" -avd Sentinel_Foundation_API36 -port 5554 -no-snapshot -no-audio
```

En otra terminal PowerShell, desde la misma raíz, comprobar el arranque e instalar la aplicación.

```powershell
$SentinelAdb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
& $SentinelAdb -s emulator-5554 shell getprop sys.boot_completed
& $SentinelAdb -s emulator-5554 install -r .\app\android\app\build\outputs\apk\debug\app-debug.apk
& $SentinelAdb -s emulator-5554 shell am start -W -n org.sentinelnode.app/.MainActivity
```

El indicador de arranque debe ser `1` antes de instalar. Comprobar el dashboard inicial y pulsar Buscar y conectar BLE. Denegar Dispositivos cercanos debe producir un mensaje de permisos accionable. Usar teléfono y placa para el código OLED, bonding y tráfico GATT real. [Limitaciones de hardware del emulador](https://developer.android.com/studio/run/advanced-emulator-usage)

Para WSS local, iniciar el broker con sus certificados vigentes y reenviar el puerto de la máquina Windows hacia este dispositivo virtual.

```powershell
& $SentinelAdb -s emulator-5554 reverse tcp:9001 tcp:9001
```

Instalar solo la CA pública del laboratorio en los ajustes del dispositivo virtual y utilizar `wss://localhost:9001/mqtt`, ID `lab-node` y una cuenta autorizada. Abrir el archivo privado de cuentas únicamente en un editor local e introducir la contraseña en la app. Una conexión WSS al broker muestra Canal conectado. El estado del ESP32 aparece cuando se publica telemetría real del nodo correspondiente. El reenvío ADB comprueba el acceso local de Android, y la conexión desde la LAN se verifica por separado.
