# Protocolo funcional Sentinel v2

Cambio archivado: [sentinel-module-contracts](../openspec/changes/archive/2026-11-11-sentinel-module-contracts/proposal.md), con [diseño](../openspec/changes/archive/2026-11-11-sentinel-module-contracts/design.md) y [cierre](../openspec/changes/archive/2026-11-11-sentinel-module-contracts/closure.md). La [spec principal](../openspec/specs/module-protocol/spec.md) conserva los seis requisitos aprobados. Los nombres exactos y tipos se publican en [v2-types.ts](src/v2-types.ts) y los objetos cerrados en [schemas/v2](schemas/v2/). El contrato HTTP de notificaciones tiene [documentación propia](notifications.md).

## Compatibilidad y negociación

| Participantes | Comportamiento |
| --- | --- |
| App de base y firmware v1 | Continúan usando las funciones y esquemas v1 sin cambios. |
| Consumidor nuevo y base v1 | Leer Metadata o estado reciente, usar las operaciones v1 y mostrar únicamente sus capacidades. |
| Consumidor nuevo y firmware funcional v2 | Usar Metadata v2, comandos v2 y resultados tipados de sus capacidades implementadas. |
| Cliente v1 y firmware v2 | Informar versión incompatible; no convertir silenciosamente una orden v1 en v2. |
| Historial con eventos v1 y v2 | Decodificar con `decodeAnyMessage`, mantener el sobre original y deduplicar por `(device_id,message_id)`. |

`decodeMessage`, `encodeMessage`, `decodeMetadata`, `makeCommand` y los tipos `Message`, `Metadata` y `CommandRequest` siguen siendo **v1**. La app de base no recibe un tipo de retorno ampliado que invalide sus comprobaciones existentes.

Las entradas nuevas son `decodeModuleMessage`, `encodeModuleMessage`, `decodeModuleMetadata`, `makeModuleCommand` y los tipos `V2Message`, `V2Metadata`, `V2CommandRequest`, `V2CommandMessage`. `decodeAnyMessage`, `encodeAnyMessage` y `decodeAnyMetadata` aceptan ambas versiones explícitamente. Sus uniones se discriminan mediante `version` antes de consumir `payload`.

```ts
import { decodeAnyMetadata, makeCommand, makeModuleCommand } from '@sentinel/contracts';

const metadata = decodeAnyMetadata(bytesFromAuthenticatedMetadata);
const command = metadata.version === 1
  ? makeCommand(metadata, { operation: 'get_state', args: {} }, commandId)
  : makeModuleCommand(metadata, { operation: 'get_state', args: {} }, commandId);
```

No enviar funciones nuevas a una base v1 ni tratar un estado MQTT retained como presencia viva. Antes de mutar deben comprobarse la sesión, el nodo, el arranque, la revisión y la vigencia. Un evento histórico v1 no cambia la versión de control de un nodo v2.

## Sobre, límites y privacidad

El sobre conserva `version,type,device_id,boot_id,message_id,seq,uptime_ms,timestamp,time_quality,payload`. V2 usa `version:2` y los tipos `state|metrics|event|command|ack`. Identificadores ASCII `[A-Za-z0-9._:-]` de 1–64; enteros no negativos seguros para JavaScript. Tiempos UTC canónicos y calendario real; `time_quality:unknown` exige `timestamp:null`. Sin hora UTC, las órdenes usan el reloj monotónico del nodo.

Cada mensaje ESP completo admite **2048 bytes UTF-8** y Metadata **256 bytes**. La fragmentación BLE no aumenta esos límites. Los SSID admiten hasta 32 bytes y las claves Wi-Fi hasta 63; los límites se verifican además de los de caracteres de JSON Schema. Las funciones de encode también validan: no publicar un objeto solamente porque TypeScript lo acepte.

Los objetos no admiten propiedades desconocidas. Eventos, ACK, métricas y estado no transportan contraseñas, usuarios capturados, tokens, cookies, cabeceras ni cuerpos completos. Los errores del codec solo incluyen códigos sanitizados. Configuración con secretos se acepta únicamente por BLE autenticado y cifrado; el consumidor debe mantenerla transitoria, sin logs, store o caché de comandos completos.

