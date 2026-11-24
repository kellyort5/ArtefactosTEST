# Escudo: descubrimiento y selección

Responsable: Persona 1. Cambio: [sentinel-escudo-descubrimiento](../../openspec/changes/archive/2026-11-11-sentinel-escudo-descubrimiento/proposal.md). Se integra con la [API de módulos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) y el [contrato v2](../../contracts/module-protocol.md). El firmware incluye `firmware/SentinelNode/src/escudo/` y habilita `SENTINEL_MODULE_ESCUDO=1` en `SentinelConfig.h`.

Build, subida y monitor tienen como destino [Arduino IDE Windows y core oficial ESP32 3.3.12](../firmware.md); la integración de dependencias está en curso. Las órdenes conservan contratos y comportamiento. Los runners Node comprueban lógica host y no sustituyen build ni pruebas físicas; el [informe del port previo](../../openspec/changes/sentinel-arduino-firmware-port/verification.md) conserva su evidencia histórica.

Este incremento implementó descubrimiento y objetivo en el ESP32 y se encuentra archivado con pruebas físicas pendientes. La [captura](escudo-capture.md) y los [cuatro detectores](escudo-detectors.md) se integran en sus incrementos posteriores; las capacidades `capture` y `detectors` reflejan esas implementaciones. El catálogo no clasifica amenazas ni interpreta SSID duplicados como maliciosos.

## Recorrido y operaciones

El cliente usa metadata v2, boot/revisión/TTL vigentes, BLE autorizado y el framing común. `set_mode` cambia a `escudo`; el gestor prepara el driver STA sin asociación. El componente no solicita credenciales, no conecta Wi-Fi y no instancia MQTT ni otro journal. BLE sigue disponible mientras se solicita el scan pasivo oficial de 2.4 GHz.

Los siguientes fragmentos son **requests** para `makeModuleCommand(metadata,request,commandId)`, no mensajes completos ni bytes listos para escribir en GATT. Usar `encodeModuleMessage` y el framing descrito en los contratos. La app v1 no controla este firmware; la adaptación v2 pertenece a Persona 4.

```json
{"operation":"set_mode","args":{"mode":"escudo"}}
```

```json
{"operation":"escudo.scan","args":{}}
```

`accepted` confirma recepción; `applied` lleva `{kind:"job",job_id:command_id}` cuando el scan se inicia. No significa que ya existan resultados. `get_state` expone `module.scan_status` (`idle`, `running`, `complete`, `error`) y `module.scan_id`. Al terminar correctamente se emite `escudo.networks` con la primera página y ese mismo `scan_id`; la consulta recupera la instantánea aunque se pierda el evento.

```json
{"operation":"escudo.networks","args":{"after":null,"limit":4}}
```

La respuesta es `{kind:"escudo.networks",scan_id,networks,next,more}`. Pedir las siguientes páginas con el cursor devuelto y metadata/revisión actualizadas:

```json
{"operation":"escudo.networks","args":{"after":"scan-ejemplo:4","limit":4}}
```

`scan-ejemplo:4` es solo ilustrativo: usar siempre el cursor recibido. No construir desplazamientos propios. Una instantánea nueva invalida los cursores anteriores y devuelve `revision_conflict`; reiniciar con `after:null`. Un cursor fabricado que no corresponde a una continuación emitida devuelve `invalid_args`. La última página tiene `next:null,more:false`.

Seleccionar o cambiar usando exactamente SSID/BSSID/canal del catálogo completo más reciente:

```json
{"operation":"escudo.select_target","args":{"ssid":"LAB","bssid":"02:00:00:00:00:01","channel":6}}
```

El ACK aplicado devuelve `{kind:"escudo.target",target:{ssid,bssid,channel,capture_status}}`, con el estado real de captura, y se emite `escudo.target`. El gestor cambia de canal y, si había captura, comprueba su reinicio en el canal nuevo. El núcleo guarda la selección con NVS oficial y commit antes de reemplazar el objetivo y confirmar éxito. BSSID no observado, SSID distinto, canal inconsistente o fallo del gestor no borran el objetivo anterior. No se asocia al AP seleccionado.

Una nueva exploración reemplaza el catálogo y conserva el objetivo seleccionado hasta que se cambie explícitamente. El catálogo de observaciones es transitorio. La selección aceptada se recupera al entrar en Escudo tras un boot nuevo o al volver desde Idle, con `scan_status:"idle"`, `scan_id:null` y captura detenida. `get_state` puede mostrar objetivo y captura detenida simultáneamente; la recuperación no explora, no inicia captura, no se asocia ni activa MQTT. El boot continúa en Idle: entrar al modo e iniciar captura requieren órdenes explícitas. Una actualización/reinstalación USB permite reinicializar la configuración según la [política de persistencia](../persistence-policy.md). Los eventos ya emitidos usan el journal compartido de 64 entradas.

