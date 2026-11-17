# Persistencia operativa y programación USB

Política aprobada en [sentinel-mvp-standard-persistence](../openspec/changes/sentinel-mvp-standard-persistence/proposal.md), con [diseño](../openspec/changes/sentinel-mvp-standard-persistence/design.md) y tareas bajo control del coordinador. Sustituye la excepción anterior de conectividad solo en RAM y la exigencia de continuidad de datos entre versiones/cargas USB. No reduce Escudo, Sanitizador, Cebo ni sus responsables.

Identidad, bonds BLE, configuración aceptada e historial confirmado deben sobrevivir al uso normal, reinicio y corte de alimentación. Solo se confirma persistencia después de completar escritura y commit; una escritura interrumpida puede perderse. Un error de NVS se informa sin formato automático ni expulsión de vínculos durante el arranque normal. Reconectar BLE/MQTT no es una reinstalación.

Reinstalar o actualizar por USB puede reinicializar esos datos: no hay migración ni garantía de continuidad entre versiones. Después puede ser necesario borrar vínculos/caché antiguos del teléfono, volver a emparejar y configurar. La app debe reconciliar historial/cursores aunque el device_id derivado de MAC siga igual; un ID igual no garantiza continuidad del journal. Una carga con borrado total desactivado no implica que siempre se borre toda la flash; tampoco garantiza conservar datos.

## Preparación y primera carga