**Definición aprobada por el usuario el 2026-11-11:** cada `event.payload` v2 incluye `origin:"real"|"simulated"`. Es obligatorio y se conserva al retransmitir y almacenar, sin alterar el ID. Los fixtures compartidos son simulados; el productor físico etiqueta observaciones reales y el simulador etiqueta las suyas. El campo es una declaración de procedencia, no una prueba criptográfica: identidad, sesión y permisos del productor deben validarse por separado. Los eventos históricos v1 conservan su formato sin este campo; si no se conoce su procedencia por contexto confiable, el consumidor la muestra como desconocida, sin inventarla ni modificar el evento.

El URI MQTT exige `mqtts`, identidad TLS válida en el consumidor y ningún usuario/contraseña/query. DoH usa un endpoint HTTPS sin credenciales, fragmento ni query de configuración; la autenticación del proveedor no se transporta mediante tokens en URL. El codec comprueba formato de URI, no cadena TLS ni confianza de certificados; estas comprobaciones pertenecen al componente real de comunicación.

## Operaciones, ejemplos y consumidores

Cada enlace contiene un **ejemplo de datos ficticios**, no evidencia de una función ejecutada. Todos los argumentos exactos se validan contra la rama de la operación; los schemas no aceptan args libres.

| Operación / ejemplo | Consumidor | Canal funcional |
| --- | --- | --- |
| [get_state](fixtures/v2/commands/get_state.valid.json) | App/núcleo; estado vigente | BLE o MQTT de Cebo |
| [set_mode](fixtures/v2/commands/set_mode.valid.json) | App/supervisor; un modo activo | BLE o MQTT de Cebo |
| [configure_connectivity](fixtures/v2/commands/configure_connectivity.valid.json) | Configuración del núcleo | Solo BLE protegido |
| [set_time](fixtures/v2/commands/set_time.valid.json) | Reloj del núcleo | Solo BLE protegido |
| [read_events](fixtures/v2/commands/read_events.valid.json) | Journal del núcleo e historial app | BLE o MQTT de Cebo |
| [escudo.scan](fixtures/v2/commands/escudo.scan.valid.json) | Descubrimiento Escudo/app | Solo BLE |
| [escudo.networks](fixtures/v2/commands/escudo.networks.valid.json) | Catálogo Escudo/app | Solo BLE |
| [escudo.select_target](fixtures/v2/commands/escudo.select_target.valid.json) | Selección Escudo/app | Solo BLE |
| [escudo.capture_start](fixtures/v2/commands/escudo.capture_start.valid.json) | Captura Escudo/app | Solo BLE |
| [escudo.capture_stop](fixtures/v2/commands/escudo.capture_stop.valid.json) | Captura Escudo/app | Solo BLE |
| [sanitizador.configure_upstream](fixtures/v2/commands/sanitizador.configure_upstream.valid.json) | STA/auditoría Sanitizador/app | Solo BLE |
| [sanitizador.configure_ap](fixtures/v2/commands/sanitizador.configure_ap.valid.json) | Pasarela Sanitizador/app | Solo BLE |
| [sanitizador.configure_doh](fixtures/v2/commands/sanitizador.configure_doh.valid.json) | DoH Sanitizador/app | Solo BLE |
| [sanitizador.audit](fixtures/v2/commands/sanitizador.audit.valid.json) | Auditoría Sanitizador/app | Solo BLE |
| [sanitizador.audit_result](fixtures/v2/commands/sanitizador.audit_result.valid.json) | Diagnóstico Sanitizador/app | Solo BLE |
| [sanitizador.gateway_start](fixtures/v2/commands/sanitizador.gateway_start.valid.json) | Pasarela Sanitizador/app | Solo BLE |
| [sanitizador.gateway_stop](fixtures/v2/commands/sanitizador.gateway_stop.valid.json) | Pasarela Sanitizador/app | Solo BLE |
| [sanitizador.clients](fixtures/v2/commands/sanitizador.clients.valid.json) | Clientes/tráfico Sanitizador/app | Solo BLE |
| [sanitizador.rules.put](fixtures/v2/commands/sanitizador.rules.put.valid.json) | Reglas DNS Sanitizador/app | Solo BLE |
| [sanitizador.rules.remove](fixtures/v2/commands/sanitizador.rules.remove.valid.json) | Reglas DNS Sanitizador/app | Solo BLE |
| [sanitizador.rules.list](fixtures/v2/commands/sanitizador.rules.list.valid.json) | Reglas DNS Sanitizador/app | Solo BLE |
| [cebo.start](fixtures/v2/commands/cebo.start.valid.json) | HTTP/Telnet Cebo/app | BLE o MQTT de Cebo |
| [cebo.stop](fixtures/v2/commands/cebo.stop.valid.json) | HTTP/Telnet Cebo/app | BLE o MQTT de Cebo |
| [cebo.configure](fixtures/v2/commands/cebo.configure.valid.json) | Límites Cebo/app | BLE o MQTT de Cebo |
| [cebo.stats](fixtures/v2/commands/cebo.stats.valid.json) | Métricas Cebo/app | BLE o MQTT de Cebo |

