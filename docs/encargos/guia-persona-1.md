# Guía de Persona 1 — Jefry: coordinación, núcleo y Escudo

**Acuerdo vigente (2026-11-17):** [MQTT anónimo, entrega P1 y reparto actualizado](../mvp-policy.md). Sin TLS/cuentas/roles/ACL MQTT; BLE y HTTPS/DoH conservan sus protecciones. Evidencia histórica no sustituye las pruebas físicas pendientes.

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-sin-cuentas/verification.md).

P1 mantiene núcleo, integración Arduino/Escudo y API central: cuatro claves mínimas, retirada de cinco overrides propios antiguos, wrapper anti-autoerase durante arranque normal y tabla nueva. Probar en placa identidad/bonds/configuración/historial tras reinicio y corte sin upload; restaurar en Idle y renovar UTC. Primera carga limpia y posteriores USB no prueban conservación; nunca grabar desde setup scripts.

Tu resultado es entregar las interfaces que consumen los compañeros, el Escudo completo y la integración del núcleo. También mantienes OpenSpec para todos. Los compañeros implementan sus encargos y aportan evidencia; tú no debes absorber sus tareas para actualizar las specs.

## Estado actual y tu siguiente trabajo

La base P1 (Escudo, contratos/API, app v2 y herramientas de pruebas sin OLED) está entregada en software. El commit b63772a incluye correcciones de caché/pila BLE: conservar y ejecutar la tarea física4.3 de [pruebas sin OLED](../../openspec/changes/sentinel-p1-pruebas-sin-oled/tasks.md), con la imagen integrada actual. Sigue [la guía P1](../guia-pruebas-persona-1.md). No cerrar por mostrar el passkey de una versión intermedia.

Coordina la retirada transversal MQTT de usuarios/roles/ACL/TLS y comparte endpoint/topics, sin credenciales MQTT. Después integra las entregas de P2/P3/P4 en sentinel-integracion-tres-modos, incluyendo OLED y alertas recientes reales. P4 consume tu controller/store/journal y panel; sus pantallas finales y gráficos siguen siendo su responsabilidad. Las correcciones del núcleo/contratos siguen contigo, un autor por spec.

## 1. Tu lectura y punto de partida

Para tu primer montaje y prueba, seguir la [guía desde cero de software, hardware y specs de Persona 1](../guia-pruebas-persona-1.md), actualizada para tu app común v2: instalación, pairing, estado, Reposo ↔ Escudo, reconexión e historial. Distingue esos controles de las pantallas específicas pendientes del equipo.

La [verificación](../../openspec/changes/archive/2026-11-12-sentinel-app-protocolo-v2/verification.md) y el [README de app](../../app/README.md) registran el incremento v2 y sus pendientes físicos. La sesión actual permite recorrer T01–T15 según condiciones usando la pantalla Escudo para catálogo/selección/captura, gráficos e incidentes; los negativos de protocolo se verifican por software. Publicar o actualizar la guía no acredita esas pruebas ni cambia sus casillas OpenSpec.

Leer la [guía del equipo](guia-equipo.md), el [reparto](README.md), la [arquitectura](../architecture.md), [R-02 a R-07 y R-11, con H-01 a H-08](../technical-decisions.md) y el [roadmap](../roadmap.md).

Para los contratos, consultar [protocolo v2](../../contracts/module-protocol.md), [notificaciones](../../contracts/notifications.md), [evidencia](../../contracts/evidence-v2.md) y [spec principal](../../openspec/specs/module-protocol/spec.md). El usuario aprobó el contrato y el cierre de 2.2; el cambio está [archivado con 7/7 tareas](../../openspec/changes/archive/2026-11-11-sentinel-module-contracts/closure.md) y publicado en el PR #2. No repetir su implementación ni confundir publicación con merge.

Revisar [cierre de la base](../../openspec/changes/archive/2026-11-10-sentinel-core-foundation/closure.md) y [prueba física](../core-foundation-test-guide.md). Las pruebas físicas asumidas al archivar la base siguen pendientes de evidencia.

## 2. Tus ocho encargos en orden

Cada enlace abre una ficha con diseño, specs y tareas oficiales. La rama prevista es `feat/<nombre completo del cambio>`.

