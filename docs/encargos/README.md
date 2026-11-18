# Reparto vigente de Sentinel Node para cuatro integrantes

**Referencia de trabajo: `develop`.** Este índice y las fichas reflejan el alcance aprobado al **17 de noviembre de 2026**. Cada encargo tiene un único autor, una rama y sus documentos OpenSpec enlazados. Los compañeros pueden seguir los Markdown sin instalar OpenSpec; Jefry mantiene el seguimiento oficial.

Antes de comenzar, leer [acuerdos vigentes del MVP](../mvp-policy.md), [guía común](guia-equipo.md) y la guía personal de abajo. MQTT usa TCP1883/WS9001 anónimos por IP en la LAN del equipo. No hay gestión de cuentas, roles, ACL ni certificados MQTT. Se conservan BLE protegido, permisos Android y HTTPS/DoH; las claves propias de integraciones se configuran fuera de Git.

Para preparar el equipo, seguir el [mini setup de Windows con Arduino IDE](../mini-setup.md).

## Estado real de la entrega compartida

P1 ya entregó **contratos v2, API/núcleo, los cuatro incrementos de Escudo, app común v2, diagnóstico BLE y salida serie sin OLED**, además de las correcciones de memoria/pila BLE y del broker anónimo. No rehacer esas entregas como requisito previo de P2/P3/P4. Consultar [verificación integrada vigente](../../openspec/changes/sentinel-mvp-sin-cuentas/verification.md).

| Entrega compartida adicional | Responsable | Seguimiento |
| --- | --- | --- |
| [Diagnóstico P1 sin OLED](../../openspec/changes/sentinel-p1-pruebas-sin-oled/tasks.md) | P1 | 8/9 tareas; **4.3 física pendiente** sobre la imagen final. [Guía de pruebas](../guia-pruebas-persona-1.md). |
| [MVP con MQTT anónimo](../../openspec/changes/sentinel-mvp-sin-cuentas/tasks.md) | P1 | 5/5 tareas de software; no acredita los pendientes físicos de otros cambios. |

**Pendiente de implementar:** Sanitizador, Cebo/servidor de alertas, pantallas finales/push y la integración completa de tres modos, según sus fichas. El panel diagnóstico no completa las pantallas de P4. OLED y sus alertas recientes siguen siendo obligatorias en la integración P1, tarea **1.5**; la salida serie permite adelantar pruebas, no acredita la OLED física.

La persistencia de identidad, bonds, configuración aceptada e historial confirmado durante uso/reinicio/corte se conserva; USB puede reinicializar datos sin migración. Ver [política](../persistence-policy.md). Firmware: **Arduino IDE Windows, core oficial ESP32 3.3.12, ESP32S3 Dev Module N16R8**, sketch único `firmware/SentinelNode/SentinelNode.ino`; seguir [preparación](../firmware.md). El SDK propio/PlatformIO de algunos archivos archivados es histórico.

## Guías personalizadas: empezar aquí

Leer primero la [guía común del equipo](guia-equipo.md): cómo encontrar requisitos, consumir contratos, preparar una rama y entregar evidencia sin tener que aprender OpenSpec.

Antes de repartir, consultar la [comparación del alcance con el PDF](../pdf-scope-comparison.md). Las alertas recientes en OLED corresponden a Persona 1, [integración de tres modos](sentinel-integracion-tres-modos.md), tarea **1.5**. Las demás diferencias quedan documentadas sin ampliar las tareas de los compañeros.

| Integrante | Guía y recorrido de trabajo |
| --- | --- |
| Persona 1 — Jefry | [Coordinación, contratos, API, Escudo e integración](guia-persona-1.md). Incluye cómo aplicar cada cambio con OpenSpec y hacer seguimiento de las entregas del equipo. |
| Persona 2 | [Auditor/Sanitizador completo](guia-persona-2.md): auditoría → pasarela/NAT → DNS → DoH, contratos, pruebas y coordinación con núcleo/app. |
| Persona 3 | [Cebo y servidor de alertas](guia-persona-3.md): HTTP → Telnet → eventos/persistencia → webhook → push servidor. |
| Persona 4 | [App, broker LAN y laboratorio](guia-persona-4.md): consume base común de P1 → infraestructura y pantallas → push Android y evidencia conjunta. |

Las guías explican el reparto aprobado; las fichas y sus specs conservan el detalle de tareas y aceptación. Un cambio sigue teniendo un solo autor. La existencia de una guía no autoriza Git ni convierte una propuesta en comportamiento implementado.