`set_mode` no inicia funciones no implementadas: el supervisor decide según capacidades reales. Escudo/Sanitizador requieren su modo activo; Escudo no se asocia a Wi-Fi ni usa MQTT directo. Configurar Cebo no permite eliminar HTTP o Telnet de su alcance final.

## Admisión, duplicados y confirmaciones

`makeModuleCommand` correlaciona `message_id`/`command_id`, destino/boot/revisión y vencimiento monotónico. Admite hasta 15000 ms de antigüedad local de la Metadata y construye una vigencia adicional de 30000 ms. Se rechazan desbordamientos y mensajes que excedan el límite.

Dos helpers **puros**, sin ejecutar operaciones ni mantener una caché, explicitan el orden de admisión:

1. `checkModuleCommandAdmission(command,context)` valida formato, autorización/cifrado, nodo, ausencia de retained, boot, vencimiento y canal permitido. MQTT además requiere un perfil con ese transporte disponible. Un timeout o un arranque viejo se rechazan antes de consultar resultados guardados.
2. El consumidor consulta su caché acotada por ID y huella: un duplicado idéntico devuelve el resultado previo; otro contenido con ese ID devuelve `id_conflict`. No guardar secretos del request en la caché; el núcleo real sigue siendo dueño de este arbitraje.
3. Solo si la orden es nueva, `checkNewModuleCommandContext(command,context)` comprueba además la revisión vigente, el modo requerido por la operación y el objetivo necesario para `escudo.capture_start`. Así un cambio posterior de perfil no invalida la respuesta ya guardada de una orden idéntica todavía vigente. El consumidor comprueba otras precondiciones de servicio que solo él conoce y encola/ejecuta una única vez.

El contexto contiene `channel,authenticated,authorized,encrypted,authorizedDeviceId,bootId,revision,uptimeMs,profile`, con `retained` y `targetSelected` cuando corresponda. La aplicación no puede suministrarse su propia autorización al dispositivo: esos datos los deriva el receptor de su sesión segura y permisos por operación. Autenticación y autorización son comprobaciones distintas. Los helpers validan ese contexto; no hacen pairing, TLS o ACL por sí mismos.

`accepted` confirma recepción y tiene `result:null`. Un `applied` de trabajo asíncrono confirma inicio/encolado, con `result:{kind:"job",job_id:<command_id>}`; la finalización llega en evento o mediante consulta por el mismo job. Un resultado de detector o auditoría no se fabrica al aceptar la orden. `rejected` requiere error sanitizado. El acuse MQTT no equivale a `applied` ni a entrega al usuario.

`MODULE_COMMAND_RESULTS` es la matriz ejecutable de operación → resultado `applied`. `checkModuleAckForCommand(command,ack)` valida el nodo, la correlación y el tipo de resultado, además de los datos seleccionados cuando hay objetivo, regla o auditoría. Un rechazo correlacionado puede informar un boot nuevo; un ACK de aplicación/aceptación de otro boot no completa la orden pendiente.

El helper también comprueba cantidad contra el límite solicitado, progreso de cursor de catálogo y coherencia del rango/avance del journal, conservando sus huecos declarados. No puede demostrar que los eventos separados hayan llegado: el consumidor verifica su recepción antes de avanzar el cursor durable. Un paquete malformado produce `ContractError` sanitizado; un paquete válido que no corresponde a la orden devuelve el código de rechazo.