| Orden | Encargo propio | Trabajo concreto | Entrega que desbloquea |
| --- | --- | --- | --- |
| 1 — completado | [sentinel-module-contracts](sentinel-module-contracts.md) | Contrato v2 aprobado, publicado y archivado; conservar compatibilidad v1, 25 operaciones, ACK/páginas, origen, privacidad y contrato HTTP/push. Consultar su cierre y reutilizar la biblioteca. | P2/P3 conocen mensajes; P4 puede adaptar app/broker; todos comparten fixtures. |
| 2 — software entregado; física pendiente | [sentinel-firmware-module-api](sentinel-firmware-module-api.md) | Publicar `sentinel_api`, hooks/lifetime, registro opcional, dispatcher v2, recursos prestados, eventos/métricas y supervisor. Integrar flags globales de forwarding/NAPT y controles físicos según diseño revisado. | P2/P3 pueden integrar módulos sin editar el núcleo; P4 prueba comandos/capacidades reales. |
| 3 — software archivado; física pendiente | [sentinel-escudo-descubrimiento](sentinel-escudo-descubrimiento.md) | Escaneo pasivo 2.4 GHz, catálogo limitado/paginado con SSID/BSSID/canal/RSSI/seguridad, selección y cambio de objetivo por BLE. | P4 conecta catálogo y selección de red. |
| 4 — software archivado; física pendiente | [sentinel-escudo-captura](sentinel-escudo-captura.md) | Modo promiscuo, cola limitada, parser Beacon/Deauth/Disassoc y metadatos Probe, prioridad de objetivo, recorrido de canales, cobertura y start/stop. | Flujo de observaciones para todos los detectores y métricas de captura. |
| 5 — software archivado; física pendiente | [sentinel-escudo-detectores](sentinel-escudo-detectores.md) | Ráfagas Deauth, desconexión sospechosa, Evil Twin contextual e indicios Karma pasivos. Conectar los cuatro al parser, eventos y métricas v2. | P4 puede integrar todos los incidentes; P4 recibe casos positivos/negativos para laboratorio. |
| 6 — software archivado; física pendiente | [sentinel-escudo-recuperacion](sentinel-escudo-recuperacion.md) | Journal compartido, recuperación tras BLE/reinicio, cursor/gap y cobertura. Intentar captura+BLE concurrentes; si fallan, pausa controlada con cobertura visible y todas las funciones conservadas. | Cierre del Escudo y prueba conjunta de historial sin duplicados. |
| 6b — software archivado; física pendiente | [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md) | Adaptación común v1/v2, navegación, comandos, historial y limpieza de secretos; entrega de P1 antes de las pantallas de P4. | Control/estado e historial comunes; faltan pantallas específicas y comprobación física. |
| 7 | [sentinel-integracion-tres-modos](sentinel-integracion-tres-modos.md) | Registrar módulos entregados, verificar parada/exclusión, OLED/botón y resumen de la alerta más reciente, reconciliación de estado/transportes/journal y recorrido final de tres modos. | Entrega conjunta de ESP32/app/alertas con evidencia de todos. |

`sentinel-app-protocolo-v2` y, por una autorización posterior específica, `sentinel-app-escudo` pasan de P4 a P1. La pantalla Escudo está implementada por software con aceptación física pendiente; ver su [guía de uso](../modules/app-escudo.md). P4 conserva sus demás encargos. Coordinar el traspaso de controller/store/transportes antes de que P4 añada pantallas.

El encargo 7 espera las dependencias enumeradas en su ficha. No es una autorización para programar las pantallas o reparar el código Sanitizador/Cebo de otro autor.

La **tarea 1.5 del encargo 7 es tuya**: conectar las alertas del núcleo a la OLED y comprobar su resumen físicamente, sin depender de abrir la app. Cubrir ausencia de alertas, sustitución por una nueva, módulo de origen, tiempo disponible y prioridad del código de emparejamiento. Las demás diferencias respecto al PDF se documentan en la [comparación de alcance](../pdf-scope-comparison.md); no se añaden nuevas funciones por esa revisión.

## 3. Qué hacer primero

