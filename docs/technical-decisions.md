# Sentinel Node: decisiones técnicas y validaciones

**Acuerdo vigente (2026-11-17):** [MQTT anónimo, entrega P1 y reparto actualizado](mvp-policy.md). Sin TLS/cuentas/roles/ACL MQTT; BLE y HTTPS/DoH conservan sus protecciones. Evidencia histórica no sustituye las pruebas físicas pendientes.

**Política aprobada vigente:** [persistencia operativa y USB](persistence-policy.md), definida en [sentinel-mvp-standard-persistence](../openspec/changes/sentinel-mvp-standard-persistence/design.md). Conservar identidad, bonds, configuración e historial confirmado en uso normal/reinicio/corte; USB permite reinicialización sin migración. Arduino oficial primero; justificar cada excepción mínima. La tabla nueva/cuatro claves/configuración persistente se verifican en la [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); los builds anteriores son históricos y no cierran pruebas físicas.

## Cómo leer este registro

Este documento conserva decisiones y alternativas de la exploración y registra la estrategia de desarrollo acelerado aprobada por el usuario. Solo las instrucciones expresas y las aprobaciones existentes se presentan como acuerdos. Las etiquetas **Confirmada**, **Recomendación** e **Hipótesis pendiente de validar** tienen el significado definido en [arquitectura](architecture.md#estado-y-fuentes). El [roadmap](roadmap.md) propone cuándo resolverlas.

La referencia funcional es [brief.md](brief.md) y el PDF identificado en arquitectura. La instrucción del 2026-11-10 mantiene la ESP32-S3-DevKitC-1 N16R8 e incorpora Auditor/Sanitizador al desarrollo paralelo desde la primera ronda. La entrega comprende los tres módulos completos y la aplicación. El reparto y sus dependencias figuran en [encargos/README.md](encargos/README.md).

## Decisiones confirmadas y requisitos de referencia

| ID | Estado y procedencia | Decisión o requisito | Alcance de la confirmación |
| --- | --- | --- | --- |
| C-01 | Confirmada por el usuario | ESP32-S3-DevKitC-1, 44 pines, N16R8, WiFi USB-C. | Verificación rápida de la unidad y del pinout durante el arranque, sin estudio exhaustivo previo. |
| C-02 | Confirmada por el usuario, actualizada el 2026-11-10 | Tres módulos completos en desarrollo paralelo desde la primera ronda. | Entrega con Escudo R-03, Sanitizador R-08 y Cebo R-10, incluida aplicación y alertas externas. |
| C-03 | Confirmada por brief/PDF y usuario | Tres modos de producto; un modo activo a la vez y supervisor central. | Desarrollar Escudo y Cebo en paralelo no autoriza ejecutarlos simultáneamente en la misma radio. |
| C-04 | Confirmada por instrucciones y actualización del usuario | Separar firmware, broker, app y simulador; integración incremental sobre hardware real desde el inicio. | La indicación anterior de simular antes de hardware se sustituye como orden obligatorio. El simulador queda como apoyo ligero separado, no como prerrequisito ni evidencia de implementación real. |
| C-05 | Actualizada por el usuario el 2026-11-13 | Firmware con Arduino IDE Windows, core oficial ESP32 3.3.12, C/C++/FreeRTOS y dependencias mantenidas; Mosquitto/Docker y React/Vite/Tailwind/Zustand/Capacitor conservan su stack; BLE en Escudo y MQTT en Cebo. | Build completo oficial Windows PASS con dos bibliotecas mantenidas y opción local aprobada; el SDK propio queda como antecedente. Se conservan regresiones y pendientes físicos. PlatformIO/ESP-IDF directo permanece como antecedente de la base. |
| C-06 | Confirmada por fuentes/instrucciones y usuario | Seguridad, persistencia offline y pruebas de lógica crítica; solo redes autorizadas; sin secretos en el repositorio ni credenciales de terceros almacenadas. | Comprobaciones funcionales breves y pruebas focalizadas; rendimiento, autonomía y estabilidad prolongada se evalúan después, salvo fallo que impida funcionar. |
| C-07 | Confirmada por el usuario para esta tarea | Preparar documentación, propuestas OpenSpec y encargos con responsable único. | Cada integrante implementará los cambios que se le asignen después de revisar su contrato y diseño. |
| C-08 | Confirmada por el usuario y R-10 | Webhooks y push Android con recepción con la app cerrada, mediante componente de notificaciones separado e infraestructura necesaria. | Las dos integraciones son obligatorias en el MVP; proveedor push, alojamiento y detalles operativos se elegirán al abordar la integración, sin investigación extensa previa. |

OLED, botón y alimentación portátil se conservan como requisitos de referencia del PDF; no hay selección final de pines ni modificación del circuito aprobada. Las comprobaciones eléctricas necesarias se realizan al conectar los componentes.

## Decisiones, recomendaciones, alternativas y riesgos

### R-01. Framework y entorno de firmware

**Estado: Decisión actualizada y ejecución autorizada por el usuario el 2026-11-13.** Todo el firmware se desarrolla, compila, sube y monitoriza con Arduino IDE Windows. La instrucción de simplificación prioriza el core oficial ESP32 **3.3.12** y bibliotecas mantenidas, sustituyendo el SDK propio como objetivo. Conserva C/C++, FreeRTOS, supervisor/gestor de radio únicos, API, contratos, Escudo completo, seguridad y datos. Node es mantenimiento opcional y no un requisito para compilar/subir firmware.

**Antecedentes comprobados:** base PlatformIO + ESP-IDF 5.5.3 y posterior paquete Sentinel reconstruido para MQTT5, TLS con fechas, BLE SC-only/nivel4 y NVS sin formato automático. La [evidencia del SDK previo](../openspec/changes/sentinel-arduino-sdk/verification.md) y el [build Windows previo](../openspec/changes/sentinel-arduino-firmware-port/integration-verification.md) permanecen vigentes como registro de lo ejecutado; no prueban el core oficial sin esas adaptaciones.

**Evidencia histórica de software anterior a la tabla nueva:** [sentinel-arduino-dependencies](../openspec/changes/sentinel-arduino-dependencies/verification.md) integra wolfMQTT2.2.0 y ESP_SSLClient3.1.3 sobre el core oficial3.3.12. Build completo Windows **PASS exit0**, imagen1376726bytes y globalRAM160228bytes, sin referencias al SDK Sentinel en730depfiles. La preparación inicial usa install-wolfmqtt.ps1 e install-board-config.ps1 para descargar/verificar la biblioteca y combinar las nueve claves aprobadas con respaldos e idempotencia; ESP_SSLClient se instala desde el IDE. Después Verificar/Subir/Monitor usan Arduino normalmente. MQTT5 usa el worker acotado; TLS oficial comprueba identidad DNS/IP y BearSSL comprueba CA/cadena/fechas antes de autenticación. BLE exige SC-only/nivel4 por runtime y revisa datos previos sin escribir. Esta evidencia no acredita enlaces ni persistencia físicos.

**Decisión vigente confirmada por el usuario:** core oficial, hasta **tres teléfonos recordados**, tabla compatible con `boot_app0` y **cuatro claves**: menú, `build.partitions`, máximo4MiB y `--wrap=nvs_flash_init`. El instalador retira cinco overrides Sentinel obsoletos, con claves ajenas/respaldo/idempotencia; carga/exportación estándar. La primera carga es limpia desde el IDE; USB puede reinicializar identidad/bonds/configuración/historial sin migración. La protección del wrapper y el preflight BLE siguen evitando borrados silenciosos durante reinicios normales. Ver [tabla y motivos](persistence-policy.md).

**Condiciones vigentes:** MQTT5/TCP sin CA según R-04 actualizada; BLE SC autenticado/nivel4 y bonds, mismos GATT/framing y contratos; persistencia operativa según la tabla nueva aprobada. No bajar protocolos, omitir validaciones, quitar funciones o cambiar contratos para compilar. El arranque normal debe evitar borrado/formato automático de NVS y conservar identidad/bonds/configuración/historial confirmado; la reinstalación USB permite reinicialización sin migración.

**Riesgos y validación:** adaptación de transportes, configuración efectiva, inicialización y conservación de datos. Reutilizar pruebas críticas sin debilitar aserciones y acreditar el build oficial completo. El botón Verificar, subida/monitor y BLE/radio/NVS físicos mantienen sus pendientes; la falta de placa identificada no bloquea la preparación de software. Ver [plan y límites](firmware-arduino-migration.md).


### R-02. Supervisor y propiedad de recursos

**Estado: Recomendación revisada y aprobada.** Supervisor único para cambios de modo, gestor de radio central y colas acotadas; MQTT, BLE y botón solicitan transiciones mediante el mismo arbitraje.

**Alternativa evaluada:** cada modo administra Wi-Fi y sus transiciones directamente.

**Justificación:** evitar carreras, servicios residuales y órdenes simultáneas incompatibles.

**Riesgos y validación:** parada incompleta, bloqueo de callbacks y agotamiento de colas. Probar transiciones fallidas, comandos concurrentes y ráfagas. La API entregada detiene/libera antes de iniciar y aplica un timeout inicial de 5000 ms; ante fallo conserva error y no inicia recursos incompatibles. Ver el ciclo de vida en desarrollo.

### R-03. Captura Wi-Fi pasiva y comunicación BLE

**Estado: Decisión arquitectónica aprobada, pendiente de validación experimental en hardware.**

**Decisión:** El módulo Escudo Pasivo de Sentinel Node permitirá seleccionar y monitorear una red Wi-Fi específica desde la aplicación móvil, utilizando Bluetooth Low Energy (BLE) como canal de comunicación con el ESP32. La auditoría será completamente pasiva, sin necesidad de asociarse a la red seleccionada.

**Funcionamiento propuesto:**

1. **Descubrimiento de redes:** El ESP32 realizará un escaneo pasivo de redes Wi-Fi disponibles en la banda de 2.4 GHz, identificando SSID, BSSID, canal, intensidad de señal (RSSI) y características de seguridad anunciadas.
2. **Selección desde el dashboard:** El dispositivo enviará las redes detectadas a la aplicación mediante BLE. El usuario podrá seleccionar una red objetivo identificada por SSID y BSSID, sin proporcionar credenciales ni establecer una conexión Wi-Fi.
3. **Captura pasiva:** El ESP32 utilizará el modo promiscuo para analizar tramas de administración 802.11, incluyendo Beacon, Deauthentication y Disassociation. La captura priorizará la red seleccionada, manteniendo la posibilidad de recorrer otros canales para identificar anomalías relacionadas.
4. **Detección de amenazas:** Se analizarán patrones de desautenticación y desconexión, así como posibles ataques Evil Twin mediante la observación de otros puntos de acceso que anuncien el mismo SSID. Se compararán BSSID, canales, RSSI y configuraciones de seguridad. La coincidencia de SSID no se considerará evidencia suficiente de un ataque.
5. **Comunicación de resultados:** Los eventos, métricas y alertas se transmitirán mediante BLE a la aplicación móvil, donde se mostrarán en el dashboard. Se priorizará la visualización en tiempo real cuando las capacidades del hardware lo permitan.
6. **Gestión de captura:** El usuario podrá iniciar, detener o cambiar la red monitoreada desde la aplicación. El dispositivo mantendrá un registro de eventos para permitir su sincronización cuando BLE no esté disponible.

**Alternativas evaluadas:**

- **BLE y captura Wi-Fi concurrentes:** permite transmitir alertas mientras se monitorea la red, pero presenta riesgos de pérdida de tramas debido a la coexistencia de ambas tecnologías en el ESP32.
- **Captura con sincronización periódica por BLE:** permite almacenar y transmitir eventos en intervalos controlados, con posibles pausas de captura si resultan necesarias.
- **Conexión STA con MQTT:** descartada para este módulo porque requiere asociarse a una red Wi-Fi, modificando el comportamiento pasivo definido para el Escudo.

**Justificación:**

Esta arquitectura permite al usuario elegir qué red desea observar y consultar sus resultados sin conectarse a una infraestructura potencialmente insegura. La captura pasiva proporciona visibilidad sobre anomalías de capa de enlace, mientras que BLE facilita la configuración y consulta de resultados sin depender de internet.

El análisis de múltiples BSSID asociados a un mismo SSID amplía las posibilidades de identificar comportamientos compatibles con Evil Twin, sin asumir que todos los puntos de acceso duplicados son maliciosos.

**Riesgos y limitaciones:**

- La coexistencia entre BLE y captura Wi-Fi puede reducir la cobertura y provocar pérdida de tramas.
- El ESP32 utiliza una sola radio Wi-Fi, por lo que el recorrido de canales genera intervalos sin observación en los demás canales.
- La detección de Evil Twins es heurística y requiere establecer criterios que minimicen falsos positivos.
- La ausencia de eventos detectados no garantiza que la red sea segura.
- La detección de ataques Karma mediante observación exclusivamente pasiva tendrá alcance limitado y no permitirá confirmar todos los comportamientos de este tipo.
- La comunicación BLE deberá incorporar mecanismos de autorización para impedir el control del dispositivo por clientes no autorizados.

**Validaciones pendientes:**

- Medir la estabilidad de la captura con BLE activo.
- Evaluar pérdida de tramas y cobertura durante el recorrido de canales.
- Determinar si es viable transmitir eventos continuamente o si debe utilizarse sincronización periódica.
- Comprobar el almacenamiento y recuperación de eventos durante desconexiones BLE.
- Validar la detección de desautenticaciones y posibles Evil Twins mediante pruebas controladas.
- Definir umbrales de detección a partir de resultados experimentales.

**Restricciones arquitectónicas:**

El Escudo Pasivo no utilizará conexión STA, MQTT directo, NAT ni auditorías DNS/HTTPS. Estas funcionalidades corresponden a otros módulos de Sentinel Node.

La selección de una red mediante BLE no implica conectarse a ella. La implementación deberá conservar este principio y documentar cualquier interrupción temporal de la captura.

### R-04. MQTT anónimo del MVP en LAN de confianza

**Decisión confirmada por el usuario, 2026-11-17:** MQTT5/TCP1883 y WS9001 sin TLS/CA, usuarios, contraseñas MQTT, roles ni ACL. App y servicios usan endpoint e ID de nodo; configure_connectivity solo ssid/wifi_password/mqtt_uri. Se retiran preparación de cuentas, password_file y rol notify. Conservar MQTT5, QoS/retained/LWT, reconexión, registro persistente, ACK/dedup y validaciones de mensajes.

**Justificación:** reducir instalación y coordinación a Arduino oficial, app y Compose, usando IP LAN del PC sin certificados, cuentas, DNS especial o administración del router. El retiro no alcanza la autenticación BLE, permisos Android imprescindibles, credenciales Wi-Fi o TLS de DoH/HTTPS.

**Riesgo aceptado:** cualquier cliente con acceso al broker puede leer/publicar e intentar órdenes Cebo; no hay aislamiento ni autenticidad del emisor. device_id y los gates del protocolo no son autenticación. Uso solo en LAN de confianza propia/autorizada, sin exposición Internet. Firewall/aislamiento de clientes requieren una red utilizable; no se promete eludirlos.

**Alternativas sustituidas:** TLS/WSS con CA/nombre/fechas y acceso TCP/WS con usuarios/ACL. Sus informes son antecedentes fechados. Se descarta conservar campos MQTT vacíos, un login app o roles en servicios como sustituto de las cuentas retiradas. Ver [política vigente](mvp-policy.md), [broker](../broker/README.md) y [cambio integrado](../openspec/changes/sentinel-mvp-sin-cuentas/design.md).

### R-05. Contratos y entrega

**Estado: Recomendación aprobada.** Contratos versionados y eventos identificables; órdenes idempotentes con confirmación de resultado, revisión esperada y caducidad.

Los contratos aprobados describen topics bajo `sentinel/v1/nodes/{id}/` y `sentinel/v2/nodes/{id}/` según versión. El firmware actual usa v2 y conserva eventos históricos v1; ver [protocolo funcional](../contracts/module-protocol.md) y [conectividad](connectivity.md).


| Sufijo         | Política contractual                                              |
| -------------- | --------------------------------------------------------------- |
| `state`        | Estado retenido con revisión y antigüedad.                      |
| `availability` | Presencia y Last Will, complementados con última comunicación.  |
| `metrics`      | QoS 0, sin retención.                                           |
| `events`       | QoS 1, identificador y deduplicación.                           |
| `commands`     | QoS 1, sin retención y con vigencia verificable.                |
| `acks`         | Resultado de aplicación o rechazo, correlacionado con la orden. |


**Alternativas evaluadas:** mensajes sin esquema común; uso de retención para comandos; confiar solo en acuses del transporte.

**Justificación y riesgos:** QoS 1 permite duplicados; retención puede entregar mensajes a nuevos suscriptores. El acuse del broker no demuestra ejecución en firmware ni recepción por la app. La [especificación MQTT](https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html) distingue esas garantías. MQTT5, sesiones, frescura monotónica y comportamiento tras reinicio están descritos en los contratos y la base; no se promete ejecución exactamente una vez. La integración de nuevas bibliotecas debe conservarlos.

### R-06. Offline e historial

**Ampliación aprobada:** identidad, bonds BLE, configuración aceptada e historial confirmado persisten durante uso/reinicio/corte sin upload. Se sustituye explícitamente RAM-only para Wi-Fi/MQTT autorizados por NVS oficial con commit antes del ack aplicado, restauración validada en Idle sin conexión ni UTC obsoleto. USB puede reinicializar sin migración; no se promete cifrado físico. P2/P3 aplican esta política a configuraciones futuras y P4 reconcilia identidad/caché/cursores después de USB.

**Estado: Recomendación aprobada.** Registro circular persistente de eventos, recuperación por cursor, límites visibles y reconexión gradual. La implementación compartida conserva 64 eventos de hasta 2048 bytes en NVS; las métricas de salida son efímeras. La app común v2 conserva hasta 256 eventos por nodo en IndexedDB y usa Zustand para el estado de interfaz.

**Alternativas evaluadas:** solo RAM; confiar en persistencia MQTT; almacenamiento histórico central mediante servicio adicional.

**Justificación:** sobrevivir a desconexiones y acotar memoria/escrituras. Un broker no recupera eventos que nunca recibió ni constituye por sí mismo una API histórica.

**Riesgos:** desgaste, corrupción por corte eléctrico, sobrescritura y pérdida de eventos mientras la app no está disponible. La capacidad y política de sobrescritura/gap están documentadas en [firmware](firmware.md#registro-circular); las garantías físicas siguen pendientes y no se borra automáticamente para recuperar. Un servicio adicional central no está aprobado.

### R-07. Aprovisionamiento BLE

**Estado: Recomendación aprobada.** La base implementa ventana física de 120 s mediante pulsación de 3 s y pairing con código OLED, BLE SC/MITM/nivel4 y bonds persistentes; su comprobación física sigue pendiente. El destino oficial aprobado recuerda tres teléfonos y conserva/falla de forma segura ante cuatro vínculos previos, con cuatro claves mínimas y protección de arranque normal; primera carga de tabla nueva limpia y sin garantía de continuidad USB. Referencia: [aprovisionamiento seguro de Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/provisioning/provisioning.html).

**Excepción temporal aprobada para pruebas P1 sin OLED:** `SENTINEL_SERIAL_TEST_DISPLAY=1` permite mostrar estado y passkey temporal localmente por UART0 a 115200. Por defecto vale `0`. Conserva botón/ventana/SC/MITM/cifrado y OLED ausente en estado; si falla la entrega de todo display se rechaza pairing. UART no confirma que el operador esté leyendo: abrir monitor antes del intento. No registrar el código en evidencia pública, MQTT, historial ni logs persistentes. No sustituye el requisito OLED ni acredita sus pruebas. El panel Android temporal P1 fue retirado por decisión del usuario al entregar `sentinel-app-escudo`; sus controles normales están en la pantalla Escudo y las pruebas negativas permanecen en software. [Guía P1](guia-pruebas-persona-1.md#01-activar-la-salida-serie-sin-oled).

**Alternativas evaluadas:** plugin BLE del brief con protocolo propio; integración con mecanismos de aprovisionamiento existentes. Un plugin GATT no aporta automáticamente seguridad de aplicación.

**Riesgos:** compatibilidad del protocolo elegido con Capacitor, permisos y restricciones por plataforma. Android es la plataforma del MVP confirmada por el usuario; no se exige compatibilidad multiplataforma exhaustiva para integrar las funciones de R-07. El [plugin de referencia](https://github.com/capacitor-community/bluetooth-le) documenta soporte y diferencias de plataforma.

### R-08. Implementación incremental de NAT, filtrado DNS y DoH

**Planificación confirmada el 2026-11-10:** desarrollar Auditor/Sanitizador desde ahora en paralelo con Escudo y Cebo. H-04 y H-05 se comprueban durante la integración de la pasarela y DNS/DoH mediante pruebas funcionales breves.

Estado: alcance funcional y desarrollo paralelo confirmados. Los detalles técnicos de cada propuesta se revisan antes de implementar y su funcionamiento conjunto se valida en hardware.

Decisión: El modo Auditor y Sanitizador de Sentinel Node implementará progresivamente sus funciones de enrutamiento NAT IPv4, resolución y filtrado DNS local, y DNS over HTTPS (DoH).

La implementación se dividirá en auditoría, conectividad STA+AP con NAT, servidor DNS local con bloqueo de dominios e integración del cliente DoH. Cada incremento tiene una comprobación funcional breve; la campaña de carga y mediciones extensas se realiza después de la integración funcional, conforme a R-11.

Se utilizarán los componentes oficiales de Espressif como referencia inicial, priorizando la reutilización de soluciones existentes y compatibles con la placa seleccionada.

Políticas de funcionamiento:

- La conexión a una red pública se configurará mediante BLE y se validará antes de habilitar la protección.
- La red `Sentinel_Secure` utilizará autenticación Wi-Fi robusta y DHCP en una subred separada de la red pública.
- Las consultas DNS de los clientes conectados al SoftAP serán atendidas por un servicio local, encargado del filtrado y del reenvío mediante DoH.
- Las consultas DoH utilizarán validación de certificados TLS.
- Si DoH deja de funcionar, no se utilizará DNS sin cifrar como alternativa automática.
- Los portales cautivos se gestionarán como un estado especial de configuración, sin declarar que la conexión está protegida.
- Se comprobarán recuperación y errores básicos durante cada incremento. Las mediciones extensas de latencia, throughput, memoria y estabilidad se mantienen como validaciones posteriores, salvo un fallo que impida funcionar.
- La implementación inicial estará limitada a IPv4; no se anunciará protección IPv6 hasta implementarla y validarla.

Riesgos: limitaciones de memoria RAM, sobrecarga TLS, concurrencia de operaciones de red, conflictos de subred, pérdida de rendimiento STA+AP, fallos de resolución y limitaciones de los mecanismos de filtrado DNS.

Restricción: la existencia de ejemplos funcionales de NAT y DoH no constituye evidencia suficiente de su viabilidad simultánea en Sentinel Node. La aprobación de cada etapa dependerá de resultados reproducibles en hardware.

### R-09. Diagnóstico y lenguaje de alertas

**Estado: Recomendación aprobada.** Presentar indicios, evidencia y limitaciones. Validar certificados mediante cadena, hostname y vigencia; no comparar únicamente el certificado con uno visto previamente.

**Alternativas evaluadas:** considerar cualquier cambio de certificado o respuesta DNS como MITM; clasificar una MAC observada como autor probado; declarar toda la red segura tras una prueba exitosa.

**Justificación y riesgos:** rotación de certificados, CDNs, balanceo, DNS dividido y SSID legítimos repetidos pueden explicar diferencias. Evil Twin/Karma requieren heurísticas contrastadas; ráfagas de desautenticación tampoco prueban identidad. Estas funciones se mantienen, con falsos positivos y cobertura por medir.

### R-10. Cebo de red local, tratamiento de datos sensibles y sistema de alertas

**Cambio confirmado por el usuario el 2026-11-11:** Telegram queda fuera del alcance. Se conservan las alertas en la app, webhooks y push Android con recepción con la app cerrada. Esta instrucción sustituye únicamente el requisito anterior de ese canal; no lo aplaza al backlog.

**Estado: Alcance funcional aprobado por la instrucción vigente del usuario; comprobación funcional durante la implementación.**

**Aclaración incremental:** HTTP, Telnet, detección y clasificación, persistencia y sincronización, MQTT, historial, webhooks y push con la app cerrada se integran en iteraciones pequeñas dentro del mismo MVP. Las validaciones pendientes de esta sección se atienden conforme se implementa cada función; las pruebas prolongadas se reservan para después del MVP. No se modifica su política de datos ni de seguridad.

**Decisión:** El módulo Cebo de Sentinel Node implementará un honeypot ligero orientado a detectar intentos de acceso no solicitados dentro de una red local autorizada. Se desarrollará incrementalmente, comenzando con un panel HTTP ficticio, incorporando posteriormente un servicio Telnet simulado y un sistema de alertas locales y externas.

**Funcionamiento propuesto:**

1. **Servicio HTTP ficticio:** El ESP32 expondrá un panel de administración simulado, sin funcionalidades administrativas reales. Registrará conexiones, accesos a rutas conocidas e intentos de autenticación, sin almacenar credenciales ni cuerpos de solicitudes que puedan contener información sensible.
2. **Servicio Telnet simulado:** Se implementará un servicio TCP en el puerto 23 que emule la presencia de un servicio antiguo, sin proporcionar acceso a una consola, ejecutar comandos ni solicitar o conservar contraseñas reales.
3. **Registro de eventos:** Se conservarán metadatos como dirección IP de origen, puerto de destino, protocolo, marca de tiempo, tipo de interacción y cantidad de intentos. Los campos recibidos se validarán y limitarán antes de registrarse.
4. **Clasificación de actividad:** Los eventos se clasificarán según su comportamiento y frecuencia, diferenciando conexiones aisladas, intentos de acceso y actividad repetitiva sospechosa. Una conexión individual no constituirá por sí sola evidencia de compromiso.
5. **Comunicación MQTT:** Los eventos y alertas se publicarán mediante MQTT para su visualización en el dashboard. Se establecerá un contrato compartido que permita agregar posteriormente otros canales de notificación.
6. **Alertas externas:** Se mantendrán webhook y notificaciones push como objetivos del proyecto. Su implementación se realizará mediante un componente de notificaciones separado, con gestión segura de credenciales y mecanismos para evitar alertas repetitivas.
7. **Funcionamiento offline:** Cuando no exista conexión con el broker MQTT, los eventos se conservarán temporalmente en un búfer de capacidad limitada y se sincronizarán al restablecer la comunicación.

**Política de tratamiento de datos:**

- No almacenar credenciales reales, tokens, cookies de sesión ni otros secretos recibidos.
- No registrar cuerpos completos de solicitudes HTTP ni entradas completas de sesiones Telnet.
- Permitir pruebas con datos sintéticos exclusivamente en entornos controlados.
- Aplicar límites de tamaño, validación y sanitización a todos los datos recibidos.
- Definir límites de almacenamiento, retención y acceso al historial de eventos.

**Medidas de seguridad:**

- El honeypot funcionará únicamente en redes propias o expresamente autorizadas.
- Los servicios simulados no ejecutarán comandos ni otorgarán acceso administrativo.
- Se limitarán conexiones simultáneas, tiempos de espera y tamaño de las solicitudes.
- No se habilitará exposición pública a internet ni redirección de puertos hacia el Cebo.
- Al cambiar de modo operativo, se cerrarán los servidores y conexiones del Cebo.
- Las credenciales utilizadas para MQTT, push o webhooks no se incluirán en el código fuente ni se mostrarán en logs.

**Justificación:**

El propósito del módulo es detectar interacciones sospechosas dirigidas a servicios ficticios, proporcionando visibilidad temprana sobre posibles exploraciones o intentos de movimiento lateral dentro de la red.

La conservación de metadatos permite identificar patrones sin recopilar contraseñas ni información innecesaria. La separación de los mecanismos de detección, almacenamiento y notificación facilita el mantenimiento y reduce la complejidad del firmware.

**Riesgos y limitaciones:**

- Los escáneres legítimos y herramientas de administración pueden generar falsos positivos.
- El Cebo solo detectará actividad dirigida a sus propios servicios; no monitoreará todo el tráfico de la red.
- Solicitudes malformadas o conexiones excesivas pueden agotar recursos del ESP32.
- La ausencia de internet puede impedir las alertas externas.
- La disponibilidad de marcas de tiempo precisas dependerá de la sincronización del reloj.
- Las notificaciones push con la aplicación cerrada requerirán infraestructura adicional.

**Validaciones pendientes:**

- Comprobar el funcionamiento de HTTP y Telnet simulados en el ESP32.
- Medir memoria disponible y estabilidad ante múltiples conexiones.
- Validar que no se almacenen accidentalmente datos sensibles.
- Determinar umbrales de clasificación y mecanismos para reducir alertas repetitivas.
- Probar almacenamiento temporal y sincronización después de desconexiones.
- Evaluar el funcionamiento de MQTT, webhook y notificaciones móviles en escenarios con y sin conectividad.

**Restricción arquitectónica:** El módulo Cebo no implementará vulnerabilidades reales ni proporcionará acceso al sistema. Su funcionamiento estará limitado a la simulación de servicios, detección de interacciones y generación de eventos de seguridad.

### R-11. Entrega completa de tres módulos con desarrollo acelerado

**Estado: Decisión y orden de desarrollo confirmados por el usuario.** Se sustituye el criterio anterior que permitía cerrar un MVP parcial y dejar detectores, Telnet o alertas externas para después.

**Decisión actualizada el 2026-11-10:** desarrollo iterativo sobre el ESP32 real con los módulos 1, 2 y 3 completos y su aplicación Android integrada. Los tres responsables comienzan en paralelo y cada cambio tiene un autor. Los contratos y puntos de integración compartidos permiten incorporar incrementos pequeños.

#### Funcionalidades obligatorias del MVP

**Módulo 1 — Escudo Pasivo, conforme a R-03:**

- Escaneo pasivo de redes de 2.4 GHz; identificar SSID, BSSID, canal, RSSI y seguridad anunciada; enviar redes a la app por BLE.
- Selección de objetivo por SSID/BSSID e inicio, detención y cambio de red desde la app.
- Captura promiscua real; análisis Beacon, Deauthentication y Disassociation; prioridad al objetivo y recorrido de otros canales para anomalías relacionadas.
- Detectar ráfagas de desautenticación y desconexiones sospechosas; heurísticas Evil Twin comparando BSSID del mismo SSID, canal, RSSI y seguridad.
- Identificar indicios compatibles con Karma observables pasivamente y documentar sus límites. La falta de observación concluyente no elimina el detector ni justifica atribuir un ataque.
- Eventos, métricas y alertas por BLE; dashboard, gráficos de actividad y volumen, tabla de incidentes con red/canal/tiempo.
- Registro durante desconexiones BLE y sincronización posterior, conforme a la base persistente de R-06; OLED y botón.
- Sin asociación Wi-Fi ni MQTT directo, NAT o auditoría DNS/HTTPS en Escudo. No clasificar SSID duplicados automáticamente como maliciosos; presentar indicios y periodos sin cobertura.

**Módulo 3 — Cebo de Red Local, conforme a R-10:**

- STA en red autorizada, panel HTTP ficticio y Telnet simulado en puerto 23; detectar conexiones, accesos e intentos de autenticación sin ejecutar comandos ni ofrecer administración real.
- Registrar IP de origen, puerto, protocolo, fecha/hora, calidad temporal, tipo de interacción, cantidad y frecuencia; clasificar comportamiento y sospecha, incluida actividad repetitiva.
- Historial de eventos/incidentes, métricas y alertas MQTT en tiempo real hacia el dashboard; alertas visibles en la app.
- Integraciones funcionales con webhooks **y** push Android con recepción con la app cerrada, incluyendo infraestructura necesaria y control de alertas repetitivas.
- Almacenamiento local persistente, búfer circular limitado, conservación de pendientes, recuperación y sincronización MQTT con identificadores y deduplicación.
- Límites de conexiones, tiempos y tamaños; sin exposición de HTTP/Telnet a Internet ni redirección pública de puertos. No almacenar contraseñas, tokens, cookies, credenciales reales ni cuerpos completos de solicitudes.

**Infraestructura obligatoria compartida:** Arduino IDE Windows con core oficial ESP32 3.3.12 y dependencias mantenidas (build oficial completo PASS; hardware pendiente), C/C++/FreeRTOS y APIs ESP-IDF; supervisor central, gestión Wi-Fi/BLE y transición segura; OLED/botón; errores y reconexiones; contratos versionados, BLE autenticado/cifrado y MQTT anónimo en LAN, confirmaciones de comandos y deduplicación. React/Vite/Tailwind/Zustand con Capacitor Android, configuración/selección de redes, gráficos, incidentes, historial y notificaciones. El control de modo usa el canal disponible: BLE para Escudo; MQTT para Cebo. La pérdida del canal al cambiar de modo se gestiona sin anunciar éxito antes de aplicar la transición.

**Módulo 2 incorporado a la entrega actual, conforme a R-08:** diagnóstico DNS/HTTPS/portal cautivo, STA+AP con autenticación y DHCP, NAT IPv4 y subredes separadas, clientes/RSSI/tráfico, DNS local con filtrado y DoH con verificación TLS y recuperación. Su línea de implementación comienza en paralelo. Las comprobaciones funcionales de R-08 forman parte del cierre conjunto.

#### Orden confirmado y división de trabajo

1. **Base compartida:** arranque real, supervisor, MQTT, BLE, contratos y dashboard conectados por incrementos pequeños.
2. **Escudo funcional completo:** descubrimiento, selección, captura, todos los detectores, registro, sincronización y visualización.
3. **Cebo funcional completo:** HTTP/Telnet, detección, clasificación, persistencia, MQTT, historial, webhook y push móvil.
4. **Integración completa:** transiciones, recursos, OLED/botón con alertas recientes, recuperación y comprobación funcional final.
5. **Sanitizador:** auditoría, pasarela IPv4, DNS local y DoH conforme a R-08, en paralelo desde la primera ronda y antes de cerrar la entrega conjunta.

Las líneas de Escudo, Sanitizador, Cebo y aplicación avanzan en paralelo sobre los contratos y servicios compartidos. OLED, botón y recuperación se conectan desde las primeras iteraciones y se comprueban en la integración de los tres modos.

Cada etapa puede abarcar varios cambios OpenSpec pequeños con requisitos y tareas concretos. Un incremento parcial puede ser demostrable, pero no se denomina MVP terminado. El simulador se conserva separado como apoyo ligero; no precede obligatoriamente al hardware ni satisface un requisito que deba funcionar en el ESP32.

#### Validaciones durante el desarrollo

Aplicar a cada incremento: implementar, compilar, hacer una comprobación funcional breve, corregir errores bloqueantes y continuar. Utilizar capacidades de ESP-IDF y componentes oficiales cuando sean adecuados. Mantener pruebas automatizadas focalizadas para lógica crítica, sin montar infraestructura de pruebas compleja.

Intentar primero captura/BLE concurrentes. Si aparecen problemas, usar sincronización periódica o pausas controladas como permite R-03, conservando todas las funciones e indicando interrupciones. Usar heurísticas iniciales razonables en Escudo/Cebo y refinarlas ante problemas observados. Resolver persistencia con una solución sencilla, acotada y adecuada al ESP32.

No exigir campañas experimentales, benchmarks, desgaste prolongado, carga sostenida, autonomía u optimización antes de empezar o cerrar la integración funcional. Si un fallo de recursos impide funcionar, resolverlo de forma focalizada. Las mediciones y límites aún desconocidos quedan documentados sin prometer garantías.

#### Criterio de finalización

La entrega termina cuando **todas** las funciones obligatorias de los tres módulos funcionan en el ESP32 y la app Android, con sus transportes e infraestructura reales. Incluye comprobación breve de webhook, recepción push con la app cerrada, pasarela IPv4 y filtrado/DoH. Cada resultado se acredita en el equipo pertinente.

Comprobar también persistencia ante reinicio breve, desconexión/reconexión, recuperación/deduplicación y exclusión de modos. Mocks, simulación, capturas de interfaz o una única categoría de alerta no acreditan un módulo completo.

Las pruebas exhaustivas de compatibilidad/segundo plano, rendimiento, carga prolongada, estabilidad, consumo y optimización son mejoras posteriores; no funcionalidades pendientes del MVP. La ausencia de Internet puede impedir alertas externas inmediatas y debe mostrarse, sin retirar sus integraciones.

**Justificación:** repartir los tres módulos entre sus responsables y resolver las incertidumbres técnicas durante incrementos funcionales, con contratos comunes y comprobaciones breves.

**Riesgos vigentes:** contención BLE/captura, falsos positivos, recursos de servidores y almacenamiento, sincronización entre transportes y entrega de notificaciones. H-01 a H-08 registran tratamiento y comprobaciones mínimas, no una fase de investigación bloqueante.

## Evidencia técnica y sus límites

Las fuentes externas son referencias de viabilidad, no resultados de pruebas propias. Las versiones citadas no fijan la versión del firmware.


| Referencia                                                                                                                       | Hecho documentado                                                                                       | Consecuencia para Sentinel                                                                                     |
| -------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html) | N16R8: 16 MB flash y 8 MB PSRAM. Hay restricciones de GPIO por memoria octal y diferencias de revisión. | Más margen nominal de memoria; comprobar configuración, pinout y recursos internos. No resuelve contención RF. |
| [Coexistencia ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-guides/coexist.html)                     | Wi-Fi/BLE comparten radio; sniffer con BLE figura con rendimiento inestable.                            | Comprobar captura/BLE al integrar; aplicar sincronización o pausas si falla. Medición exhaustiva posterior.                                                             |
| [Wi-Fi ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/api-guides/wifi.html)                               | STA+AP comparte canal con prioridad STA; captura concurrente afecta throughput.                         | Medir reconexiones y carga; no prometer cobertura multicanal continua durante NAT.                             |
| [RFC 8484](https://www.rfc-editor.org/info/rfc8484/)                                                                             | DoH cifra consultas y dificulta inspección DNS en tránsito.                                             | El DoH propio de clientes puede eludir el filtro local. DoH no cifra todo el tráfico ni garantiza anonimato.   |


El usuario aportó una [imagen de pinout](architecture.md#pinout-aportado-por-el-usuario), conservada en el repositorio como referencia. Este aporte está confirmado; su correspondencia con la revisión física y la asignación de periféricos siguen pendientes. La imagen expone GPIO35–37, pero su presencia en el conector no elimina las restricciones de memoria octal indicadas por Espressif para N16R8.

La descripción USB-C y la imagen no bastan para atribuir a la unidad todos los detalles de una revisión oficial. Quedan pendientes comprobación física y esquema del fabricante de la unidad concreta. La base usa valores configurables SDA GPIO8, SCL GPIO9 y botón GPIO4; su presencia en configuración no acredita cableado ni validación física. Ver [firmware](firmware.md).

## Hipótesis H-01 a H-08: tratamiento durante la implementación

La dirección de desarrollo y el alcance están confirmados; los resultados de hardware siguen pendientes. Estas hipótesis se resuelven con comprobaciones breves dentro del incremento correspondiente. No son requisitos de investigación previos ni autorizan eliminar funcionalidades.

| ID | Decisión o tratamiento confirmado | Comprobación funcional mínima y momento | Pendiente posterior o condicionado |
| --- | --- | --- | --- |
| H-01 | Configurar inicialmente ESP32-S3 N16R8 y verificar rápidamente la unidad. | En etapa 1: compilar, grabar, arrancar, comprobar detección de flash/PSRAM, alimentación y pines realmente utilizados. Corregir incompatibilidades concretas. | Estudio exhaustivo de memoria, consumo y autonomía después de la integración; no exigirlo para comenzar. |
| H-02 | Arduino IDE Windows con core oficial ESP32 3.3.12 y dependencias mantenidas; sustituye el SDK propio como objetivo. | Configuración/instalación entregadas y [build oficial completo Windows PASS](../openspec/changes/sentinel-arduino-dependencies/verification.md), con wolfMQTT2.2.0 y NetworkClient oficial; ESP_SSLClient exclusivo de MQTT fue retirado; Node es mantenimiento opcional. SDK propio y base PlatformIO son evidencia histórica. Selección/Verificar visual y subida/BLE/MQTT/NVS físicos siguen pendientes. | Resolver incompatibilidades sin reducir Escudo, contratos, datos ni seguridad; no esperar benchmarks NAT/DoH para avanzar. |
| H-03 | Intentar captura Wi-Fi/BLE concurrentes; si presentan problemas, sincronización periódica o pausas controladas conservando el Escudo completo. | En etapa 2: descubrimiento, selección/control y recepción de eventos reales; desconectar/reconectar BLE y recuperar pendientes. Mostrar intervalos sin captura. | Medición exhaustiva de pérdidas, cobertura y estabilidad después; no bloquear detectores ni exigir captura continua como garantía no demostrada. |
| H-04 | Implementar STA+AP/NAT IPv4 desde la línea paralela del Sanitizador. | Integrar un cliente real, comprobar DHCP, salida IPv4, subredes separadas, contadores y parada de la pasarela según R-08. | Rendimiento sostenido y optimización posteriores a la integración funcional. |
| H-05 | Implementar DNS local, filtrado y DoH en los incrementos del Sanitizador. | Comprobar resolución, bloqueo, CA/hostname/fecha y fallo de DoH con error DNS visible, conforme a R-08. | Estabilidad prolongada, rendimiento y afinado posteriores a la integración funcional. |
| H-06 | Implementar todas las detecciones de Escudo y Cebo con heurísticas iniciales razonables y refinarlas al observar problemas. | En etapas 2 y 3: ejemplos breves benignos/sospechosos, Beacon/Deauth/Disassociation, ráfagas, Evil Twin, indicios Karma pasivos y actividad repetitiva HTTP/Telnet. Pruebas focalizadas para parser, clasificación y límites. | Evaluación estadística y ajuste fino después; no eliminar detectores por falta de benchmarks ni convertir SSID duplicado en veredicto malicioso. |
| H-07 | Persistencia local sencilla y acotada, búfer circular y sincronización con identificación/deduplicación, conforme a R-06 y R-10. | Reutilizar journal NVS de64 eventos: probar reinicio y corte sin upload tras commit, recuperando identidad/bonds/configuración/historial; Idle sin conexión/reloj anterior y dedup/gap. Separar primera carga limpia USB y reconfiguración; no exigir migración entre versiones. | Desgaste y resistencia prolongada después. Los nuevos módulos no crean otro almacén ni requieren microSD; capacidad finita y pérdidas visibles. |
| H-08 | Android con Capacitor, BLE, MQTT, dashboard, historial y notificaciones; webhooks y push son parte del MVP. | Teléfono real en etapas 1–4: permisos Android/BLE y flujo completo, MQTT anónimo sin login/roles; comprobar cada integración externa y recepción push con la app cerrada. Configurar infraestructura mínima real, no mocks. | Compatibilidad extensa y campañas de segundo plano/estabilidad después. La comprobación mínima con app cerrada sí es obligatoria antes de cerrar el MVP. |

No se han fijado garantías de Mbps o autonomía. La base tiene límites de colas/journal y los nuevos diseños proponen límites adicionales; estos son cotas de implementación, no mediciones de capacidad o estabilidad. Ajustarlos ante fallos básicos, sin crear una ronda previa de benchmarks.

## Política de pruebas y pendientes

**Obligatorio durante cada incremento:** compilación, conectividad y comprobación funcional breve del comportamiento implementado; corregir bloqueantes y mantener pruebas sencillas de lógica crítica. Verificar autenticación, validación de entradas, datos sensibles, exclusión de modos, persistencia y deduplicación de forma focalizada.

**Posterior al MVP:** rendimiento, autonomía, carga y estabilidad prolongadas, optimización, compatibilidad extensa y refinamiento estadístico de heurísticas. No se requiere una campaña de esta clase para comenzar.

**Trabajo paralelo confirmado:** NAT/DNS/DoH se validan en los cambios del Sanitizador. Las comprobaciones descritas en R-03, R-06, R-07 y R-10 se integran en los encargos de sus responsables conforme a R-11.

## Tratamiento de cambios respecto al PDF

**Revisión confirmada el 2026-11-11:** corregir la asignación de alertas recientes en OLED conforme a la página 4 del PDF, a cargo de Persona 1 en `sentinel-integracion-tres-modos`, tarea 1.5. Mostrar un resumen de la alerta más reciente sin depender de abrir la app, reutilizando el flujo central y sin añadir otro almacén. El resto del plan se conserva y sus diferencias se registran en la [comparación de alcance](pdf-scope-comparison.md), distinguiendo adaptaciones de limitaciones no demostradas. Esta revisión no añade métricas/campos ni otros requisitos funcionales.

La instrucción vigente del 2026-11-10 incluye los tres módulos completos desde la primera ronda de trabajo. Sustituye la priorización anterior que difería el Módulo 2; no recorta funciones de Escudo/Cebo. El orden de hardware real desde el inicio sustituye la prioridad temporal de simulación descrita en el brief; el componente simulador se conserva separado.

Los cambios OpenSpec existentes conservan sus tareas y evidencia. Esta actualización de documentación no acredita implementación nueva ni completa pendientes físicos.

Si una función no cumple las pruebas, registrar resultado, condiciones, impacto y alternativa propuesta antes de pedir revisión del alcance. No retirarla silenciosamente ni presentarla como opcional por conveniencia. La restricción de no almacenar credenciales de terceros prevalece sobre el registro sensible descrito en el PDF.