| Operación | `result` de `applied` |
| --- | --- |
| get_state | null después de state individual, como v1; o `{kind:"state",state}` completo |
| set_mode, configure_connectivity, set_time | null; reconciliar mediante estado/revisión |
| read_events | `{kind:"events",next_cursor,retained_from,retained_to,more,gap}` |
| escudo.scan, sanitizador.audit | `{kind:"job",job_id}`; solo inicio/encolado |
| escudo.networks | `{kind:"escudo.networks",scan_id,networks,next,more}` |
| escudo.select_target | `{kind:"escudo.target",target}` con el objetivo solicitado y estado de captura |
| escudo.capture_start/stop, sanitizador.configure_upstream/ap/doh, sanitizador.gateway_start/stop, cebo.start/stop/configure | null; comprobar subestado real del módulo |
| sanitizador.clients | `{kind:"sanitizador.clients",clients,next,more}` |
| sanitizador.rules.put | `{kind:"sanitizador.rule",rule}` con la regla aplicada |
| sanitizador.rules.remove | `{kind:"sanitizador.rule_removed",rule_id}` |
| sanitizador.rules.list | `{kind:"sanitizador.rules",rules,next,more}` |
| sanitizador.audit_result | `{kind:"sanitizador.audit",audit}`; audit es el resultado de ese job o null |
| cebo.stats | `{kind:"cebo.stats",interval_ms,values}` |

La app nueva correlaciona primero por comando y luego por `result.kind`: no tratar cualquier resultado no nulo como página de journal. Los productores y consumidores usan esta matriz; no inventan un formato alternativo por pantalla.

Para un scan, `scan_id` es exactamente el `job_id`/`command_id` que lo inició. El nodo conserva una última instantánea acotada: consultar `get_state` identifica `module.scan_id`/`scan_status` y `escudo.networks` con `after:null` recupera esa instantánea aunque se haya perdido el evento de fin. Un scan nuevo invalida cursores anteriores; se devuelve `revision_conflict` y se comienza de nuevo. No mezclar páginas de distintos `scan_id`.

`sanitizador.audit_result` consulta el job explícito de la última auditoría retenida. `audit:null` no demuestra éxito: si coincide con `module.audit_job_id` y el subestado es running, sigue pendiente; si el job no está retenido/disponible, la app informa esa condición. El estado separa aceptación, ejecución y resultado; no se promete un archivo ilimitado de auditorías.

## Páginas, retención y cobertura

Catálogo de redes, clientes y reglas reciben `{after:null|string,limit:1..4}`. El cursor identifica una instantánea: si cambia, el productor informa error y el consumidor comienza otra consulta. Al construir una respuesta, el productor intenta `encodeModuleMessage`; si supera 2048 bytes, reduce el número de elementos. Si ni un elemento cabe, devuelve un error explícito sin truncar sus campos.

`read_events` conserva el mecanismo de base: hasta ocho **mensajes de evento separados**, antes del ACK de página. El cursor persistente, rango retenido y `gap` distinguen historial disponible de pérdida por sobrescritura. La app no adelanta el cursor durable solo por recibir un evento en vivo. La entrega repetida mantiene identidad y versión.

`state` distingue perfil/fase/capacidades reales de ejecución del módulo. `metrics` discrimina scope sistema/Escudo/Sanitizador/Cebo; tiempos en ms, RSSI en dBm y tráfico en bytes IPv4, sin llamarlo throughput RF. Los incidentes Escudo muestran evidencia e indicios, no atribución; SSID duplicado no es por sí mismo malicioso. Pausas de captura y huecos de historial son visibles.

## Codebook para los consumidores

Los tipos cerrados de [v2-types.ts](src/v2-types.ts) son la referencia completa. La fuente reproducible de esquemas/tipos es [generate-v2-schemas.mjs](tools/generate-v2-schemas.mjs); no editar manualmente sus salidas para crear otro dialecto. Son definiciones fuente mantenidas del contrato, no artefactos de compilación. Para regenerarlas: `node contracts/tools/generate-v2-schemas.mjs` desde la raíz y revisar el diff.

