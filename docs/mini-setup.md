# Mini setup de Sentinel Node — Windows

Para preparar el proyecto sin WSL ni PlatformIO. El firmware se compila y sube con **Arduino IDE**. Para empezar con las pruebas BLE de P1 puedes omitir Mosquitto y la app web; necesitas el APK Android.

## 1. Tener lo básico

- ESP32-S3 N16R8, cable USB **de datos** y botón externo. OLED opcional para estas pruebas.
- Arduino IDE 2 y Git.
- Para la app: Node **24.13.1** y npm **11.8.0**.
- Para MQTT: Docker Desktop iniciado.
- Solo para generar el APK: JDK **21** configurado y Android SDK **36**, Build Tools **36**. Puedes instalarlos con Android Studio.

Abre PowerShell en la raíz del repositorio. Si tu copia está limpia, actualízala:

```powershell
git status --short --branch
git switch develop
git pull --ff-only origin develop
```

Si aparecen cambios locales, consérvalos antes de cambiar de rama. Para desarrollar, crea después la rama `feat/*` de [tu encargo](encargos/README.md).

## 2. Preparar Arduino una vez

1. En Arduino IDE, abre **Archivo → Preferencias** y agrega esta URL al gestor de placas, conservando las existentes:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. En el gestor de placas instala **esp32 de Espressif Systems, versión 3.3.12**.
3. Cierra Arduino IDE y ejecuta desde la raíz:

   ```powershell
   .\firmware\arduino\install-wolfmqtt.ps1
   .\firmware\arduino\install-board-config.ps1
   ```

   Esto instala wolfMQTT **2.2.0** y prepara el menú de particiones. No graba la placa ni instala otro SDK. Si PowerShell bloquea los scripts, ejecuta `Set-ExecutionPolicy -Scope Process Bypass` en esa terminal y repítelos.
4. Abre Arduino IDE y el archivo **`firmware\SentinelNode\SentinelNode.ino`** desde el repositorio completo; no copies solo el `.ino`.

En **Herramientas**, selecciona:

| Opción | Valor |
| --- | --- |
| Placa | **ESP32S3 Dev Module** del paquete oficial |
| Flash Size | **16MB** |
| Flash Mode | **QIO 80MHz** |
| PSRAM | **OPI PSRAM** |
| Partition Scheme | **Sentinel standard USB (4MiB factory, no OTA)** |
| USB CDC On Boot | **Disabled** |
| Puerto | El COM de la ESP32 |

Si falta la partición Sentinel, revisa la versión del core y repite el segundo instalador con el IDE cerrado.

## 3. Conectar y subir el firmware

Con la placa desconectada, conecta el **botón externo entre GPIO4 y GND**. No es el botón BOOT. Si tienes OLED SSD1306 **128×32**, conecta VCC a 3V3, GND a GND, SDA a GPIO8 y SCL a GPIO9.

**Si todavía no tienes OLED:** en `firmware\SentinelNode\SentinelConfig.h`, cambia únicamente `#define SENTINEL_SERIAL_TEST_DISPLAY 0` a `#define SENTINEL_SERIAL_TEST_DISPLAY 1`. El estado y el código BLE temporal saldrán por serie; el botón sigue siendo necesario. Conserva los flags Cebo/Sanitizador en `0` hasta integrar sus implementaciones.

1. Conecta la ESP32 por el conector **USB-UART**, selecciona su COM y pulsa **Verificar**.
2. Para la **primera carga con esta tabla**, selecciona **Erase All Flash Before Sketch Upload → Enabled** y pulsa **Subir**. Esto borra los datos anteriores; tendrás que emparejar de nuevo. Después vuelve a **Disabled**. Las actualizaciones USB pueden reinicializar datos; los reinicios/cortes normales deben conservarlos.
3. Abre **Monitor serie a 115200**. Debe arrancar en **Idle**. Cierra otros programas que ocupen el COM. Si falla la subida, revisa cable, conector y puerto antes de cambiar opciones.

No guardes capturas ni logs con el código de emparejamiento. Al disponer de OLED, vuelve la opción serie a `0` y comprueba la pantalla física. Más detalle: [firmware y cableado](firmware.md).

## 4. Levantar Mosquitto

Con Docker Desktop abierto, desde la raíz:

```powershell
docker compose -f broker/compose.yaml up -d
docker compose -f broker/compose.yaml ps
```

El servicio debe aparecer iniciado. No hay que crear usuarios ni certificados. Usa una LAN propia/de confianza; cualquier cliente con acceso al broker puede leer y publicar. Conserva el volumen: no uses `down -v`.

Con `ipconfig`, identifica la IPv4 del PC en la red que comparte con el teléfono/ESP32:

| Cliente | Dirección de ejemplo; sustituye la IP |
| --- | --- |
| Firmware, cuando use MQTT | `mqtt://192.168.1.10:1883` |
| App | `ws://192.168.1.10:9001/mqtt` |

Desde el teléfono, `localhost` no es tu PC. Si no conecta, revisa que la red permita comunicación entre clientes y que el firewall local permita 1883/9001. No abras puertos del router. **Escudo funciona por BLE y no necesita MQTT.**

## 5. Abrir la app web

Desde la raíz, una vez instalados Node/npm:

```powershell
npm ci
npm run build
npm run dev
```

Abre la URL que imprime Vite. Para MQTT introduce endpoint WS e ID del nodo, sin cuenta. La web permite revisar la UI y MQTT; **el BLE de este proyecto se prueba desde Android**.

## 6. Generar e instalar la app Android

Después de `npm ci`, con JDK21 y SDK36 disponibles, ejecuta desde la raíz. La ruta SDK mostrada es la habitual de Windows; ajústala si instalaste en otra ubicación:

```powershell
$env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
$env:VITE_SENTINEL_P1_TOOLS = "1"
npm run android:sync
Remove-Item Env:VITE_SENTINEL_P1_TOOLS
Push-Location app/android
.\gradlew.bat assembleDebug
Pop-Location
```

El flag incorpora **Diagnóstico P1** al APK para probar Escudo. Para generar después un APK sin ese panel, repite `android:sync` y el ensamblado sin establecer la variable.

Copia `app\android\app\build\outputs\apk\debug\app-debug.apk` al teléfono, instálalo y autoriza los permisos Android que solicite. Es una app empaquetada: no necesitas dejar Vite ejecutándose para usarla.

## 7. Primera prueba breve

1. Activa Bluetooth y abre la app. Mantén el botón externo de la ESP32 **3 segundos**: abre la ventana BLE de **120 segundos**.
2. Pulsa **Buscar y conectar BLE** e introduce las seis cifras de OLED o `[P1 PAIR CODE]` del monitor serie cuando Android las solicite.
3. Comprueba estado **Reposo (Idle)**, entra en **Escudo** y usa **Diagnóstico P1** para escanear redes propias de 2.4 GHz, elegir un objetivo e iniciar/detener captura. Revisa ACK y estado confirmado.

Esto permite comenzar las pruebas de la base/Escudo. Cebo, Sanitizador, pantallas finales y push siguen en los [encargos del equipo](encargos/README.md); no aparecerán operativos solo por levantar Docker y la app. Para el recorrido completo y registrar pendientes físicos sigue la [guía de pruebas P1](guia-pruebas-persona-1.md).