## Reparto confirmado y responsabilidad

**Ajuste confirmado el 2026-11-11:** se elimina Telegram del alcance de Cebo y de P3. Se mantienen alertas en la app, webhook y push móvil; el encargo se llama ahora `sentinel-alertas-webhook`. El reparto conserva los mismos responsables y cinco encargos de P3.

**Reasignación confirmada el 2026-11-12:** P1 asume exclusivamente `sentinel-app-protocolo-v2`. P4 conserva sus otros seis encargos. P4 consume la entrega común de P1 y evita editar sus archivos mientras se integra esta spec.

El usuario coordina OpenSpec y desarrolla Escudo. El Sanitizador comienza desde la primera ronda, por instrucción del 2026-11-10; los tres módulos conservan todo su alcance funcional. Solo un modo usa la radio a la vez en cada ESP32.


| Integrante        | Trabajo propio                                                                   | Encargos originales |
| ----------------- | -------------------------------------------------------------------------------- | -------------- |
| Persona 1 — Jefry | Contratos, API/supervisor, Escudo, app común v2, integración y seguimiento OpenSpec | 8              |
| Persona 2         | Auditor/Sanitizador completo                                                     | 4              |
| Persona 3         | Cebo completo y envío de webhook/push                                   | 5              |
| Persona 4         | Pantallas de tres modos, recepción push Android, broker LAN y laboratorio        | 6              |


Los números no representan esfuerzo equivalente. Persona 4 tiene la mayor variedad: debe consumir la app común de P1 y adelantar broker, reutilizar componentes de UI y coordinar evidencia. Persona 3 se encarga del servidor de alertas para repartir infraestructura. No se inventan fechas, nombres de compañeros ni disponibilidad de otras placas. Con una sola ESP32, acordar turnos de pruebas breves; el trabajo de carpetas separadas avanza en paralelo.

**Un cambio = un autor.** No hay una spec compartida entre firmware y UI, ni entre envío y recepción push. El revisor es otro integrante y no toma tareas de implementación dentro del mismo cambio. Si alguien necesita ayuda, se transfiere el encargo completo de forma explícita o se crea otro cambio acotado; no asignar simultáneamente sus casillas a dos personas.

## Contratos y decisiones antes de programar

El alcance de los tres módulos, el transporte por modo, la exclusión de modos, la API y el contrato v2 ya están aprobados. Consumir [protocolo](../../contracts/module-protocol.md), [API de firmware](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3), schemas y fixtures existentes; no crear formatos paralelos.

El firmware actual anuncia v2; la app común negocia v1/v2 y conserva el historial v1. Todo evento v2 incluye `origin: real | simulated`. Un fixture no prueba integración física. MQTT es anónimo: `device_id`, schema y deduplicación no autentican al emisor.

Las elecciones aún propuestas de implementación (por ejemplo proveedor FCM/plugin Android), límites y umbrales se identifican en los diseños. Revisar decisiones arquitectónicas nuevas con Jefry sin reabrir acuerdos existentes ni exigir investigación extensa previa. Las claves propias del webhook/proveedor/API HTTPS del contrato no son cuentas de usuario MQTT; no añadir registro de usuarios, roles o paneles de permisos. P3 entrega esa integración a P4.

## Orden de trabajo desde develop

1. **P1:** ejecutar las pruebas físicas pendientes de API/Escudo/app y de la imagen final BLE con la [guía P1](../guia-pruebas-persona-1.md). Mantener la integración completa preparada para las entregas de los demás.
2. **P2:** empezar `sentinel-sanitizador-auditoria`, después pasarela → DNS → DoH. La API ya está disponible; validar NAT/DoH durante sus incrementos, con los componentes oficiales primero.
3. **P3:** empezar `sentinel-cebo-http`, después Telnet → eventos/historial → webhook → push servidor. Reutilizar broker anónimo y journal del núcleo.
4. **P4:** verificar broker en la LAN del equipo y preparar laboratorio; desarrollar pantallas Escudo → Sanitizador → Cebo y recepción push. Escudo y app común ya permiten empezar su integración. Las otras pantallas pueden prepararse con fixtures mientras llega el módulo correspondiente; cerrar requiere datos reales.
5. Cada autor integra por PR revisado. Las dependencias de abajo son necesarias para el cierre, no una prohibición de preparar lógica o vistas. No mantener ramas simultáneas que editen el mismo archivo.
6. P1 completa `sentinel-integracion-tres-modos` con recursos, transiciones, OLED/botón y alertas recientes. P4 consolida evidencia; cada dueño corrige su módulo. El cierre exige Escudo, Sanitizador y Cebo completos, webhook y push con app cerrada.