- **SSID:** string UTF-8 sin NUL y hasta32 bytes; oculto es `""`. Para bytes que no puedan representarse así usar `{encoding:"hex",value:<hex lowercase de pares, hasta64 caracteres>}`. Esta representación se conserva al seleccionar objetivo; nunca sustituir bytes no UTF-8 por caracteres de reemplazo. Las credenciales de conexión usan SSID string no vacío.
- **BSSID/MAC:** seis octetos hex lowercase separados por `:`. **IPv4:** notación decimal válida. **Dominios:** ASCII/punycode lowercase, sin punto final, hasta253 caracteres y etiquetas válidas. **Cursores de catálogo:** string opaco 1–128 de `[A-Za-z0-9._:~-]`, o null; no son cursores numéricos de journal.
- **Seguridad observada:** auth `open|wep|wpa_personal|wpa2_personal|wpa3_personal|wpa_wpa2_personal|wpa2_wpa3_personal|enterprise|unknown`; cipher `none|wep|tkip|ccmp|gcmp|mixed|unknown`; PMF `disabled|capable|required|unknown`. Unknown expresa dato no observado, sin completar características inventadas.
- **Captura:** `stopped|starting|running|paused|error`. Pausas: `none|ble_sync|channel_hop|target_change|resource_error|stopped`. Cobertura incluye canal o null, inicio monotónico, duración y descartes de tramas/eventos.
- **Servicios:** `{available:false,status:"unavailable"}` o `{available:true,status:"stopped"|"starting"|"running"|"error"}`. Available no implica running. Pasarela `running` exige upstream conectado y AP/DHCP/NAT/DNS/DoH running; una función ausente o fallida no se anuncia protegida.
- **Auditoría:** cada prueba DNS/HTTPS/portal devuelve `ok|warning|failed|inconclusive`, evidencia enum y tiempo de respuesta o null. **Clientes:** RSSI numérico o null con motivo `not_available|not_associated|driver_unsupported`; un RSSI observado lleva motivo null.
- **Cebo:** protocolos `http|telnet`, interacciones `connect|access|auth_attempt|input|disconnect`, sospecha `informational|medium|high` ligada a severidad `info|warning|critical`. Rutas permitidas: null, `/`, `/login`, `/admin`, `/favicon.ico`, `/robots.txt`, `/other`; Telnet siempre null. No conservar query/fragmento ni cuerpos.
- **Evidencia:** códigos de los enums `V2DetectionEvidence`, `V2AuditEvidence` y `V2CeboEvidence`, sin texto libre. Detectores `deauth_burst|suspicious_disconnect|evil_twin|karma_indication`; ningún código significa atacante identificado. Los ejemplos de SSID duplicado benigno son catálogo, no un veredicto.

Los contadores de métricas son acumulados por arranque mientras se ejecuta el scope correspondiente; no reiniciarlos por reconectar transportes. Free/min heap, queue/journal count, canal, clientes y conexiones activas son valores de estado; `interactions_per_minute` es la tasa de la ventana móvil de60 s. `dns_latency_ms` es la última medida o null. `interval_ms` expresa el intervalo cubierto entre informes; las gráficas calculan deltas de contadores del mismo arranque y reinician la referencia si cambia boot o se detecta reinicio del contador. Los bytes por cliente describen su asociación observada y pueden reiniciar al reasociarse; no confundirlos con los contadores globales del scope.

## MQTT y BLE

`mqttTopic(version,deviceId,kind)` construye exclusivamente `sentinel/v{1|2}/nodes/<device_id>/<kind>`, validando ID y tipo. `MQTT_POLICY` conserva state/availability QoS1 retained, metrics QoS0 sin retained y events/commands/acks QoS1 sin retained. `availability` sigue siendo texto `online|offline`, no un sexto mensaje JSON.

El firmware funcional usa topics v2; eventos v1 del journal se reenvían intactos por v1/events. Nuevos consumidores escuchan ambas familias de eventos y deduplican mediante `eventIdentity(event)`. Las ACL autorizan cada topic por nodo/rol; no se modifica el broker desde este cambio de contratos.

Se conservan UUID GATT, cabecera LE de ocho bytes, límite lógico2048, ensamblado/timeout y limpieza por sesión. `fragmentMessage` y `BleAssembler` sirven para mensajes de ambas versiones sin reinterpretarlos. Una transferencia incompleta nunca se convierte en una orden.

## Comprobación

Desde la raíz, con Node24:

```sh
npm run check:contracts
npm run build -w contracts
npm run test:app
npm run build
```

Las pruebas de contrato son fixtures y lógica ejecutable. No acreditan captura, NAT/DoH, pairing BLE, HTTP/Telnet o recepción push real; esas funciones se integran y comprueban en sus cambios respectivos.
