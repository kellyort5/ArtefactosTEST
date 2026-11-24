# Design

## Context

Ver [proposal.md](proposal.md) para motivación y alcance. El inventario OpenSpec estaba vacío y el repositorio solo tenía documentación, instrucciones y configuración inicial; no hay implementación o tests que reutilizar. Las siete [specs del cambio](specs/) definen comportamientos observables nuevos.

Fuentes de verdad: [arquitectura](../../../../docs/architecture.md), [decisiones e hipótesis](../../../../docs/technical-decisions.md) y [roadmap](../../../../docs/roadmap.md). Las instrucciones vigentes priorizan hardware real y desarrollo incremental sobre el orden previo de simulación del brief/config inicial. No se altera el alcance completo del MVP R-11.

Este diseño es una propuesta para revisión. Las decisiones ya aprobadas se distinguen de los detalles concretados aquí; no afirma que se hayan probado hardware o bibliotecas.

## Goals / Non-Goals

**Goals:**

- Proporcionar una base compilable con control real app–ESP32 y broker protegido, usando el stack aprobado sin servicios adicionales.
- Resolver interfaces comunes antes de desarrollar Escudo y Cebo en cambios independientes.
- Mantener errores, capacidades y pruebas de base observables, sin confundir conectividad preparada con módulos implementados.

**Non-Goals:**

- Implementar descubrimiento/captura/detección, honeypot o sus notificaciones externas.
- Certificar coexistencia de captura/BLE, rendimiento, desgaste o autonomía.
- Incorporar OTA, infraestructura cloud, base histórica central o frameworks adicionales de monorepo.

El registro compartido de eventos de sistema sí entra: corresponde al incremento 1.5 del roadmap y permite verificar reconexión sin fabricar incidentes de seguridad.

## Decisions

### D-01. Organización y dependencias

**Acuerdos aplicados:** separar firmware, broker, app y simulador; PlatformIO + ESP-IDF + FreeRTOS; stack React/Capacitor Android.

**Detalle propuesto:** npm workspaces solo para `app` y `contracts`; PlatformIO y Docker permanecen independientes. `simulator/` contiene inicialmente instrucciones y un límite de integración, sin desarrollar un simulador completo. No añadir Nx, Turborepo, backend ni orquestador distinto de Compose.

```text
firmware/
  platformio.ini, CMakeLists.txt, sdkconfig.defaults, partitions.csv
  src/                     app_main y composicion
  components/              supervisor, radio, BLE, MQTT, contratos,
                           registro y perifericos
  test/                    comprobaciones focalizadas
broker/
  compose.yaml, config/    mosquitto.conf y ACL sin secretos
  scripts/                 preparacion local de certificados/usuarios
  .secrets/, data/         privados y estado, ignorados
contracts/
  schemas/v1/, fixtures/   JSON Schema, ejemplos y tipos compartidos
app/
  src/                     UI, store y adaptadores
  android/                 proyecto nativo versionado
simulator/                 documentacion de integracion futura
docs/                      arranque y comprobacion funcional
```

Una raíz npm con scripts de desarrollo, build, contratos y Android es suficiente. Versionar lockfile npm y dependencias ESP-IDF; no añadir un lockfile alternativo para el mismo workspace.