El helper [escudo_target.cpp](../../firmware/SentinelNode/src/sentinel/escudo_target.cpp) pertenece al núcleo y se llama desde el supervisor; usa `nvs_open/get_blob/set_blob/commit/close` del [SDK oficial](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/storage/nvs_flash.html). Guarda un registro de 51 bytes en `nvs`, namespace `escudo_target`, clave `selection`, con firma/versión, SSID terminado de hasta 32 bytes, BSSID unicast no nulo, canal 1–14 y enums de auth/cifrados validados. No guarda RSSI, catálogo, uptime, generación, captura ni evidencia/cooldowns. No hay migración entre versiones ni formato de NVS ante errores.

La seguridad guardada es contexto de la selección para identificar desconexiones; no constituye una observación actual del AP. Al recuperar, la base temporal de Evil Twin queda invalidada hasta observar un Beacon/Probe Response real del objetivo en esta entrada al modo. Un candidato actual puede conservarse dentro de su ventana y compararse cuando llegue esa observación real; no se inventan RSSI, tiempos ni anuncios para completar la comparación. Deauth, desconexiones sospechosas y Karma conservan su identidad de objetivo y requieren evidencia nueva; los cooldowns en RAM conservan la política existente durante el mismo boot.

## Datos, límites y errores

| Dato o recurso | Comportamiento |
| --- | --- |
| Catálogo | Máximo 32 BSSID en RAM, deduplicados por BSSID. Un SSID compartido conserva varios BSSID. Una observación repetida actualiza sus datos. |
| Límite observado | Si el callback SDK informa más de 32 resultados, se emite `system.status` con `status_code:"capacity_exceeded",resource:"wifi"`. No se inventa un total cuando el SDK no lo proporciona. |
| Páginas | 1–4 entradas; presupuesto conservador de 1280 bytes para dejar espacio al sobre/ACK de 2048 bytes, incluso con IDs largos. Se reduce la cantidad si hace falta, sin truncar campos. `more` describe las entradas retenidas, no todas las redes del entorno. |
| SSID | Hasta 32 bytes del registro SDK. Oculto/vacío sigue `""`, sin nombres inventados. Bytes no imprimibles o UTF-8 inválido se representan mediante `{encoding:"hex",value:"..."}`. Se compara la misma secuencia al seleccionar. El scan SDK usa SSID terminado en NUL; no reconstruye bytes/nombres que no expone. |
| Seguridad | Auth y cifrados se mapean desde los enums del ESP-IDF fijado. Tipos sin representación contractual, como OWE/WAPI, se publican `unknown`; no se confunden con una red abierta. Cifrados anunciados distintos se representan `mixed`. |
| PMF | `unknown`: `wifi_ap_record_t` no expone PMF. No se infiere de WPA3. El parser de tramas/IE pertenece al incremento de captura. |
| RSSI y tiempo | RSSI se limita al rango contractual -127..0 dBm. `seen_uptime_ms` es el momento del callback de fin, no la hora de cada Beacon; el SDK de scan no entrega ese detalle. El núcleo asigna tiempo/calidad del evento sin conectar Wi-Fi. |
| Trabajo pendiente | Sin tareas propias ni callbacks que escriban flash. `tick` procesa como máximo ocho eventos de radio por vuelta. Mantiene como máximo un aviso de catálogo, uno de objetivo y un diagnóstico; reintenta su entrega sin bloquear cuando el núcleo no acepta la salida. |
| Scan concurrente | Otra petición durante un scan o con un aviso pendiente devuelve `busy`. Consultar estado y solicitar Idle siguen disponibles mediante el núcleo. |
| Fallo del SDK | Error al iniciar, leer resultados o cambiar canal devuelve `resource_error`; un callback de fallo deja `scan_status:"error"` y diagnóstico. Una lectura correcta con cero redes produce catálogo vacío, no error. |
| Persistencia del objetivo | Ausencia permite seleccionar normalmente. Error de lectura/corrupción/tamaño/versión/campos inválidos emite `system.status` con `status_code:"resource_error",resource:"system"`; en una instancia nueva el objetivo permanece null y en el mismo boot se conserva la selección válida previa. No se escribe ni se borra al recuperar; una selección explícita válida puede reemplazar la entrada defectuosa. |
| Fallo al guardar | Un fallo de apertura/escritura/commit rechaza con `resource_error` y conserva el objetivo anterior. La captura preparada para el objetivo nuevo se detiene e invalida su generación antes de volver al supervisor; si el driver rechaza la parada, queda `capture_status:"error"`, se reintenta desde `tick` y se impide reiniciar/seleccionar hasta liberar la captura. El fallo nunca deja capturando el objetivo rechazado como si fuera el anterior. Tras liberar, una nueva captura requiere orden explícita. |
| Timeout | Tras 10 s sin finalización, deja `scan_status:"error"` y diagnóstico `timeout`. Pasar por Idle y volver a Escudo antes de repetir; el gestor cancela el scan físico al parar Wi-Fi. Esto evita asignar una finalización tardía al siguiente job. |

