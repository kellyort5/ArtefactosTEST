# Comprobación de compilación Windows por el coordinador

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

El CLI nativo incluido en Arduino IDE 2.3.10 compiló el sketch completo con salida 0. No se modificaron fuentes ni bibliotecas para esta comprobación.

```powershell
& "$env:LOCALAPPDATA/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe" compile --fqbn sentinel:esp32:sentinel_s3_n16r8 --build-path "$env:TEMP/sentinel-arduino-firmware" firmware/SentinelNode
```

- Imagen: **1358113/4194304 bytes**; RAM estática: **161556/327680 bytes**. Son cifras del enlazador, no mediciones físicas ni benchmark.
- Log: `/tmp/sentinel-arduino-full-compile.log`; recibo: `/tmp/sentinel-arduino-full-build-receipt.json`.
- ELF: `%TEMP%/sentinel-arduino-firmware/SentinelNode.ino.elf`; SHA-256: `7ec3342610ea3191f4bb39a4c579df13dfa18fbdbbfc6f62249341b8b376f044`.
- `nm.exe` del paquete confirma exactamente una definición de `app_main`, `sentinel_escudo_module` y `sentinel_arduino_nvs_status`. Sanitizador/Cebo no definen factories porque siguen pendientes de implementar, con flags deshabilitados; no se anuncian sus servicios.
- El build contiene los seis objetos de `module`, `discovery`, `capture`, `channel_scheduler`, `frame_parser` y `detectors` de Escudo.
- El validador se generó en el build Windows con los hashes de los 15 inputs fuente y salida coincidentes. SHA-256 de salida: `9c6e455d0b59877eb3749aef89f68db27540b1bff1ac7351857e9dfd7ccd4b68`. No se genera dentro del sketch ni se usa un contrato alternativo.

Arduino IDE 2.3.10 abrió la ventana **SentinelNode | Arduino IDE 2.3.10** desde el checkout. La comprobación visual de selección de placa sigue pendiente: la primera observación de accesibilidad agotó su tiempo; se intenta una observación sin captura. Abrir la ventana no acredita por sí mismo compilar mediante el botón del IDE.

No hubo subida, erase-all, monitor de placa ni prueba BLE/radio/NVS física. La detección Windows previa mostró solo COM3/COM4 Bluetooth. La tarea 2.3 permanece pendiente hasta identificar y probar la ESP32 real.
