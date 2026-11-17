# Conectividad de la base

El firmware comparte GATT protegido y gestión de radio; STA/MQTT se permiten únicamente en Cebo cuando su módulo esté disponible. Escudo implementa descubrimiento/captura pasivos y detectores, sin asociación, escaneo activo ni MQTT. El firmware completo compiló con core oficial ESP32 3.3.12 y bibliotecas mantenidas; la evidencia del transporte anterior se conserva por separado. Consultar [firmware.md](firmware.md) para cableado, build, puertos y evidencia; [contracts/README.md](../contracts/README.md) define los mensajes exactos; [broker/README.md](../broker/README.md) prepara el laboratorio TLS.

## Pairing y control local

1. Comprobar alimentación, OLED y botón; abrir el monitor a115200. Arranca idle con revisión0 y un boot ID nuevo, sin conectarse a Wi-Fi. La ausencia de OLED impide nuevos pairings.
2. Mantener el botón3 s. Se abre una ventana física de120 s; liberar no cambia el perfil. No hay una operación remota para abrirla.
3. En una app Android compatible, conectar al servicio `a90d0001-6d64-4f43-9e6d-9db51f1f0000` y completar la passkey de seis cifras mostrada en OLED. El firmware exige LE Secure Connections, MITM, cifrado de128 bits y bonding; no admite Just Works ni legacy. El código no aparece por serie, historial ni MQTT.
4. Fuera de la ventana solo se aceptan pares previamente autorizados. El core oficial aprobado recuerda hasta **tres teléfonos**. Antes de iniciar el host BLE, un preflight de solo lectura detecta cuatro vínculos anteriores, formato incompatible o errores de lectura: informa el problema y deja BLE detenido sin sobrescribirlos. No borra automáticamente el más antiguo ni ningún otro. Si se llena, el intento falla y requiere gestión explícita posterior; ninguna pulsación borra bonds o historial. Esta preservación está comprobada por software; reutilizar vínculos en una placa real sigue pendiente.
5. Leer metadata protegida en `a90d0002-…` (hasta256 bytes), suscribirse a indications protegidas `a90d0004-…` y escribir con respuesta en `a90d0003-…`. Leer metadata mediante una lectura larga coherente si MTU es23. El estado completo se pide con `get_state`.

Metadata contiene device_id, boot_id, revisión, uptime y máximo2048. Todos los comandos identifican nodo/arranque destinatarios, revisión esperada y un vencimiento futuro dentro de30 s de uptime del dispositivo; no necesitan UTC. Se verifican autorización y vigencia antes de consultar la caché32. Un duplicado lógico vigente usa digest SHA256 y resultado sanitizado; el mismo ID con otros argumentos falla `id_conflict`. Cambiar campos de transporte no produce un segundo efecto. Consultas no incrementan revisión.

Cada mensaje lógico máximo2048 bytes UTF-8 se fragmenta con cabecera LE de8 bytes: transfer ID32, offset16 y total16. MTU23 deja12 bytes de datos por fragmento; los offsets son contiguos, el timeout total es10 s y solo hay una transferencia por dirección/conexión. Indications se secuencian esperando su confirmación ATT. Desorden, exceso, timeout o desconexión descartan el parcial; un parcial nunca se ejecuta. Tras reconectar se consulta estado y se envía un comando completo vigente.

## Aprovisionamiento privado y reloj

Mediante BLE autenticado, con metadata recién leída:

1. Enviar `set_time` con `args.timestamp` UTC ISO8601. Usar hora confiable del operador; la calidad pasa a `app_sync`. El reloj desconocido mantiene MQTT bloqueado y visible como error; no se inicia Wi-Fi en Escudo para obtener hora. Fecha, CA y nombre se verifican durante TLS.
2. Consultar metadata o state otra vez porque aplicar hora cambia revisión. Enviar `configure_connectivity` con los campos planos del contrato: SSID de red propia, contraseña Wi-Fi, `mqtts://host[:puerto]`, usuario/password MQTT y CA PEM pública. El URI no lleva credenciales, ruta ni query. La CA debe ser un certificado CA válido y todo el mensaje debe caber en2048 bytes; usar una CA compacta de laboratorio si hace falta.
3. No devolver los argumentos privados: accepted/applied/rejected y estado solo exponen resultado/capacidades/errores. El cliente debe descartar formularios privados después del envío.
4. Aprovisionar preferentemente en Idle: no abre una conexión de red. Escudo rechaza la configuración de conectividad. Consultar estado actualizado y pedir `set_mode` Cebo cuando su módulo esté disponible. El perfil se confirma cuando los recursos están preparados; STA/MQTT continúan conectando asíncronamente. Reconfigurar mientras el worker MQTT está activo devuelve `busy` antes de tocar radio, propietario o credenciales: solicitar Idle, esperar su confirmación y repetir la configuración. Con el transporte ya detenido se aplica el camino de configuración normal.

La excepción anterior **solo RAM queda sustituida** por la [política aprobada](persistence-policy.md). SSID/contraseñas/endpoint/CA autorizados se guardan mediante NVS oficial con límites y validación. `configure_connectivity` solo confirma `applied` después de escritura/commit; un `accepted` no acredita persistencia. Un fallo informa error y conserva la configuración anterior. Al reiniciar, se valida/restaura en Idle sin conectarse ni iniciar módulos. Ausencia significa no configurado; corrupción/error se informa sin formatear ni borrar bonds/historial. Hay que renovar la referencia UTC: no se confía en hora guardada antes del boot.

