# Evidencia de sentinel-module-contracts

Fecha: **2026-11-11**, America/El_Salvador. Rama: **feat/sentinel-module-contracts**, creada desde `origin/develop` después de integrar la base. Base Git: `f696371`.

Estado: implementación y comprobaciones técnicas terminadas; **contrato aprobado expresamente por el usuario y tarea 2.2 cerrada el 2026-11-11**. Su respuesta fue «Apruebo el contrato; cerrar 2.2». También eligió «Sincronizar los seis requisitos y archivar (recomendado)». La aprobación no se infiere de una prueba automática ni de una opinión de agente.

## Entrega

- Siete esquemas de mensajes v2 y siete de notificaciones, objetos cerrados y tipos públicos con prefijo V2.
- 25 operaciones, cuatro perfiles y scopes, 13 variantes de eventos, cuatro detectores y 11 clases de resultado ACK.
- Codec v2 y entradas Any; las APIs, esquemas, tipos, encuadre BLE y fixtures v1 conservan su comportamiento.
- Admisión pura por autenticación, autorización, cifrado, canal/nodo/boot/vigencia y comprobación de revisión/modo para órdenes nuevas. La caché y la ejecución siguen siendo responsabilidad del receptor real.
- Correlación ACK/operación, límites solicitados de página, progreso de cursor y rango del journal; el consumidor debe acreditar los eventos separados antes de avanzar su cursor durable.
- Registro/baja/status de notificaciones y payload push con validación de contexto por nodo y event_id original. Son contratos ejecutables y ejemplos, no un servidor ni un envío real.
- `payload.origin:real|simulated` obligatorio en eventos v2, **aprobado expresamente por el usuario** durante esta implementación; origen preservado durante replay y sin modificar eventos históricos v1.

Referencia para consumidores: [protocolo y codebook](module-protocol.md), [notificaciones](notifications.md), [tipos v2](src/v2-types.ts), [spec principal](../openspec/specs/module-protocol/spec.md) y [cierre del cambio](../openspec/changes/archive/2026-11-11-sentinel-module-contracts/closure.md). Cada operación tiene ejemplo y consumidor identificados en la tabla del protocolo.

## Comprobaciones ejecutadas

Entorno: Node **24.13.1**, dependencias del lockfile existente, sin nuevas dependencias.

| Comando o revisión | Resultado |
| --- | --- |
| `npm run check:contracts` | PASS: compila TypeScript y pasan los cuatro archivos de pruebas. |
| `node --test --test-isolation=none contracts/test/*.test.mjs` | **153/153 PASS**, incluidos los 24 casos v1, contratos de notificaciones y casos de código/fixtures v2. El modo sin aislamiento muestra los casos individuales en este entorno. |
| `node --import tsx --test --test-isolation=none app/test/*.test.ts` | **9/9 PASS**, reconciliación e historial de la app v1. |
| `npm run build` | PASS: `npm run build -w contracts` y compilación TypeScript/Vite de app; aviso informativo por tamaño de chunk, sin error. |
| `python3 firmware/test/check_contracts.py` | PASS: lógica portátil C++, serialización C++→AJV y comandos v1 compartidos→cJSON. No es prueba de radio ni NVS física. |
| `openspec validate sentinel-module-contracts --strict --json` | PASS, sin incidencias. |
| `git diff --check -- contracts openspec/changes/sentinel-module-contracts` | PASS; revisión adicional de whitespace y contenido en archivos nuevos. |
| Conservación v1 | **23 archivos idénticos a HEAD**: seis esquemas v1, 15 fixtures y `types.ts`/`ble.ts`. El test v1 solo adapta la búsqueda de fixtures para no leer el directorio v2 como JSON. |
| Regeneración | Ejecutar `node contracts/tools/generate-v2-schemas.mjs` conserva exactamente los hashes de esquemas/tipos mantenidos. |
| Alcance, enlaces y secretos | PASS sobre código, esquemas, fixtures y documentación mantenidos de contratos, comparando también las seis contraseñas privadas existentes sin imprimirlas; ninguna copia de credenciales ni clave privada encontrada. |

Las últimas comprobaciones se ejecutaron sobre el código integrado después de corregir los hallazgos del revisor. Los logs de trabajo quedan en `.cache/module-contracts-check/`, fuera de Git; este informe conserva los comandos y resultados reproducibles.

## Revisión mediante Orca