```mermaid
flowchart TD
  C["P1 · Contratos"] --> API["P1 · API y supervisor"]
  C --> APP["P1 · App común"]
  C --> BROKER["P4 · Broker LAN"]
  C --> LAB["P4 · Preparar laboratorio"]
  API --> E["P1 · Escudo por incrementos"]
  API --> S["P2 · Auditor / NAT / DNS / DoH"]
  API --> H["P3 · HTTP / Telnet / eventos"]
  H --> N["P3 · Webhook / push servidor"]
  BROKER --> N
  APP --> UI["P4 · Pantallas y push Android"]
  E --> UI
  S --> UI
  N --> UI
  UI --> I["P1 · Integración núcleo"]
  I --> FIN["Evidencia real y revisión de los cuatro"]
  LAB --> FIN
```

## Encargos y dependencias de integración

Las dependencias comunes de P1 ya están integradas en develop. La tabla distingue los siete encargos originales archivados de los dieciséis aún activos; las casillas reflejan evidencia, no estimaciones de avance. Un archivo histórico conserva su resultado y sus pendientes físicos. [Resultado de la revisión de coherencia](revision-alcance.md).


| Cambio y ficha                                                            | Autor | Tareas y estado | Depende de                                                                                                                    |
| --- | --- | --- | --- |
| [sentinel-module-contracts](sentinel-module-contracts.md) | P1 | 7/7 · archivado | Base existente                                                                                                                |
| [sentinel-firmware-module-api](sentinel-firmware-module-api.md) | P1 | 6/7 · archivado | `module-contracts`                                                                                                            |
| [sentinel-escudo-descubrimiento](sentinel-escudo-descubrimiento.md) | P1 | 4/6 · archivado | `firmware-module-api`                                                                                                         |
| [sentinel-escudo-captura](sentinel-escudo-captura.md) | P1 | 4/6 · archivado | `escudo-descubrimiento`                                                                                                       |
| [sentinel-escudo-detectores](sentinel-escudo-detectores.md) | P1 | 5/6 · archivado | `escudo-captura`                                                                                                              |
| [sentinel-escudo-recuperacion](sentinel-escudo-recuperacion.md) | P1 | 3/6 · archivado | `escudo-detectores`                                                                                                           |
| [sentinel-sanitizador-auditoria](sentinel-sanitizador-auditoria.md) | P2 | 0/6 · activo | `firmware-module-api`                                                                                                         |
| [sentinel-sanitizador-pasarela](sentinel-sanitizador-pasarela.md) | P2 | 0/6 · activo | `sanitizador-auditoria`                                                                                                       |
| [sentinel-sanitizador-dns](sentinel-sanitizador-dns.md) | P2 | 0/6 · activo | `sanitizador-pasarela`                                                                                                        |
| [sentinel-sanitizador-doh](sentinel-sanitizador-doh.md) | P2 | 0/6 · activo | `sanitizador-dns`                                                                                                             |
| [sentinel-cebo-http](sentinel-cebo-http.md) | P3 | 0/6 · activo | `firmware-module-api`                                                                                                         |
| [sentinel-cebo-telnet](sentinel-cebo-telnet.md) | P3 | 0/6 · activo | `cebo-http`                                                                                                                   |
| [sentinel-cebo-eventos-historial](sentinel-cebo-eventos-historial.md) | P3 | 0/6 · activo | `cebo-telnet`                                                                                                                 |
| [sentinel-alertas-webhook](sentinel-alertas-webhook.md) | P3 | 0/7 · activo | `module-contracts`, `broker-lan`, `cebo-eventos-historial`                                                                    |
| [sentinel-alertas-push-servidor](sentinel-alertas-push-servidor.md) | P3 | 0/6 · activo | `alertas-webhook`                                                                                                    |
| [sentinel-app-protocolo-v2](sentinel-app-protocolo-v2.md) | P1 | 5/6 · archivado | `module-contracts`                                                                                                            |
| [sentinel-app-escudo](sentinel-app-escudo.md) | P4 | 0/6 · activo | `app-protocolo-v2`, `escudo-descubrimiento`, `escudo-detectores`, `escudo-recuperacion`                                       |
| [sentinel-app-sanitizador](sentinel-app-sanitizador.md) | P4 | 0/6 · activo | `app-protocolo-v2`, `sanitizador-auditoria`, `sanitizador-doh`                                                                |
| [sentinel-app-cebo](sentinel-app-cebo.md) | P4 | 0/6 · activo | `app-protocolo-v2`, `cebo-eventos-historial`, `alertas-webhook`                                                      |
| [sentinel-app-push](sentinel-app-push.md) | P4 | 0/6 · activo | `app-protocolo-v2`, `app-cebo`, `alertas-push-servidor`                                                                       |
| [sentinel-broker-lan](sentinel-broker-lan.md) | P4 | 0/6 · activo | `module-contracts`                                                                                                            |
| [sentinel-laboratorio-pruebas](sentinel-laboratorio-pruebas.md) | P4 | 0/6 · activo | `module-contracts`                                                                                                            |
| [sentinel-integracion-tres-modos](sentinel-integracion-tres-modos.md) | P1 | 0/7 · activo | `escudo-recuperacion`, `sanitizador-doh`, `cebo-eventos-historial`, `app-escudo`, `app-sanitizador`, `app-push`, `broker-lan` |