Wi-Fi conserva `WIFI_STORAGE_RAM` para evitar guardado implícito por el driver; la persistencia explícita vive en NVS. No se promete cifrado físico de flash. Los cuerpos privados en cola/cJSON se limpian después de procesarlos; la caché guarda digest/resultado, nunca el comando completo. No guardar credenciales recibidas por Cebo/terceros ni secretos en Git, logs, eventos o estado público. `configure_connectivity` y `set_time` por MQTT siguen rechazados como unauthorized.

Identidad, bonds, configuración e historial confirmado sobreviven a reinicios/cortes normales; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de la nueva tabla: **Erase All Flash Before Sketch Upload: Enabled**, después emparejar/configurar. Tras USB, reconciliar historial/cursores en la app aunque el device_id derivado de MAC siga igual; retirar vínculos/caché obsoletos del teléfono si fueron reinicializados. Ver [carga inicial y recetas estándar](firmware.md).

## Broker/TLS y acceso LAN

Generar material local bajo `broker/.secrets/` siguiendo su README. La CA pública se copia al comando protegido; claves privadas y passwords permanecen en rutas ignoradas. La identidad MQTT del nodo debe tener ACL para su device_id real, obtenido en metadata o consola, y para ningún otro nodo. El client ID se deriva del dispositivo; la autenticación utiliza la cuenta del broker.

Para ESP32, `localhost` designa la propia placa. Preparar explícitamente hostname/IP de LAN autorizada, SAN correspondiente del certificado, enlace LAN del listener8883 y firewall. Mantener loopback por defecto cuando no se necesite acceso LAN; no abrir puertos del router. El puerto9001/WSS y la confianza CA de la app corresponden al cliente/app, no al transporte MQTT/TLS del ESP32.

El transporte usa wolfMQTT2.2.0 para MQTT5. `NetworkClientSecure` oficial valida CA e identidad DNS/IP de la conexión; después el motor BearSSL de ESP_SSLClient3.1.3 comprueba la vigencia de la CA candidata y valida esa misma cadena DER con hora explícita del nodo. Se necesita una ruta completa válida; una CA ajena vencida dentro de un bundle no invalida otra ruta correcta. Ambas comprobaciones deben terminar antes de enviar cualquier byte MQTT o credencial. Peer ausente, reloj desconocido, nombre/CA/fechas incorrectos cierran el canal sin autenticación MQTT. No hay alternativa plana ni verificaciones omitidas. La composición conserva SAN de IP, que el cliente BearSSL directo no admite en esta versión.

DNS, TCP/TLS y los ACK se atienden en un worker y colas acotadas. Un fallo de DNS, asociación, autenticación o TLS deja error/desconexión y reintenta1,2,4…30 s sin bloquear botón, OLED ni supervisor. Un cambio de perfil solicita el cierre y conserva al propietario anterior hasta confirmar la liberación, dentro del plazo compartido de5000 ms. Si falla esa liberación queda error y no arranca otra red. Las pruebas focalizadas del worker se registran en [dependencias](../openspec/changes/sentinel-arduino-dependencies/verification.md); no sustituyen la sesión TLS física del ESP32.

## MQTT y recuperación

MQTT5 conserva Clean Start y Session Expiry0. El firmware actual publica mensajes nuevos bajo `sentinel/v2/nodes/<device_id>/`; los eventos históricos v1 se recuperan bajo `sentinel/v1/nodes/<device_id>/` con formato e identidad intactos. Ver [protocolo v2](../contracts/module-protocol.md). El número de versión Sentinel y la versión MQTT son independientes:

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

Al cambiar Cebo→Escudo se detienen MQTT y STA antes de declarar el nuevo perfil. La confirmación final puede perderse por el canal anterior; la app consulta por BLE seguro la revisión y `last_command_id`. Sin estado/ack que demuestre aplicación, el resultado sigue pendiente. Escudo anuncia descubrimiento/captura/detectores reales; Sanitizador y Cebo siguen no disponibles mientras sus módulos no estén registrados. Una capacidad ausente se rechaza sin servicios ficticios.

## Comprobación física pendiente

El usuario autorizó usar ESP32 y Android, pero no se identificó placa USB/COM: Windows solo mostró COM3/COM4 Bluetooth y ADB no mostró teléfono. Las tareas físicas heredadas siguen pendientes; no se afirma pairing, SC/MITM negociado, sesión MQTT ni persistencia física como probados. La compilación oficial completa y los tests host no reemplazan el canal real.

Cuando se disponga del equipo identificado: verificar rechazo de un cliente nuevo fuera de ventana; pairing con passkey y nivel negociado; metadata/get_state completo con MTU23; desconexión a mitad de comando; reloj y aprovisionamiento; TLS correcto y CA/hostname/fecha incorrectos; orden MQTT, duplicado y retained; cambio ida/vuelta de perfil y ausencia de asociación en Escudo; reinicio/corte sin upload y recuperación de identidad, bonds, configuración validada e historial con mismo ID; Idle sin asociación y UTC desconocido. Separar la primera carga limpia y la reinstalación USB con pérdida permitida de esas pruebas de conservación. Registrar resultados de placa/OLED/puerto real en firmware.md y en la comprobación integral antes de marcar esas tareas.
