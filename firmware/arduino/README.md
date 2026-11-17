# Preparacion inicial en Windows

`install-board-config.ps1` conserva cuatro claves propias: menú, `build.partitions`, máximo4MiB y `--wrap=nvs_flash_init`. Retira cinco overrides Sentinel antiguos de carga/exportación; preserva claves/comentarios ajenos, crea respaldo y es idempotente. No modifica core/SDK/compilador ni graba una placa. Carga y exportación vuelven a las recetas oficiales.

Aplicar la [política vigente de persistencia y USB](../../docs/persistence-policy.md): primera carga de la nueva tabla con **Erase All Flash Before Sketch Upload: Enabled** desde el IDE; pérdida de datos anteriores y nuevo emparejamiento/configuración, sin migración. Después volver a Disabled si no se solicita borrado total; USB no garantiza conservación. Reinicios/cortes normales sí deben conservar datos confirmados. El estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md); las pruebas físicas siguen pendientes y los logs anteriores son históricos.

Instalar core oficial ESP32 **3.3.12** y **ESP_SSLClient3.1.3** desde Arduino IDE. Cerrarlo y ejecutar en PowerShell, desde la raiz del repositorio:

```powershell
.\firmware\arduino\install-wolfmqtt.ps1
.\firmware\arduino\install-board-config.ps1
```

Reiniciar el IDE. Elegir **ESP32S3 Dev Module**, **16MB**, **QIO 80MHz**, **OPI PSRAM** y **Sentinel standard USB (4MiB factory, no OTA)**; luego Verificar y seguir la primera carga limpia indicada arriba antes de las cargas normales. Ver [guia completa](../../docs/firmware.md#instalación-inicial).

Los scripts usan las rutas del IDE, son repetibles y no se ejecutan durante el build. `-WhatIf` muestra la accion sin descargar ni modificar archivos. No requieren administrador, Python ni wolfSSL. Si la politica de PowerShell los bloquea, `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass` permite ejecutarlos solo durante esa terminal.

## ZIP existente y rutas personalizadas

```powershell
.\firmware\arduino\install-wolfmqtt.ps1 -ArchivePath 'C:\Descargas\wolfMQTT-2.2.0.zip'
.\firmware\arduino\install-wolfmqtt.ps1 -SketchbookPath 'D:\Arduino'
.\firmware\arduino\install-board-config.ps1 -ArduinoDataPath 'D:\Arduino15'
```

El ZIP debe ser el original2.2.0 con el SHA256 de dependencies.json. Los proveedores quedan fuera del repositorio. La configuracion personalizada sigue limitada a los tres archivos de wolfmqtt/; el helper y los instaladores solo preparan el entorno.

## Respaldos y errores

- Biblioteca reemplazada: `<Sketchbook>\.sentinel-backups\wolfMQTT-<id>`, fuera de libraries para evitar duplicados. Para restaurar, cerrar el IDE, apartar la biblioteca actual y mover el respaldo a `libraries\wolfMQTT`.
- Configuracion modificada: `boards.local.txt.sentinel-<id>.bak`, junto al archivo. Para restaurar, cerrar el IDE y copiar su contenido a boards.local.txt.
- Hash incorrecto, core ausente o archivo bloqueado: corregir la causa indicada y repetir. Los destinos previos se conservan; no borrar flash/NVS ni desactivar las guardas.

## Pruebas opcionales de mantenimiento

Windows PowerShell5.1, destinos temporales fuera del repositorio:

```powershell
.\firmware\test\installers_test.ps1 -ArchivePath 'C:\Descargas\wolfMQTT-2.2.0.zip' -TestDownload
```

El ZIP se verifica contra el manifest. TestDownload añade una descarga HTTPS real; sin el switch, se reutiliza el ZIP local. Comprueba repeticion sin escrituras, reparacion, respaldos, claves ajenas, duplicados, hashes invalidos y bloqueos Windows. No compila ni programa una placa.