1. Conservar la referencia actual y su trabajo local; no cambiar de rama ni descartar cambios ajenos. La rama compartida de simplificación es `feat/sentinel-arduino-migration`.
2. Revisar el [estado de integración oficial](../firmware.md#preparar-arduino-ide). Legibilidad, mantenimiento host y build completo oficial Windows tienen evidencia; seguir las bibliotecas/opciones actuales y conservar las pruebas físicas pendientes. No reinstalar el SDK propio para declarar cumplido el destino nuevo.
3. Reutilizar API/contratos y los cuatro incrementos de Escudo entregados. Consultar [mapa del núcleo](../development.md#mapa-de-lectura-del-núcleo) y sus límites; no repetir su implementación para conectar módulos futuros.
4. Comunicar al equipo API, flags, dependencias definitivas y resultado del build cuando se entreguen. P2/P3 incorporan sus módulos; P4 conserva pantallas específicas, broker y laboratorio.
5. Preparar cableado y criterios T01–T15 de la guía. Ejecutar upload/BLE/radio cuando exista placa identificada y procedimiento verificado; documentar pendientes sin inventar resultados. La integración final mantiene todas sus funciones, incluidas alertas recientes en OLED.

## 4. Límites de tu implementación

Eres autor de `contracts/`, núcleo `src/sentinel/`, API `src/sentinel_api/`, sketch `SentinelNode.ino`, `SentinelConfig.h` y `src/escudo/`, con sus tests/documentación. Las rutas específicas se detallan en cada ficha.

- Un solo supervisor y gestor de radio. Los módulos solicitan recursos; no crean drivers/supervisores alternativos. Solo un modo activo por ESP32.
- El núcleo crea identidad/cursor, valida y persiste eventos. Reutilizar el journal NVS de la base; P2/P3 no implementan otros journals en paralelo.
- En callbacks, copiar datos a colas acotadas; evitar flash o trabajo bloqueante. Respetar lifetime y parada antes de conceder recursos al siguiente modo.
- Escudo nunca se asocia a la red auditada ni usa MQTT directo. BLE sigue siendo su control y salida.
- SSID duplicado no es prueba de malicia. Los cuatro detectores publican indicios, evidencia y límites; no prometen certeza o cobertura continua no demostrada.
- P1 implementa el adaptador común `sentinel-app-protocolo-v2` y la pantalla `sentinel-app-escudo`; P4 controla las demás specs de `app/`, `broker/`, simulador y lockfile raíz. P3 controla `notifications/`; P2 controla Sanitizador. Solicitar cambios a su dueño.

## 5. Cómo usar OpenSpec en tu día a día

Las 23 propuestas ya existen. No volver a proponerlas ni aplicar todas juntas. El adaptador común v2 ya está archivado con 2.1 pendiente: sus tareas/evidencia se consultan en el archivo, no mediante `openspec apply`. El ejemplo siguiente es para integración y se ejecuta cuando estén listas sus dependencias. Antes de cada cambio, revisar sus artefactos y dependencias. En una sesión con las skills del repositorio puedes pedir, por ejemplo:

```text
$openspec-apply-change sentinel-integracion-tres-modos
Lee AGENTS.md, su ficha, diseño, specs y tareas. Implementa solo este cambio,
preserva trabajo ajeno y verifica compilación y comportamiento básico.
No hagas commit, push, merge ni archivo sin autorización explícita.
```

Después, repetir con cada nombre de la tabla cuando corresponda. Esa línea es una instrucción para el asistente, no un comando de shell. Para consultar desde terminal:

```sh
openspec status --change sentinel-integracion-tres-modos --json
openspec instructions apply --change sentinel-integracion-tres-modos --json
openspec validate sentinel-integracion-tres-modos --strict --json
```

Para P2/P3/P4: pedir tarea numerada, resultado, referencia y evidencia. Leer la línea original de `tasks.md`, contrastarla con el comportamiento y marcar únicamente lo demostrado. Una aceptación de pendientes se documenta como aceptación, sin convertirla en prueba realizada. Si cambia alcance/interfaz, mantener ficha, diseño, spec y contrato coherentes antes del consumidor.

La sincronización/archivo se gestiona después de implementar y revisar el cambio por su flujo OpenSpec autorizado. No copiar deltas pendientes a specs principales como si estuvieran terminados.

## 6. Comprobaciones y coordinación

Desde raíz, con dependencias preparadas según [desarrollo](../development.md):

```sh
npm run check:contracts
npm run build -w contracts
SENTINEL_SANITIZE=1 npm run test:firmware
SENTINEL_SANITIZE=1 npm run test:escudo
node firmware/test/arduino_build_test.mjs
```

La integración oficial y su build completo ya tienen evidencia; compilar el sketch real con Arduino IDE Windows conforme a [firmware](../firmware.md). Node es mantenimiento opcional; los comandos anteriores son comprobaciones host/contratos. Ejecutar las comprobaciones que corresponden al cambio y añadir tests focalizados de exclusión, admisión/duplicados, parsers, detectores o límites. Compilar no demuestra BLE/captura/NVS físicos. Para grabar y observar la placa, seguir la [guía física](../core-foundation-test-guide.md) y acordar el turno de hardware.

| Con quién | Lo que tú entregas | Lo que necesitas recibir |
| --- | --- | --- |
| P2 | Contrato, hooks, netifs y flags de núcleo. | Registro/requisitos de Sanitizador, resultados NAT/DNS/DoH y errores reproducibles. |
| P3 | Contrato, hooks, journal y publicación central. | Registro Cebo, requisitos de recursos y evidencia de servicios/persistencia/alertas. |
| P4 | Operaciones, capacidades y observaciones Escudo reales. | UI/transportes integrados, broker LAN y matriz de evidencia; él mantiene los cambios de app. |

Tu cierre requiere Escudo completo y los tres modos integrados sin recursos residuales, OLED/botón reales, recuperación, app y canales externos comprobados. Devolver cada fallo al dueño de su encargo y conservar el alcance completo. Usa el [formato de entrega común](guia-equipo.md#6-rutina-por-tarea-y-entrega).

[Volver al reparto](README.md) · [Guía del equipo](guia-equipo.md)