## Propiedad de archivos y solicitudes entre compañeros


| Zona                                                                  | Único autor | Cómo pedir un cambio                                                               |
| --------------------------------------------------------------------- | ----------- | ---------------------------------------------------------------------------------- |
| contracts/ y schemas/fixtures de contrato                             | P1          | Describir campo, unidad, límite y caso; el cambio entra por su spec de contrato    |
| Núcleo firmware, sentinel\_api, sketch, SentinelConfig y integración Arduino        | P1          | P2/P3 entregan requisito de hook/config, P1 integra sin editar lógica ajena        |
| firmware/SentinelNode/src/escudo y sus tests/docs                           | P1          | Hallazgo con observación/fixture y resultado esperado                              |
| firmware/SentinelNode/src/sanitizador y sus tests/docs                      | P2          | Mismo procedimiento; P1 solo integra el registro en núcleo                         |
| firmware/SentinelNode/src/cebo y sus tests/docs                             | P3          | Mismo procedimiento; no editar desde laboratorio                                   |
| notifications/ y su package/lock/Compose propios                      | P3          | P4 consume API HTTPS; entrega casos de error/registro                              |
| app/, package.json y package-lock.json raíz                           | P4          | P1 entrega base v2/diagnóstico; P4 consume y mantiene UX/dependencias, coordinando cambios comunes hasta cerrar4.3; no editar a la vez         |
| broker/                                                               | P4          | P3 recibe endpoints/topics; P4 verifica TCP/WS anónimo en la LAN del equipo                   |
| simulator/, docs/lab y consolidación de evidencia                     | P4          | Cada dueño entrega evidencia; el coordinador no se convierte en autor de su módulo |
| docs/architecture, technical-decisions, roadmap y artefactos OpenSpec | P1          | Cambios aprobados de alcance/interfaz quedan registrados por coordinador           |


## Cobertura de funcionalidades obligatorias


