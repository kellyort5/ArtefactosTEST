# Conectividad de la base

La base dispone de NimBLE GATT real en los tres perfiles y de STA/esp-mqtt5 únicamente en Cebo. Escudo no se asocia, no escanea activamente, no captura y no inicia MQTT. Consultar [firmware.md](firmware.md) para cableado, build, puertos y evidencia; [contracts/README.md](../contracts/README.md) define los mensajes exactos; [broker/README.md](../broker/README.md) prepara el laboratorio TLS.

## Pairing y control local

1. Comprobar alimentación, OLED y botón; abrir el monitor a115200. Arranca idle con revisión0 y un boot ID nuevo, sin conectarse a Wi-Fi. La ausencia de OLED impide nuevos pairings.
2. Mantener el botón3 s. Se abre una ventana física de120 s; liberar no cambia el perfil. No hay una operación remota para abrirla.
3. En una app Android compatible, conectar al servicio `a90d0001-6d64-4f43-9e6d-9db51f1f0000` y completar la passkey de seis cifras mostrada en OLED. El firmware exige LE Secure Connections, MITM, cifrado de128 bits y bonding; no admite Just Works ni legacy. El código no aparece por serie, historial ni MQTT.
4. Fuera de la ventana solo se aceptan pares previamente autorizados. Hasta4 bonds NVS: no se elimina automáticamente el más antiguo para admitir otro. Si se llena, el intento falla y requiere gestión explícita posterior; ninguna pulsación borra bonds o historial.
5. Leer metadata protegida en `a90d0002-…` (hasta256 bytes), suscribirse a indications protegidas `a90d0004-…` y escribir con respuesta en `a90d0003-…`. Leer metadata mediante una lectura larga coherente si MTU es23. El estado completo se pide con `get_state`.

Metadata contiene device_id, boot_id, revisión, uptime y máximo2048. Todos los comandos identifican nodo/arranque destinatarios, revisión esperada y un vencimiento futuro dentro de30 s de uptime del dispositivo; no necesitan UTC. Se verifican autorización y vigencia antes de consultar la caché32. Un duplicado lógico vigente usa digest SHA256 y resultado sanitizado; el mismo ID con otros argumentos falla `id_conflict`. Cambiar campos de transporte no produce un segundo efecto. Consultas no incrementan revisión.

Cada mensaje lógico máximo2048 bytes UTF-8 se fragmenta con cabecera LE de8 bytes: transfer ID32, offset16 y total16. MTU23 deja12 bytes de datos por fragmento; los offsets son contiguos, el timeout total es10 s y solo hay una transferencia por dirección/conexión. Indications se secuencian esperando su confirmación ATT. Desorden, exceso, timeout o desconexión descartan el parcial; un parcial nunca se ejecuta. Tras reconectar se consulta estado y se envía un comando completo vigente.

## Aprovisionamiento privado y reloj

Mediante BLE autenticado, con metadata recién leída:

1. Enviar `set_time` con `args.timestamp` UTC ISO8601. Usar hora confiable del operador; la calidad pasa a `app_sync`. El reloj desconocido mantiene MQTT bloqueado y visible como error; no se inicia Wi-Fi en Escudo para obtener hora. Fecha, CA y nombre se verifican durante TLS.
2. Consultar metadata o state otra vez porque aplicar hora cambia revisión. Enviar `configure_connectivity` con los campos planos del contrato: SSID de red propia, contraseña Wi-Fi, `mqtts://host[:puerto]`, usuario/password MQTT y CA PEM pública. El URI no lleva credenciales, ruta ni query. La CA debe ser un certificado CA válido y todo el mensaje debe caber en2048 bytes; usar una CA compacta de laboratorio si hace falta.
3. No devolver los argumentos privados: accepted/applied/rejected y estado solo exponen resultado/capacidades/errores. El cliente debe descartar formularios privados después del envío.
4. Consultar estado actualizado y pedir `set_mode` Cebo. El perfil se confirma cuando los recursos están preparados; STA/MQTT continúan conectando asíncronamente. Configurar en idle/Escudo no abre una conexión de red. Reconfigurar en Cebo cierra el cliente previo antes de reutilizar sus buffers privados y reinicia conectividad.

