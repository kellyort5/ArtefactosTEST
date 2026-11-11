# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [roadmap](../../../docs/roadmap.md). Los encargos futuros del equipo se publican por separado; no son una dependencia de lectura de esta entrega de contratos.

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: base existente. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Contratos compilados y ejemplos de todas las operaciones/familias aceptados o rechazados según sus límites; revisión explícita antes de consumidores.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-03, R-04, R-06, R-07, R-08, R-10 y R-11 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.


### Propuesta a revisar: versión funcional v2

La base v1 tiene esquemas cerrados, capacidades futuras fijas en `false` y estados sin perfil Sanitizador; sus órdenes reservadas no implementan esos servicios. Añadir campos sin versión rompería sus validadores. Se propone v2 junto a los esquemas v1 intactos. La alternativa de ampliar v1 silenciosamente se descarta por esa incompatibilidad observable. La app nueva interpreta ambas versiones; el firmware funcional anuncia v2. Esto es diseño propuesto, no una decisión previamente aprobada.

### Sobre, transporte y compatibilidad

- Conservar `version,type,device_id,boot_id,message_id,seq,uptime_ms,timestamp,time_quality,payload`; v2 fija `version:2`. Identificadores ASCII de 1–64 caracteres, enteros seguros JS y reglas UTC/calidad de v1. Mensajes completos de hasta 2048 bytes UTF-8, rechazados antes de procesar si exceden ese límite; la fragmentación GATT existente no cambia.
- `command` conserva `command_id,target_boot_id,expected_revision,expires_at_uptime_ms,operation,args`. Mutaciones requieren revisión actual; reintentos idénticos conservan resultado, un ID reutilizado con contenido diferente se rechaza. Mantener la política de frescura y rechazo de comandos retained de la base.
- `ack` conserva correlación, revisión, error y estados de v1; extender `result`, ya existente, con una unión cerrada de resultados de la tabla inferior. `accepted` conserva `result:null`. Para trabajos asíncronos, `job_id` es el `command_id` original y `applied` confirma el inicio/encolado con `{kind:"job",job_id}`, no la finalización; el resultado llega en evento y se consulta por ese ID. `applied` de una orden síncrona confirma su efecto descrito, no una protección inexistente. Errores de base más `invalid_state,invalid_args,timeout,upstream_unavailable,subnet_conflict,capacity_exceeded`.
- BLE Metadata conserva la forma v1 con versión 2 y límite 256 bytes; no cambiar UUID ni encuadre. Leer Metadata y estado vivo antes de mutar. App v2 + base v1 conserva diagnóstico/control v1 y oculta funciones ausentes. App v1 + firmware v2 informa incompatibilidad, sin ejecutar órdenes v1.
- Topics: `sentinel/v2/nodes/<device_id>/{state,availability,metrics,events,commands,acks}` con QoS/retain/ACL equivalentes a v1. Estado retained es una pista: la app comprueba boot/revisión y presencia actual antes de mutar. Eventos v1 persistidos se reenvían intactos por `v1/.../events`; la app nueva escucha eventos v1 y v2. En BLE pueden recibirse eventos históricos de ambas versiones. No migrar sus identificadores ni sus cursores.
- Todos los objetos son cerrados; cada operación tiene una rama de esquema con argumentos exactos. Catálogos, reglas y clientes usan páginas solicitadas de 1–4 elementos, reducidas si el sobre supera 2048; si un elemento solo excede el límite se rechaza sin truncarlo silenciosamente. `read_events` mantiene hasta 8 eventos separados por página y las señales `gap/more/next` de la base.

### Tipos de estado, eventos y métricas

`state.payload` conserva fase, revisión, último comando, transportes, periféricos, capacidades y error de v1; `profile` añade `sanitizador`. Añadir `module` como unión etiquetada `kind:idle|escudo|sanitizador|cebo`, con resumen acotado: objetivo/captura/cobertura; auditoría/pasarela/DNS/DoH; servicios/conexiones, respectivamente. Capacidades solo verdaderas para implementaciones presentes; estado de servicio distingue disponible de ejecutándose. El ESP32 no afirma que un proveedor externo entregó una notificación: ese estado se consulta al componente de alertas.

Los eventos conservan el sobre de identidad/tiempo y los campos v1 `journal_seq,name,profile,code`. Ampliar las familias de `name` y los códigos correspondientes, y añadir `severity:info|warning|critical` y `details` como unión cerrada; el nombre de familia selecciona exactamente sus campos. La base no tiene severidad ni detalles funcionales. Para Cebo, sospecha informativa corresponde a info, media a warning y repetición alta a critical; siguen siendo indicios, no atribuciones. No admitir un objeto libre con cuerpos o credenciales.