Los eventos del componente real llevan `origin:"real"` y se serializan/persisten en el núcleo. Los mensajes generados por el sink de pruebas de host llevan `origin:"simulated"`. El componente no almacena contraseñas ni cuerpos de solicitudes ni escribe directamente en NVS. Sus diagnósticos no incluyen contenido recibido por BLE ni errores SDK sin sanitizar.

## Verificación de software

Desde la raíz, con el entorno fijado de [desarrollo](../development.md):

```sh
node firmware/test/arduino_build_test.mjs
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH SENTINEL_SANITIZE=1 node firmware/test/escudo/check_contracts.mjs
```

Las pruebas compilan el componente real junto al supervisor/gate/serializador portables. Usan un recurso de radio de prueba para cubrir SSID oculto/binario, BSSID duplicados, saturación, páginas/cursor obsoleto, IDs largos, ACK, selección inválida sin mutación, error/timeout, retención acotada de avisos, Idle y nuevo scan. Los mensajes producidos se validan con el codec compartido y `checkModuleAckForCommand`. Esto acredita lógica e integración de software; no acredita recepción Wi-Fi ni BLE real.

`npm run test:escudo` incluye además [target_persistence_test.cpp](../../firmware/test/escudo/target_persistence_test.cpp), con helper y módulo reales y dobles solo en los límites SDK/NVS/radio: commit antes de éxito, cambios de canal, fallos de apertura/escritura/commit y parada/reinicio, restauración por nueva instancia/Idle sin actividad automática, corrupción/tamaño/versión/SSID/BSSID/canal inválidos y evidencia temporal descartada. Las regresiones previas enlazan el mismo helper real con NVS sin estado; sus fuentes C++ se conservan. Estas pruebas host no acreditan durabilidad física ante un corte.

Los ajustes compartidos son necesarios para integrar el descubrimiento: metadatos `scan_status/scan_count`, `read_scan_results` con retorno de éxito y su concesión, y activación del componente. Las interfaces anteriores se conservan; no cambia la propiedad de los drivers ni los contratos de mensajes.

## Prueba física pendiente — tareas 2.1 y 2.2

El usuario autorizó continuar conservando estas comprobaciones pendientes. No se ha grabado ni probado este incremento en una ESP32.

1. Conectar ESP32-S3 N16R8, OLED y botón según [cableado](../firmware.md). Identificar el puerto real y ejecutar upload/monitor; no asumir un nombre de puerto.
2. Abrir la ventana de emparejamiento con el botón largo de 3 s y conectar un cliente BLE autorizado que use v2 y el framing común. Usar el adaptador v2 integrado en la app; las pantallas funcionales específicas conservan su encargo de Persona 4.
3. Leer metadata y `get_state`: Idle al arrancar. Cambiar a Escudo y consultar `scan_status:"idle"`, objetivo null si aún no hay selección persistida, o el objetivo aceptado anterior con captura detenida, y capacidades acordes con los incrementos realmente incluidos.
4. Solicitar un scan en LAN propia/autorizada. Registrar command/job/scan ID y ACK; consultar estado durante el scan para confirmar que el control BLE responde.
5. Recibir `escudo.networks`, consultar todas las páginas y cotejar SSID/BSSID/canal/RSSI/seguridad con la red propia. Documentar datos no observables y límites, sin inventar resultados.
6. Seleccionar una tupla observada, comprobar ACK/estado/evento y enviar luego una tupla inconsistente. El objetivo válido debe conservarse. Probar cambio a otro BSSID observado si hay uno disponible.
7. Verificar que no hubo asociación, dirección IP ni MQTT del nodo durante Escudo. Registrar evidencia del monitor/estado y, si se observa tráfico, hacerlo únicamente en la LAN autorizada; una compilación no prueba ausencia de probes reales.
8. Iniciar otro scan y solicitar Idle antes de finalizar. Confirmar el ACK de modo y ausencia de recursos residuales; volver a Escudo y repetir un scan completo.
9. Registrar puerto/build, cliente BLE, IDs y observaciones en la [evidencia del cambio](../../openspec/changes/archive/2026-11-11-sentinel-escudo-descubrimiento/verification.md). Mantener abiertas las casillas físicas hasta demostrar estos pasos. La transición también puede aportar evidencia para la tarea 2.2 pendiente de la [API archivada](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/tasks.md).
10. Con una selección confirmada, pasar a Idle y volver a Escudo; después reiniciar y cortar/restablecer alimentación sin subir firmware. Comprobar boot en Idle y objetivo recuperado al entrar en Escudo, sin catálogo ni captura automática; iniciar captura explícitamente y confirmar que las detecciones usan tramas nuevas. La programación USB puede reinicializar el objetivo. Esta comprobación física permanece pendiente y no la sustituye el runner host.