SSID/contraseñas/endpoint/CA se conservan en buffers de RAM del gestor de red; Wi-Fi usa `WIFI_STORAGE_RAM`. Los cuerpos privados en cola y cJSON se borran después de procesarlos; la caché guarda digest y resultado, nunca el comando completo. Identidad, bonds y eventos sanitizados sí persisten en NVS. Tras reboot hay que reprovisionar conectividad y referencia temporal, mientras el boot ID nuevo invalida comandos anteriores. `configure_connectivity` y `set_time` enviados por MQTT se rechazan como unauthorized.

## Broker/TLS y acceso LAN

Generar material local bajo `broker/.secrets/` siguiendo su README. La CA pública se copia al comando protegido; claves privadas y passwords permanecen en rutas ignoradas. La identidad MQTT del nodo debe tener ACL para su device_id real, obtenido en metadata o consola, y para ningún otro nodo. El client ID se deriva del dispositivo; la autenticación utiliza la cuenta del broker.

Para ESP32, `localhost` designa la propia placa. Preparar explícitamente hostname/IP de LAN autorizada, SAN correspondiente del certificado, enlace LAN del listener8883 y firewall. Mantener loopback por defecto cuando no se necesite acceso LAN; no abrir puertos del router. El puerto9001/WSS y la confianza CA de la app corresponden al cliente/app, no al transporte MQTT/TLS del ESP32.

El cliente verifica CA, hostname/SAN y fecha, con mbedTLS date validation habilitada explícitamente y comprobaciones de compilación que impiden TLS inseguro. Nunca recurre a MQTT claro ni omite verificaciones. Un fallo de DNS, asociación, autenticación o TLS deja error/desconexión y reintenta1,2,4…30 s desde el supervisor sin bucles de espera de reconexión. El stack de transporte tiene timeout acotado; un cambio de perfil espera la liberación antes de activar el siguiente. Si falla esa liberación queda error y no arranca otra red.

## MQTT y recuperación

MQTT5 usa Clean Start y Session Expiry0, con prefijo `sentinel/v1/nodes/<device_id>/`:

| Sufijo | QoS y retención |
| --- | --- |
| state | QoS1 retained, actualización cada5 s con uptime/revisión reales |
| availability | QoS1 retained, online al conectar y LWT offline |
| metrics | QoS0 sin retención, heap/journal reales cada5 s |
| events | QoS1 sin retención, IDs y cursor originales |
| commands | QoS1 sin retención; firmware rechaza flag retained |
| acks | QoS1 sin retención, accepted/applied/rejected correlacionados |

El cierre MQTT5 solicita publicación del Will offline también al salir limpiamente del perfil. Un estado retained antiguo no demuestra presencia; la app marca antigüedad según el límite15 s del contrato. El acuse QoS no equivale a aplicar una orden ni a recibirla en el dashboard.

Después de reconexión se reenvía el ring retenido con IDs originales. `read_events` permite recuperar páginas1…8 por BLE/MQTT con rango, gap y siguiente cursor; los eventos salen individualmente antes del ack aplicado. Repetir una página interrumpida con command ID nuevo y el cursor previamente confirmado. Nunca borrar por PUBACK: el ring sobrescribe por capacidad64, y la app deduplica por device_id/message_id.

Al cambiar Cebo→Escudo se detienen MQTT y STA antes de declarar el nuevo perfil. La confirmación final puede perderse por el canal anterior; la app consulta por BLE seguro la revisión y `last_command_id`. Sin estado/ack que demuestre aplicación, el resultado sigue pendiente. Sanitizador/captura/honeypot responden not_implemented y sus capacidades permanecen false.

## Comprobación física pendiente

No se detectó placa serie en WSL y el usuario pidió no utilizar Android por ahora. Por ello las tareas6.1–6.8 siguen abiertas aunque sus adaptadores y documentación estén implementados; tampoco se afirma pairing, SC/MITM negociado ni sesión MQTT del ESP32 como probados. Los tests host no reemplazan el canal real.

Cuando se autorice disponer de ese equipo: verificar rechazo de un cliente nuevo fuera de ventana; pairing con passkey y nivel negociado; metadata/get_state completo con MTU23; desconexión a mitad de comando; reloj y aprovisionamiento; TLS correcto y CA/hostname/fecha incorrectos; orden MQTT, duplicado y retained; cambio ida/vuelta de perfil y ausencia de asociación en Escudo; reboot y recuperación del evento persistido con mismo ID. Registrar resultados de placa/OLED/puerto real en firmware.md y en la comprobación integral antes de marcar esas tareas.