**Aprobación explícita del usuario el 2026-11-11:** añadir `origin:"real"|"simulated"` obligatorio al payload de eventos v2, conforme al requisito de procedencia de arquitectura. Se conserva en replay, sin modificar identidad/cursor. V1 permanece intacto; ausencia de procedencia histórica no se interpreta como observación real. El receptor valida además la identidad y autorización del productor; el campo no sustituye autenticación.

| Familia / resultado | Campos funcionales permitidos |
| --- | --- |
| `escudo.networks` | `scan_id`, lista con `ssid` (0–32 bytes, representación segura si no UTF-8), `bssid`, `channel`, `rssi`, `auth`, `cipher`, `pmf` (`unknown` cuando no se observe), `seen_uptime_ms`; paginación `next,more` |
| `escudo.target` | SSID/BSSID/canal seleccionados y `capture_status`; no clave Wi-Fi |
| `escudo.detection` | detector `deauth_burst|suspicious_disconnect|evil_twin|karma_indication`, SSID/BSSID y BSSID relacionado opcional, canal/RSSI/seguridad observada, cantidad, ventana, nivel de indicio y códigos de evidencia; nunca atribución de atacante |
| `escudo.coverage` | canal, motivo de pausa, inicio/duración y contadores descartados |
| `sanitizador.audit` | `job_id`, DNS/HTTPS/portal con estado `ok|warning|failed|inconclusive`, códigos de evidencia, tiempo de respuesta, conectividad; no cabeceras, cuerpos ni credenciales |
| `sanitizador.clients` | MAC/IP local, RSSI o null con motivo, bytes IPv4 de subida/bajada y tiempo observado; paginación |
| `sanitizador.rules` | `rule_id,domain,action,include_subdomains`, con paginación; hasta 32 reglas |
| `sanitizador.dns` | tipo de resolución/bloqueo/fallo, dominio normalizado acotado, cliente local, caché/DoH, latencia y motivo; sin URL privada completa |
| `cebo.interaction` | IP origen, puerto origen y destino, `protocol:http|telnet`, interacción `connect|access|auth_attempt|input|disconnect`, ruta canónica permitida o null, contador/ventana, sospecha y códigos; sin usuarios, contraseñas, tokens, cookies ni cuerpos |
| `system.status` | código cerrado de error/recuperación y recurso afectado; sin texto arbitrario de excepciones |

`metrics.payload` discrimina `scope:system|escudo|sanitizador|cebo`, intervalo y valores cerrados. Sistema: cola/journal/pérdidas. Escudo: Beacons/Deauth/Disassoc, alertas por detector, canal/tiempo de cobertura. Sanitizador: clientes, bytes IPv4 de subida/bajada, consultas/bloqueos/caché/fallos/latencia. Cebo: conexiones/accesos/auth por protocolo, frecuencia y alertas. Contadores no negativos, RSSI en dBm, duración en ms y volumen en bytes; nunca llamar throughput RF a un contador IP.

### Catálogo de órdenes v2

`{}` significa sin argumentos; `page` es `{after:string|null,limit:1..4}` con cursor opaco de la instantánea. Cambiar el catálogo invalida su cursor y devuelve error visible para volver a comenzar. Mutaciones se rechazan si el modo requerido no está activo. Configuraciones con secretos se aceptan únicamente por BLE autenticado/cifrado, se mantienen en RAM y no se reflejan en ack/estado/logs.