1. Instalar las dependencias y ejecutar el [instalador de configuración](firmware.md#instalación-inicial) con el IDE cerrado. Conserva respaldo y claves ajenas; elimina solo cinco overrides Sentinel obsoletos de carga/exportación y mantiene cuatro claves: menú, `build.partitions`, `upload.maximum_size=4194304` y enlace `--wrap=nvs_flash_init`. Repetirlo correctamente no cambia el archivo. Los scripts de setup no graban placas.
2. Reiniciar Arduino IDE y seleccionar core oficial ESP32 3.3.12, ESP32S3 Dev Module, flash16MB/QIO80MHz, OPI PSRAM y la opción Sentinel. Verificar el sketch completo con un directorio de build nuevo; no reutilizar exportaciones anteriores.
3. Identificar el COM real. Para la **primera carga de la tabla nueva**, seleccionar **Erase All Flash Before Sketch Upload: Enabled** y **Subir** desde Arduino IDE. Se pierden los datos anteriores; no hay migración. Esta inicialización no se usa como prueba de persistencia y no ocurre en cada boot.
4. Tras inicializar, volver a **Disabled** para cargas normales si no se quiere solicitar borrado total. Las siguientes actualizaciones/reinstalaciones USB tampoco ofrecen garantía de conservar datos. Emparejar de nuevo, configurar por BLE protegido y producir eventos confirmados.
5. Probar reinicio y corte de alimentación **sin subir firmware**: misma identidad, bonds, configuración validada e IDs/cursores de eventos retenidos; boot nuevo, Idle sin asociación ni arranque de módulos, y reloj UTC desconocido hasta nueva referencia confiable. Mantener estas pruebas físicas como pendientes hasta obtener evidencia real.

La carga USB habitual usa las recetas estándar de Arduino, incluido `boot_app0.bin` en `0xe000`. Una imagen `merged.bin` de exportación es una imagen de **inicialización** que puede rellenar regiones de datos; no es un respaldo de NVS ni un mecanismo de actualización que preserve datos. Ambas rutas carecen de garantía de conservación. Cambiar a firmware con otra tabla requiere de nuevo carga limpia.

## Distribución aprobada

Flash nominal16MiB; tabla fuente en [partitions.csv](../firmware/SentinelNode/partitions.csv):

| Partición | Tipo/subtipo | Inicio | Tamaño |
| --- | --- | --- | --- |
| otadata | data/ota | 0xe000 | 0x2000 |
| factory | app/factory | 0x10000 | 0x400000 (4MiB) |
| nvs | data/nvs | 0x410000 | 0x6000 (24KiB) |
| event_log | data/nvs | 0x416000 | 0x40000 (256KiB) |

`otadata` reserva el destino estándar de `boot_app0`; no implementa OTA funcional. No hay partición PHY separada: el SDK oficial incluye initdata en la aplicación y conserva calibración en NVS. El journal sigue limitado a64 eventos de hasta2048bytes, con sobrescritura/gap y deduplicación. No se amplía el historial a toda la flash.

El wrapper NVS sigue siendo una excepción mínima justificada: `initArduino()` inicializa NVS antes de `setup()` y puede borrarla ante NO_FREE_PAGES/NEW_VERSION_FOUND. La protección conserva el error original y evita ese borrado durante reinicios normales; no aporta compatibilidad entre versiones. Evaluar APIs, bibliotecas y recetas oficiales antes de personalizar core/compilador/carga, y justificar cualquier excepción mínima restante.

## Configuración y privacidad

Wi-Fi/MQTT configurados por el usuario autorizado se guardan mediante NVS oficial, con entrada acotada/validada y confirmación aplicada solo después de commit. Si falla, se informa error y se conserva la configuración anterior. Ausencia significa no configurado; corrupción/error se expone sin borrar bonds/historial. Al arrancar se restaura y valida en Idle sin conectarse ni confiar en un reloj UTC viejo. `WIFI_STORAGE_RAM` evita almacenamiento implícito del driver, pero no elimina la persistencia explícita aprobada.

Escudo conserva también la selección aceptada SSID/BSSID/canal mediante el helper NVS del núcleo, sin almacén ni journal del módulo. Su registro acotado y validado incluye auth/cifrados únicamente como contexto de la selección; excluye RSSI, catálogo, uptime, generación, evidencias recientes y estado de captura. Comprueba el cambio de canal y guarda con commit antes del ACK aplicado. Un fallo devuelve `resource_error`, conserva el objetivo previo e invalida/detiene la captura preparada para el objetivo rechazado; si falla la parada, expone error, reintenta liberar y bloquea nuevos arranques hasta lograrlo.

El boot permanece en Idle. Al entrar en Escudo tras boot o Idle se recupera el objetivo con captura detenida, sin scan, asociación ni MQTT automáticos; `get_state` ya representa esa combinación sin cambios wire. Ausencia permite seleccionar; corrupción/error se informa mediante `system.status` sin formato ni borrado ajeno y permite una selección explícita válida. Si había selección válida en RAM en este boot, un error de lectura no la descarta. La recuperación descarta toda base temporal de detección: Evil Twin exige una observación nueva del objetivo para compararlo con candidatos actuales; la seguridad anterior no se presenta como anuncio nuevo. Ver [conducta y pruebas del objetivo](modules/escudo-discovery.md).

Se permiten las credenciales propias autorizadas y CA en NVS estándar; **no se promete cifrado físico de flash**. No guardar credenciales recibidas por honeypot/terceros. Nunca incluir secretos en Git, eventos, estado público, logs o evidencia. Mantener BLE protegido y TLS con CA/nombre/fechas; descartar formularios y copias transitorias tras procesarlos.

P2 extenderá esta política a upstream/AP/DNS/DoH/reglas cuando implemente sus encargos. P3 persistirá configuración de Cebo e historial sanitizado mediante los recursos centrales. P4 gestionará identidad/caché/cursores tras inicialización USB sin prometer migración. Estos módulos, sus pantallas y verificaciones físicas siguen pendientes según sus fichas.

## Evidencia y límites

Los builds y logs anteriores conservan sus resultados y hashes como evidencia histórica de sus propias entradas. No acreditan la nueva tabla, cuatro claves ni persistencia de configuración. La [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md) registra el resultado del build final integrado y sus límites. Hardware/reinicio/corte/primera carga limpia siguen pendientes. Ninguna casilla física se cierra por documentación, compilación o pruebas host.