| Funcionalidad                                                         | Entrega del dispositivo/servidor                | Pantalla o integración                                        |
| --------------------------------------------------------------------- | ----------------------------------------------- | ------------------------------------------------------------- |
| Escaneo pasivo, SSID/BSSID/canal/RSSI/seguridad y objetivo            | P1 escudo-descubrimiento                        | P4 app-escudo                                                 |
| Inicio/detención/cambio de red por BLE                                | P1 escudo-descubrimiento + captura              | P4 app-escudo                                                 |
| Beacon, Deauth, Disassoc, prioridad objetivo y recorrido              | P1 escudo-captura                               | P4 app-escudo                                                 |
| Ráfagas, desconexiones, Evil Twin contextual e indicios Karma pasivos | P1 escudo-detectores                            | P4 app-escudo, evidencia y límites                            |
| Eventos/métricas/alertas BLE, gráficos y tabla red/canal/tiempo       | P1 detectores + contrato                        | P4 app-escudo                                                 |
| Escudo offline/persistencia/sync/dedup/cobertura                      | P1 escudo-recuperacion + journal base           | P1 app-protocolo-v2 + P4 app-escudo                              |
| Diagnóstico DNS/HTTPS/certificados/portal                             | P2 sanitizador-auditoria                        | P4 app-sanitizador                                            |
| STA+AP autenticado, DHCP, NAT IPv4/subred separada                    | P2 sanitizador-pasarela                         | P4 app-sanitizador                                            |
| Clientes, RSSI/calidad, subida/bajada                                 | P2 sanitizador-pasarela                         | P4 app-sanitizador                                            |
| DNS local, reglas/Sinkhole, excepciones, caché y bloqueos             | P2 sanitizador-dns                              | P4 app-sanitizador                                            |
| DoH TLS, no fallback plano, degradación y recuperación                | P2 sanitizador-doh                              | P4 app-sanitizador                                            |
| HTTP panel ficticio, conexiones/accesos/auth                          | P3 cebo-http                                    | P4 app-cebo                                                   |
| Telnet puerto23, conexiones/input/auth, sin ejecución                 | P3 cebo-telnet                                  | P4 app-cebo                                                   |
| IP/puertos/protocolo/tiempo, cantidad/frecuencia/sospecha             | P3 cebo-eventos-historial                       | P4 app-cebo                                                   |
| Journal circular persistente offline, recuperación/IDs/dedup          | P3 cebo-eventos-historial + journal base        | P1 app-protocolo-v2 + P4 app-cebo                                |
| MQTT real, historial/incidentes/métricas/gráficos/alertas internas    | P3 cebo-eventos-historial                       | P4 app-cebo + broker-lan                                      |
| Webhooks, dedup y control de saturación                     | P3 alertas-webhook                     | P4 app-cebo consulta estado                                   |
| Push móvil incluso app cerrada                                        | P3 alertas-push-servidor                        | P4 app-push; teléfono real obligatorio                        |
| TCP/WS, BLE autenticado, contratos/ACK/replay                        | P1 contratos + API + base                       | P4 broker-lan + P1 app-protocolo-v2                              |
| OLED con alertas recientes/botón, supervisor, cambios seguros y errores | P1 firmware-module-api + integracion-tres-modos (alertas OLED: tarea 1.5) | P4 pantallas reflejan estado, laboratorio recoge evidencia    |
| Datos sensibles, límites y uso solo autorizado                        | Cada autor en su componente                     | P4 laboratorio verifica; no se guardan credenciales recibidas |


## Flujo diario sin conocer OpenSpec

1. El compañero abre su ficha y diseño, revisa las dependencias y usa su rama feat indicada. Cada ficha enumera los archivos que puede modificar y tareas pequeñas con comprobación.
2. Implementa una tarea, compila, realiza una comprobación breve y registra resultados. Mantiene pruebas focalizadas de parsers, reglas, límites, privacidad o dedup cuando sea lógica crítica.
3. Abre PR conforme a CONTRIBUTING con descripción, resultados y ficha/spec enlazadas. Al menos otro integrante revisa. No integrar cambios de contratos improvisados ni código ajeno sin acordarlo con su dueño.
4. Tú revisas coherencia con OpenSpec y actualizas casillas según evidencia. Los compañeros no necesitan CLI ni redactar propuestas. Si usas Codex para implementar, solicita explícitamente aplicar un cambio concreto; no aplicar los 23 juntos.
5. Después de integrar y comprobar cada cambio, tú gestionas sincronización/archivo por el flujo OpenSpec autorizado. No copiar deltas pendientes al inventario principal como si estuvieran terminados ni tratar un archivo con pendientes aceptados como validación física completa.

## Qué queda pendiente y qué se difiere

**Pendiente durante implementación:** pruebas físicas conservadas en los cierres de base/API/Escudo/app y tarea4.3 de diagnóstico; todos los incrementos activos P2/P3/P4 e integración P1. Configurar endpoints y claves propias de integraciones durante sus encargos, y probar BLE, NAT+DoH, persistencia normal, red LAN y recepción push cerrada con evidencia real. No hay una API compartida pendiente de desarrollar antes de empezar.

**Después de la entrega funcional completa:** benchmarks, autonomía, carga prolongada, resistencia flash y campañas extensas de compatibilidad/estabilidad. Ningún detector, servicio, canal de alerta o función del Sanitizador se mueve aquí para declarar éxito antes.

Referencias: [arquitectura](../architecture.md), [decisiones e hipótesis](../technical-decisions.md), [roadmap](../roadmap.md), [contribución](../../CONTRIBUTING.md).
