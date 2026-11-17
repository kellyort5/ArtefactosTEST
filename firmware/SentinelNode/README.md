# SentinelNode con Arduino IDE Windows

Aplicar la [política vigente de persistencia y USB](../../docs/persistence-policy.md): primera carga de la nueva tabla con **Erase All Flash Before Sketch Upload: Enabled** desde el IDE; pérdida de datos anteriores y nuevo emparejamiento/configuración, sin migración. Después volver a Disabled si no se solicita borrado total; USB no garantiza conservación. Reinicios/cortes normales sí deben conservar datos confirmados. El estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md); las pruebas físicas siguen pendientes y los logs anteriores son históricos.

Abrir `SentinelNode.ino` en Arduino IDE 2.3.10. Usar **esp32 de Espressif Systems 3.3.12**, placa **ESP32S3 Dev Module**, Flash Size **16MB**, Flash Mode **QIO 80MHz**, PSRAM **OPI PSRAM** y Partition Scheme **Sentinel standard USB (4MiB factory, no OTA)**. El SDK distribuido es IDF5.5.5; no hace falta reconstruirlo ni instalar el paquete Sentinel anterior.

## Preparación una vez por equipo

1. Instalar el core oficial indicado mediante Boards Manager. Conservar otras placas/instalaciones.
2. Instalar [ESP_SSLClient3.1.3](https://github.com/mobizt/ESP_SSLClient/releases/tag/v3.1.3) con **Sketch → Include Library → Add .ZIP Library**.
3. Cerrar Arduino IDE. Desde PowerShell en la raiz del repositorio ejecutar:

   ```powershell
   .\firmware\arduino\install-wolfmqtt.ps1
   .\firmware\arduino\install-board-config.ps1
   ```

4. Reiniciar el IDE y seleccionar la opcion de particiones indicada arriba. Los scripts detectan las rutas del IDE; admiten `-SketchbookPath` o `-ArduinoDataPath` y `-WhatIf`. El instalador wolfMQTT admite `-ArchivePath` para el ZIP original ya descargado. Ver [instalacion y recuperacion](../../docs/firmware.md#instalación-inicial).

La preparacion es inicial, sin administrador: descarga original2.2.0 verificada por SHA256, estructura Arduino y tres archivos personalizados. No instala wolfSSL. Si el destino ya es correcto no escribe ni descarga; si cambia la biblioteca, conserva la anterior fuera de libraries en Sketchbook/.sentinel-backups. La configuracion respalda boards.local y mantiene cuatro claves, elimina cinco overrides Sentinel obsoletos de carga/exportación y conserva opciones ajenas, respaldo e idempotencia. No se reemplaza el core ni se reconstruye un SDK. Versiones/hashes y soporte fuente en ../arduino; proveedores y binarios permanecen fuera de Git.

## Compilar y preparar carga

Usar **Verify** y después **Upload** solo al identificar una placa real. No elegir un COM Bluetooth por su número; el borrado inicial se hace únicamente en la carga limpia indicada. Desde el CLI nativo del IDE, con las dependencias y opción ya instaladas:

```powershell
& "$env:LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe" compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=sentinel firmware/SentinelNode
```

Cada Verify/Upload usa las recetas estándar de Arduino y las herramientas internas del IDE, sin ejecutar Python/generadores propios del proyecto ni pasos manuales por compilación. `src/sentinel/v2_validator.h` es una fuente local comprobada contra los contratos por las pruebas del repositorio. Arduino descubre recursivamente los `.cpp` bajo `src`; no incluir otros `.cpp` ni copiar los módulos a `libraries`.

La tabla reserva `otadata`0xe000/0x2000, factory0x10000/4MiB, NVS0x410000/24KiB y `event_log`0x416000/256KiB. No hay partición PHY separada ni OTA funcional. Arduino carga también `boot_app0` en la región reservada y permite exportación estándar; `merged.bin` es imagen de inicialización sin garantía de preservar datos. Usar directorio de build nuevo y no reutilizar exportaciones de otra tabla. El wrapper NVS sigue justificándose por evitar autoerase antes de setup en reinicios normales, no por continuidad entre versiones.

El adaptador NVS guarda el resultado real de la primera inicialización del core. Evita su borrado/reintento automático ante NO_FREE_PAGES o NEW_VERSION_FOUND; el runtime recibe el error real y detiene identidad antes de BLE/modos. No formatea ni anuncia persistencia sana cuando falla.

## Transportes y recuperación de vínculos

El gestor central conserva Wi-Fi/netifs, captura pasiva, NAPT prestado al Sanitizador y exclusión de radio; ningún módulo usa `WiFi.begin()` ni otro host BLE/journal. MQTT5 usa wolfMQTT con una tarea y outbox acotados; DNS/TCP/TLS y ACK del broker no bloquean el supervisor. La transición conserva el propietario anterior hasta confirmar cierre del worker, dentro del plazo existente. Se conservan sesión limpia/expiry0, QoS, LWT offline, RETAIN y rechazo de órdenes retenidas, identificadores y reconexión. Si se intenta reprovisionar mientras el worker mantiene una sesión, se responde `busy` sin detener ni cambiar credenciales/red/propietario; pasar explícitamente a Idle, esperar su confirmación y repetir la configuración.

Cada conexión TLS exige primero CA y nombre DNS/IP con `NetworkClientSecure` oficial. Antes de cualquier byte MQTT o credencial, el motor BearSSL mantenido comprueba primero las fechas del certificado CA configurado y luego la misma cadena DER contra ese ancla y tiempo obligatorio del nodo. Un bundle puede ofrecer varias CA: se exige una ruta válida, sin aceptar una CA vencida ni rechazar la ruta por una CA ajena vencida. Cualquier fallo o cadena ausente cierra el canal sin autenticación MQTT; no hay alternativa plana, `setInsecure` ni sustitución por fingerprint. La composición conserva SAN IP, que el cliente BearSSL directo no admite en esta versión.

BLE conserva ventana física, passkey en OLED, MITM, Secure Connections obligatorias, claves de16bytes y un único cliente activo. Permite **tres teléfonos recordados**, aprobado por el usuario; la capacidad compilada del core es3. La presencia de legacy en el SDK no habilita su uso: se configuran SC-only/nivel4 en runtime y se comprueban los bonds antes de iniciar el host.

**Si había cuatro vínculos o datos incompatibles:** el preflight abre `nimble_bond` solo para lectura y detiene BLE antes de la restauración/escritura. No omite el cuarto, no lo sobrescribe ni expulsa teléfonos. El estado BLE permanece `stopped` y la consola informa incompatibilidad preservada. Conservar una copia de la flash/NVS y el firmware anterior para una decisión explícita de recuperación; no borrar ni volver a emparejar sobre esos datos como solución automática. Tres bonds solo se reutilizan si sus formatos y parámetros SC/MITM/16bytes son compatibles; un formato desconocido o fallo de lectura también detiene BLE conservando bytes.

## Módulos y comprobación

`src/sentinel_api/include/module_api.hpp` conserva la API compartida. Los módulos incluyen rutas relativas a esa API y reciben recursos mediante `ModuleContext`; se mantienen `init/start/request_stop/stopped/handle/tick/snapshot` y las factories `sentinel_escudo_module`, `sentinel_sanitizador_module`, `sentinel_cebo_module`. Escudo está habilitado; Sanitizador y Cebo siguen deshabilitados hasta existir sus implementaciones reales. Habilitar un flag sin factory provoca error de enlace.

Pruebas portables desde la raíz, con Node24, cc/c++, OpenSSL, tar y unzip:

```sh
npm run test:firmware
npm run test:escudo
```

La evidencia del build oficial completo y las limitaciones físicas se registra en [verification.md](../../openspec/changes/sentinel-arduino-dependencies/verification.md). Un build o una prueba host no acredita upload, radio, pairing Android, MQTT/TLS en ESP32, OLED, botón ni persistencia física. Solo probar redes propias/autorizadas; configurar credenciales por BLE protegido y no incluirlas en fuentes, logs o ejemplos.