Run `run_c55d0c53d938`, con archivos separados y un único responsable de integración:

| Trabajo | Evidencia |
| --- | --- |
| Esquemas/tipos | `ctx_2bb620eda1da`: entregó los siete esquemas, tipos, fuente reproducible y suite AJV independiente; corrigió anclas de fin real para impedir newline en IDs/dominios/cursores. |
| Contrato de notificaciones | `ctx_1b35c1417bd0`: entregó siete esquemas, helpers puros, ejemplos JSON/HTTP, documentación y pruebas. |
| Revisión inicial de consumidor | `ctx_2f4e225051ac`: confirmó el enfoque de APIs separadas v1/v2 y detalló correlación, paginación, codebooks, origen y límites de los consumidores futuros. |
| Revisión de implementación | `ctx_4d8e20b59505`: aprobó técnicamente el vocabulario/codec con dos límites menores, corregidos y cubiertos por pruebas posteriormente. No sustituye revisión humana ni acredita adaptadores físicos. |

El primer intento de reabrir la revisión, `ctx_3cc3439870b9`, falló por el aviso de actualización del CLI, no por Sentinel. Se liberó ese intento y se reintentó la misma tarea en una sesión temporal verificada de Orca; no se actualizó Codex ni se cambió su configuración global. La tarea de revisión terminó con informe verificable.

Hallazgos e integración resueltos:

1. La suite v1 intentaba leer `fixtures/v2` como archivo: filtra los JSON de su nivel, conservando todos sus casos y semántica.
2. El portal de auditoría admite HTTP o HTTPS; el codec se alineó con el schema.
3. Se corrigieron patrones v2 que podían aceptar un newline final. V1 permanece intacto.
4. Se rechazan surrogates aislados tanto literales como escapados, antes de cualquier sustitución silenciosa de TextEncoder.
5. Un ACK de catálogo no supera el límite solicitado ni devuelve el mismo cursor no terminal; un ACK del journal no retrocede ni avanza sobre huecos sin declararlos.
6. El resultado se correlaciona con operación, nodo, command_id/boot y datos solicitados de objetivo/regla/auditoría. La revisión y el modo para órdenes nuevas se comprueban después del lookup de duplicados, evitando repetir efectos o invalidar respuestas ya guardadas.

## Límites y siguiente paso

### Revisión posterior del alcance: canales de notificaciones

El 2026-11-11 el usuario retiró Telegram del módulo Cebo porque utilizará la app móvil. Se ajustaron el tipo, esquema y fixtures de estado para exigir únicamente `webhook` y `push`; las alertas internas, registro Android y recepción push con app cerrada se conservan. Se añadió una regresión que acepta ambos canales, rechaza su ausencia y rechaza el campo retirado como propiedad desconocida. `npm run check:contracts` compila y pasa los cuatro archivos de pruebas con esta revisión.

Este ajuste posterior no reescribe los artefactos archivados ni los resultados originales: documenta la instrucción vigente y la corrección del contrato antes de implementar sus consumidores. Los límites, autenticación y privacidad se conservan; no se implementa ningún servidor o proveedor de notificaciones.

Las pruebas son de contratos y lógica real de la biblioteca. No acreditan captura/detectores en ESP32, NAT/DoH, HTTP/Telnet, pairing/TLS negociados, persistencia física ni recepción push. El cliente nuevo y firmware v2 se integrarán en sus specs; la app actual conserva su API v1.

Los objetos cerrados no autentican por sí mismos el origen ni eliminan un secreto que un productor inserte en un campo permitido. El receptor deriva autorización de una sesión confiable; el productor sanitiza y limita los datos conforme al codebook. El estado de notificaciones distingue aceptación por proveedor de lectura, que permanece desconocida.

La revisión final del usuario sobre la entrega del [protocolo](module-protocol.md), la [API de notificaciones](notifications.md) y esta evidencia quedó aprobada mediante su respuesta explícita; la casilla **2.2** está completa. El usuario autorizó publicar el trabajo en ramas y PR hacia develop antes del archivo. Contratos se publicó primero en [PR #2](https://github.com/Jefry-P/ArtefactosProyecto/pull/2) y la planificación del equipo en [PR #3](https://github.com/Jefry-P/ArtefactosProyecto/pull/3). Los ajustes de cierre se publican en esas mismas ramas. No se autorizaron ni ejecutaron merges; la aprobación del contrato no acredita las integraciones físicas pendientes.