| Operación | Argumentos | Resultado / condición |
| --- | --- | --- |
| `get_state,set_mode,configure_connectivity,set_time,read_events` | Semántica y campos de v1; `set_mode.mode` incluye Sanitizador | Estado/ack/página de eventos; configuración de red solo BLE |
| `escudo.scan` | `{}` | `job_id`; termina con catálogo y estado, escaneo pasivo |
| `escudo.networks` | `page` | Página `escudo.networks` |
| `escudo.select_target` | `{ssid,bssid,channel}`; BSSID observado y canal válido del país | Objetivo exacto; puede cambiar durante monitoreo con pausa visible |
| `escudo.capture_start`, `escudo.capture_stop` | `{}` | Estado aplicado, requiere objetivo para iniciar |
| `sanitizador.configure_upstream` | `{ssid,wifi_password}` con límites v1 | Solo BLE; conecta STA para auditar, sin activar AP automáticamente |
| `sanitizador.configure_ap` | `{ssid,wifi_password,ipv4,netmask}`; clave 8–63 | Solo BLE; valida subred y se aplica antes de iniciar pasarela |
| `sanitizador.configure_doh` | `{endpoint,ca_certificate}` HTTPS sin usuario/token; CA válida dentro del sobre | Solo BLE; URL hasta 256, CA hasta 1536, tamaño total comprobado |
| `sanitizador.audit` | `{dns_name,https_url,portal_url}` destinos autorizados; URLs ≤256 | `job_id`; ejecución acotada, sin enviar credenciales |
| `sanitizador.audit_result` | `{job_id}` | Resultado de última auditoría o no disponible |
| `sanitizador.gateway_start`, `sanitizador.gateway_stop` | `{}` | Estado de AP/DHCP/NAT/DNS/DoH; no anunciar protegido si alguno falla |
| `sanitizador.clients` | `page` | Página de clientes |
| `sanitizador.rules.put` | `{rule_id,domain,action:allow|block,include_subdomains}` | Regla aplicada; dominio ≤253, ID ≤32; límite 32 |
| `sanitizador.rules.remove` | `{rule_id}` | Eliminación idempotente |
| `sanitizador.rules.list` | `page` | Página de reglas |
| `cebo.start`, `cebo.stop` | `{}` | Inicio/parada de HTTP y Telnet dentro de Cebo con LAN lista |
| `cebo.configure` | `{http_enabled:true,telnet_enabled:true,max_connections:1..6,idle_timeout_ms:1000..60000}` | No permite desactivar permanentemente parte del alcance; aplicar detenido |
| `cebo.stats` | `{}` | Resumen de métricas Cebo |

Escudo y Sanitizador se controlan por BLE. Cebo admite BLE y MQTT según disponibilidad, con el mismo arbitraje. No introducir MQTT en Escudo ni exigir MQTT para configurar Sanitizador. El historial común usa `read_events`; la selección de objetivos no contiene claves ni implica asociación.

### Contrato independiente para notificaciones, propuesto para revisión

API HTTPS local del componente de notificaciones: `POST /v1/registrations` con `{device_id,installation_id,platform:"android",token}`; `DELETE /v1/registrations/<installation_id>`; `GET /v1/status?device_id=...`. Bearer propio por nodo en configuración local, siempre validado contra el nodo permitido; registro de otra identidad rechazado. Cuerpo ≤4096 y token ≤2048; registros acotados a 32, reemplazo por instalación, expiración 30 días renovable, baja y borrado explícitos. Solo el token de entrega y metadatos mínimos se guardan en el directorio privado ignorado del componente; nunca en Git, respuesta o log. Respuestas de registro incluyen solo IDs/estado y ningún secreto. API restringida a LAN, HTTPS con CA confiada y CORS de orígenes explícitos; no publica HTTP/Telnet del Cebo.

Payload push: notificación visible genérica y `data` de strings `{device_id,event_id,severity,route:"cebo"}`. `event_id` es exactamente el `message_id` del evento original, sin generar otro ID; dedup usa esa identidad junto al nodo. Al tocarla se consulta historial autorizado cuando el nodo/canal esté disponible, con aviso si el evento ya salió de retención. `GET /v1/status` devuelve canales configurados, última entrega/error por canal y conteos de pendientes/suprimidos, sin tokens ni destinos privados. No presentar aceptación por el proveedor como lectura del usuario.

Alternativa evaluada: MQTT Android permanente en segundo plano. Se descarta como mecanismo único de push con la app cerrada. FCM y el plugin oficial Capacitor se proponen en los cambios de push, pendientes de revisión, reutilizando Android existente.


### Interfaces y propiedad

Contrato único: este diseño y el [protocolo funcional implementado](../../../contracts/module-protocol.md). La API de hooks y recursos del firmware se implementará en el cambio independiente sentinel-firmware-module-api, respetando estos mensajes. No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `contracts/schemas/v2/`, `contracts/src/ (codec y exportaciones)`, `contracts/fixtures/v2/`, `contracts/test/`, `contracts/README.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Cambio de versión → conservar fixtures/decodificador v1 y probar eventos históricos sin reescribirlos.
- Mensajes voluminosos → límites de 2048 bytes y paginación verificados con ejemplos de frontera.
- Detalles de protocolo nuevos → revisión del usuario y un consumidor antes de implementar los adaptadores.

## Migration Plan

Trabajar en rama `feat/sentinel-module-contracts`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