**Versiones:** base firmware propuesta `platformio/espressif32@6.13.0` con su ESP-IDF 5.5.3, sin sustituir paquetes por ramas develop. Fijar también versión de PlatformIO Core usada. El [release oficial](https://github.com/platformio/platform-espressif32/releases/tag/v6.13.0) declara esa combinación. Un ajuste de parche compatible ante un bloqueo real debe quedar registrado, sin migrar a Arduino.

Para app: React 19, Vite 7, TypeScript 5, Tailwind 4 con su plugin Vite, Zustand 5, mqtt.js 5, Capacitor 8 y plugin BLE 8 de la comunidad. Node 24 LTS para el workspace. Resolver parches compatibles una vez, guardar dependencias exactas/lockfile y documentar Node/npm/JDK/SDK Android realmente usados. AJV 8 se limita a validar esquemas compartidos; no se introduce una plataforma de generación de código.

Alternativa descartada: instalar siempre “latest” sin fijar versiones o construir primero un simulador integral.

### D-02. Placa y periféricos configurables

La definición genérica `esp32-s3-devkitc-1` de PlatformIO corresponde a N8 sin PSRAM; no basta por sí sola para N16R8. Aplicar overrides de flash 16 MB y configurar PSRAM octal 8 MB mediante los parámetros de placa y `sdkconfig.defaults`. Verificar rápidamente en arranque los tamaños reales; no continuar silenciosamente con configuración distinta. [Referencia PlatformIO](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html)

Propuesta inicial de particiones, sin OTA: NVS general para identidad/bonding, PHY, una aplicación factory y NVS separado `event_log` de 256 KiB; tamaños/offsets válidos dentro de 16 MB y configuración fuente versionada. No usar toda la PSRAM como si fuera RAM interna disponible para radio/TLS.

**Supuestos de cableado propuestos, no confirmación física:** SSD1306 I2C 128×32, dirección 0x3C, SDA GPIO8, SCL GPIO9; botón externo GPIO4 a GND con pull-up interno. Son valores iniciales configurables por Kconfig, revisados contra el pinout aportado antes de conectar. No usar GPIO35–37 ni ocupar USB/UART por defecto. No confundir el botón externo con BOOT.

Usar driver SSD1306 de `esp_lcd` y controlador I2C de ESP-IDF; dibujar texto mínimo con fuente pequeña, sin LVGL. La [referencia de Espressif](https://components.espressif.com/components/espressif/ssd1306/versions/1.0.5~1) remite al driver SSD1306 de ESP-IDF. OLED ausente: diagnóstico y funcionamiento de control degradado, pero no pairing nuevo que necesite mostrar el código.

Pulsación corta al liberar, con debounce inicial de 50 ms: solicitar alternancia de perfiles Escudo/Cebo; desde reposo seleccionar Escudo. Pulsación de 3 s: abrir ventana de emparejamiento de 120 s, sin disparar la acción corta. Ambos patrones pasan por el supervisor. No borrar registros/bonds por una pulsación no documentada.

### D-03. Supervisor y perfiles de recursos

**Acuerdo:** un único supervisor modifica el estado mediante cola FreeRTOS acotada. Callbacks de radio/transportes y GPIO solo depositan solicitudes; no activan recursos por separado.

Estado mínimo: `idle`, `transitioning`, `ready`, `error`; perfil `idle|escudo|cebo`. `sanitizador` permanece reservado y devuelve `not_implemented`. `ready` significa perfil de recursos preparado, no detección/honeypot disponibles. Publicar capacidades separadas: control BLE/MQTT, registro y periféricos; captura, detectores, HTTP/Telnet y notificaciones externas en falso.

Interfaces locales pequeñas de módulo con inicio/parada y capacidades permitirán conectar futuras implementaciones. Este cambio implementa perfiles reales de recursos, no módulos ficticios con eventos de amenaza fabricados.

- Escudo: BLE de control, Wi-Fi sin asociación ni escaneo activo y MQTT detenido. La inicialización del driver para uso pasivo futuro no inicia captura ahora.
- Cebo: STA autorizada y cliente MQTT; BLE protegido permanece disponible para configuración/reconciliación local. Sin HTTP/Telnet.
- Reposo/error: sin recursos de red del modo, manteniendo controles locales seguros y BLE según política de autorización.

Transición: validar → aceptar orden → detener perfil anterior → comprobar liberación → aplicar perfil nuevo → incrementar revisión → emitir resultado/estado. La conexión STA/MQTT es asíncrona: perfil Cebo aplicado puede tener conectividad desconectada, expresada aparte. Si falla liberación, no iniciar el siguiente perfil; dejar error y permitir recuperación a reposo.

La revisión cambia al aplicar configuración/perfil, no por cada actualización de RSSI o conectividad. Cola inicial 8 solicitudes; caché de 32 resultados de comandos; exceder capacidad devuelve `busy`, sin crecimiento ilimitado. Límites iniciales ajustables ante fallos funcionales, no resultados de benchmark.

### D-04. Contratos JSON v1 y control de órdenes

**Detalle propuesto para R-05:** JSON UTF-8 de máximo 2048 bytes por mensaje lógico, JSON Schema 2020-12, propiedades/tipos y límites explícitos. App valida con AJV; firmware con cJSON y comprobaciones equivalentes sobre los campos utilizados. Fixtures compartidos válidos e inválidos evitan generar dos protocolos distintos sin montar un compilador de esquemas C++.

Sobre común: `version:1`, `type`, `device_id`, `boot_id`, `message_id`, `seq`, `uptime_ms`, `timestamp` (UTC o null), `time_quality` (`unknown|app_sync|network_sync`) y `payload`. Identidad de nodo estable en NVS; `boot_id` aleatorio en cada arranque. Los eventos añaden cursor persistente `journal_seq`; no usar el reinicio del contador de uptime como identidad.

Tipos: `state`, `metrics`, `event`, `command`, `ack`. Estado incluye perfil, fase, revisión, transporte, periféricos y capacidades reales. Eventos iniciales solo `system.booted`, `system.profile_changed` y fallos no sensibles. Métricas, si se publican, son diagnóstico real, nunca actividad de detección inventada.

Comandos de base: `get_state`, `set_mode`, `configure_connectivity`, `set_time` y `read_events`. Captura/honeypot/Sanitizador devuelven `not_implemented`. Configuración privada solo por BLE autorizado; no publicar sus valores por MQTT. `set_time` fija referencia UTC sin conectar Wi-Fi en Escudo.

Cada comando contiene `command_id`, `target_boot_id`, `expected_revision`, `expires_at_uptime_ms`, operación y argumentos. La app consulta primero metadatos protegidos por BLE o estado MQTT y calcula vencimiento máximo de 30 s sobre el uptime recibido; ninguna orden requiere reloj UTC confiable. El nodo comprueba autorización, nodo/arranque, tamaño/versión y vigencia; consulta después la caché de duplicados y, si es una orden nueva, valida revisión antes de ejecutarla. Las consultas no mutan esa revisión. Un rechazo autorizado incluye el sobre y revisión vigentes para permitir reconciliación, nunca argumentos privados.

`ack` correlacionado distingue `accepted`, `applied` y `rejected`, e incluye revisión y error acotado. Códigos iniciales: `invalid_message`, `unsupported_version`, `unauthorized`, `wrong_boot`, `expired`, `revision_conflict`, `busy`, `id_conflict`, `not_implemented`, `resource_error`. Un duplicado idéntico vigente devuelve resultado conocido sin repetir efecto; mismo ID con contenido distinto se rechaza. Tras reboot se rechaza el boot anterior. No conservar contraseñas o argumentos privados completos en caché: solo digest y resultado sanitizado.

Si la transición corta MQTT/BLE, la confirmación final puede recuperarse mediante estado/revisión y `last_command_id` por el canal autorizado disponible. La UI mantiene estado pendiente hasta reconciliar, sin inventar éxito.

### D-05. BLE autenticado sin criptografía propia

**Elección propuesta para concretar R-07:** NimBLE de ESP-IDF como periférico y plugin `@capacitor-community/bluetooth-le` como central Android. Usar Bluetooth LE Secure Connections con MITM, passkey aleatorio mostrado en OLED y bonding; rechazar Just Works/legacy o enlaces que no alcancen el nivel de protección requerido. Nuevos bonds solo dentro de ventana física; hasta 4 bonds iniciales. No combinar en esta base otro protocolo de aprovisionamiento con criptografía de aplicación propia.

La [configuración NimBLE oficial](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/kconfig-reference.html) contempla seguridad autenticada; el [plugin BLE](https://github.com/capacitor-community/bluetooth-le) expone bonding Android. Implementar y comprobar el nivel negociado, no asumir que llamar a `createBond` demuestra MITM.

GATT propuesto: servicio UUID `a90d0001-6d64-4f43-9e6d-9db51f1f0000`; características con prefijos `a90d0002` (metadatos de arranque/revisión/uptime, read, máximo 256 bytes), `a90d0003` (entrada, write con respuesta), `a90d0004` (salida, indicate), conservando el resto del UUID. El estado completo se solicita con `get_state` y se entrega por salida fragmentada, no como atributo de 2048 bytes. Lecturas, escrituras y suscripciones de datos protegidas; publicidad solo identidad de servicio, sin secretos.

Mensajes completos de 2048 bytes máximo. Cada fragmento: cabecera binaria de 8 bytes little-endian (`transfer_id:uint32`, `offset:uint16`, `total:uint16`) y datos hasta MTU−3−8 bytes. Una transferencia por dirección y conexión, offsets contiguos, timeout inicial 10 s; rechazar exceso/desorden y limpiar al desconectar. No convertir fragmentos en comandos antes del ensamblado. Este framing soporta MTU por defecto sin depender de negociar uno grande.

No depender de éxito Web Bluetooth para la comprobación Android. Ante permisos denegados o Bluetooth apagado, mostrar error accionable sin modo mock.

### D-06. Wi-Fi, MQTT y broker local seguro

Usar `esp_netif`, eventos Wi-Fi y `esp-mqtt` de ESP-IDF. Configuración de red propia/broker por BLE protegido; mantener contraseñas Wi-Fi/MQTT solo en RAM en esta base, usando almacenamiento Wi-Fi RAM. Tras reinicio se reprovisionan; persistir secretos cifrados sería otra decisión, no requisito para este cambio. Persisten identidad, bonds y eventos sanitizados, no las contraseñas de laboratorio.

MQTT 5, client ID por dispositivo; Clean Start y Session Expiry 0 para no encolar comandos antiguos. TLS con CA y hostname verificados; nunca `rejectUnauthorized:false`, `insecure` ni salto de fecha/hostname. La app puede enviar referencia temporal vía BLE antes de STA/TLS para el entorno local. El reloj sin referencia impide conectar TLS de forma insegura y deja visible cómo sincronizarlo. Reintentos asíncronos 1, 2, 4... hasta 30 s, reiniciados al recuperar.

Broker: imagen oficial `eclipse-mosquitto`, una versión 2.x exacta y digest registrados al implementar; Compose sin proxy adicional. Listeners propuestos MQTT/TLS 8883 y WSS 9001, mínimo TLS 1.2 y `allow_anonymous false`. Contraseñas con `mosquitto_passwd`, ACL explícitas, sin cuentas admin en la app.

| Identidad de laboratorio | Lectura permitida | Escritura permitida |
| --- | --- | --- |
| Nodo `node-<id>` | Solo sus commands | Solo sus state, availability, metrics, events y acks |
| Operador `operator-<id>` | Salidas del nodo autorizado | Solo commands del nodo autorizado |
| Observador `viewer-<id>` | Salidas del nodo autorizado | Ninguna |

No basar autorización solo en un client ID elegido por el cliente. Las ACL enumeran identidad y nodo y niegan por defecto otros topics; replicar la política en ambos listeners.

Topics `sentinel/v1/nodes/<id>/`: `state` y `availability` QoS1 retenidos; `metrics` QoS0 no retenido; `events`, `commands`, `acks` QoS1 no retenidos. LWT offline y publicación online al conectar; refrescar estado con uptime cada 5 s mientras haya MQTT. La app marca estado antiguo si no llega actualización en 15 s; esos límites son configurables. Un estado retenido por sí solo no prueba presencia. Rechazar comandos marcados retenidos en firmware además de no publicarlos así en app.

Configuración, CA pública y ACL revisables; claves privadas, passwords y certificados locales generados bajo directorios ignorados. Preparación local sin contraseñas en argumentos/logs, certificados con SAN del hostname/IP realmente usado y permisos legibles por el contenedor sin hacerlos públicos.

Publicar puertos de Compose en loopback por defecto; para ESP32/teléfono configurar explícitamente IP de LAN autorizada y firewall. No abrir puertos del router a Internet. Datos del broker en volumen persistente; documentación no los presenta como archivo histórico.

La confianza TLS debe resolverse realmente: CA pública local accesible al firmware y app. En Android usar configuración de red debug/CA para el entorno o CA confiable instalada, sin desactivar verificación y sin permitir globalmente tráfico claro; en navegador confiar en la CA local. Parámetros y rutas de CA no son secretos; las claves privadas sí.

### D-07. Registro local sencillo y recuperación

Usar NVS de ESP-IDF en partición `event_log`, sin sistema de archivos, SD o base de datos adicional. Ring inicial de 64 registros, máximo 2048 bytes cada uno; claves de slot y cursor persistente creciente en el propio registro. Recuperar slots válidos al arrancar y reconstruir el mayor cursor, tolerando un índice auxiliar desactualizado. Confirmar persistencia solo al completar escritura/commit; no borrar automáticamente NVS ante errores.

Conservar registros hasta sobrescritura, aunque el broker haya confirmado entrega. Reintentar pendientes por MQTT con el mismo ID; un reboot puede reenviar el ring y la app deduplica. En BLE recuperar mediante `read_events(after_cursor,limit)`, límite inicial 8: entregar eventos individuales y un resultado con siguiente cursor, rango retenido, `more` y `gap`. La paginación nunca combina ocho eventos de tamaño máximo en un único mensaje de 2048 bytes.

La app conserva cursor e historial acotado en IndexedDB: últimos 256 eventos de sistema por nodo para esta base. El ring del dispositivo es la fuente para replay mientras los registros existan; el ack MQTT no demuestra entrega a la app. Repetir una consulta con nuevo command ID si se interrumpe la página; no borrar eventos por recepción de un acuse. No prometer garantías frente a todo corte eléctrico o retención ilimitada.

### D-08. App de control y diagnóstico

Separar adaptadores `BleTransport` y `MqttTransport`, codec/validador de contratos, store Zustand y UI. Credenciales de operador solo en memoria por sesión; endpoints/IDs no sensibles pueden recordarse. No usar variables `VITE_*`, bundle o IndexedDB para incrustar contraseñas.

Pantallas mínimas: conexión/configuración de laboratorio, estado del nodo/perfil, acciones de base, lista de eventos de sistema y errores. Reconciliar por estado/revisión cuando cambia el canal. Reservar puntos de extensión para catálogo, detectores, métricas Cebo y notificaciones, mostrados como no implementados y sin datos ficticios.

La base no requiere integración push ni servicios de notificaciones ahora: esos canales siguen obligatorios dentro del MVP completo y se implementan con Cebo en su cambio. La app Android real con BLE/MQTT sí se compila e instala aquí.

Alternativa descartada: una UI con mocks como evidencia de integración o añadir servidor de autenticación/histórico antes de necesitarlo.

## Risks / Trade-offs

- [Placa o cableado difieren del supuesto] → configuración por Kconfig y comprobación rápida; cambiar pines/dirección declarados sin rediseñar la arquitectura.
- [NimBLE o pairing Android no alcanzan MITM] → comprobar nivel real y corregir integración; no aceptar silenciosamente enlace sin autenticación.
- [BLE fragmentado o conexión cortada] → buffers limitados, timeout, consulta de estado/replay y mismas identidades.
- [Cambio de transporte pierde resultado final] → revisión, command ID y reconciliación; no reutilizar comandos vencidos.
- [CA local/fecha impiden TLS] → guiar instalación de CA/SAN y referencia temporal; no desactivar verificaciones.
- [NVS falla o ring se llena] → error visible, política circular y discontinuidad; sin borrado masivo ni infraestructura de desgaste.
- [Configuración privada solo RAM] → reprovisionar tras reboot; es una limitación explícita de base, no pérdida del registro persistente.
- [No hay ESP32/OLED/teléfono conectados al implementar] → completar trabajo independiente y compilaciones; dejar comprobación real pendiente, nunca marcarla hecha con mocks.
- [Contenido previo no versionado] → no incluirlo automáticamente en commits ni modificarlo fuera de alcance; trabajar en `feat/sentinel-core-foundation` sin commits/push/merges automáticos.

## Migration Plan

No hay implementación previa ni datos que migrar. Implementar por grupos pequeños: estructura/contratos, runtime/periféricos, broker, radio/transportes, registro, app y recorrido real.

Versionar únicamente fuente, configuración y lockfiles; generar material privado localmente. Para rollback de desarrollo detener Compose y volver al firmware/configuración previamente conocidos mediante acciones explícitas del equipo, preservando datos y secretos locales. No usar operaciones destructivas Git ni borrar volúmenes/NVS como parte automática de la recuperación.

Al cerrar, registrar comandos y resultados de compilación y comprobación breve. Las tareas de hardware solo se completan con evidencia real. Los siguientes cambios incorporan Escudo y Cebo completos sobre estas interfaces; no se declara terminado el MVP R-11.

## Open Questions

Solo quedan datos locales que no cambian las specs: puerto USB de la unidad, dirección I2C/cableado final frente a los valores propuestos, SSID propio y hostname/IP/SAN del broker de laboratorio. Resolver al configurar el entorno, sin investigación previa extensa ni introducir valores privados en la propuesta.
