# Cómo trabajar con las guías de Sentinel Node

**Acuerdo vigente (2026-11-17):** [MQTT anónimo, entrega P1 y reparto actualizado](../mvp-policy.md). Sin TLS/cuentas/roles/ACL MQTT; BLE y HTTPS/DoH conservan sus protecciones. Evidencia histórica no sustituye las pruebas físicas pendientes.

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

Estas guías explican el reparto existente; no añaden funcionalidades, cambian la arquitectura ni acreditan implementaciones pendientes. Cada spec tiene **un único autor**. Otro integrante revisa su entrega sin tomar sus tareas de implementación.

| Integrante | Guía personal | Resultado a su cargo |
| --- | --- | --- |
| Persona 1 — Jefry | [Coordinación, contratos, núcleo y Escudo](guia-persona-1.md) | Interfaces compartidas, Escudo completo, integración del núcleo y seguimiento OpenSpec. |
| Persona 2 | [Auditor y Sanitizador](guia-persona-2.md) | Auditoría, STA+AP/NAT IPv4, clientes/tráfico, DNS y DoH. |
| Persona 3 | [Cebo y servidor de alertas](guia-persona-3.md) | HTTP/Telnet, clasificación y persistencia, webhook y envío push.           |
| Persona 4 | [App, broker y laboratorio](guia-persona-4.md) | UI de los tres módulos, Android/BLE/WS/push, broker LAN y evidencia conjunta. |

## 1. Qué leer y para qué sirve

1. [Brief](../brief.md): propósito del proyecto.
2. [Reparto](README.md) y tu guía: responsabilidad, orden y dependencias.
3. [Arquitectura](../architecture.md), [decisiones técnicas e hipótesis](../technical-decisions.md) y [roadmap](../roadmap.md): alcance aprobado, restricciones y validaciones pendientes. El Sanitizador ya comienza en paralelo y forma parte de la entrega conjunta.
4. La ficha `docs/encargos/sentinel-<cambio>.md` de tu siguiente encargo: pasos concretos, archivos propios y comprobaciones.
5. Los documentos de `openspec/changes/<cambio>/` enlazados desde la ficha:

| Archivo | Cómo usarlo |
| --- | --- |
| `proposal.md` | Entender el problema y los límites del encargo. |
| `design.md` | Seguir la integración, propiedad de recursos y alternativas propuestas. Distinguir lo aprobado de lo que aún pide revisión. |
| `specs/<capacidad>/spec.md` | Convertir requisitos y escenarios en comportamiento comprobable. Leerlos antes de programar. |
| `tasks.md` | Seguir tareas numeradas; informar resultados con esos números. Jefry mantiene las casillas oficiales. |

`openspec/specs/` contiene las especificaciones consolidadas; `openspec/changes/` contiene incrementos y deltas. Un cambio propuesto no significa que su comportamiento ya exista. Si se archiva un cambio, seguir su referencia actualizada en el índice, sin recrearlo.

Ante una contradicción entre ficha, spec, contrato o decisión aprobada, indicar los dos archivos a Jefry. No escoger silenciosamente una interpretación ni alterar un contrato para acomodar tu implementación. Las guías son ayudas de ejecución; no sustituyen las fuentes anteriores.

## 2. Estado desde el que parte el equipo

- La base está [archivada con aceptación de pendientes](../../openspec/changes/archive/2026-11-10-sentinel-core-foundation/closure.md): 26/45 tareas verificadas y 19 comprobaciones aceptadas bajo una suposición de funcionamiento físico. No son 19 pruebas realizadas.
- `sentinel-module-contracts` está implementado, aprobado por el usuario y [archivado con 7/7 tareas](../../openspec/changes/archive/2026-11-11-sentinel-module-contracts/closure.md). Ver [evidencia](../../contracts/evidence-v2.md) y [spec principal](../../openspec/specs/module-protocol/spec.md). Está publicado en el [PR #2](https://github.com/Jefry-P/ArtefactosProyecto/pull/2); no rehacer la biblioteca ni asumir que un PR abierto ya está integrado.
- Escudo tiene implementación y tests de software; sus pruebas físicas continúan pendientes. Sanitizador/Cebo y las pantallas específicas describen trabajo futuro. La base original usó v1. El firmware actual y la app común de P1 ya consumen v2; los eventos históricos v1 se conservan. Las pantallas específicas y los servicios de Sanitizador/Cebo siguen en sus encargos.
- Las fichas/specs/guías están publicadas en el [PR #3](https://github.com/Jefry-P/ArtefactosProyecto/pull/3), con dependencia de #2. Confirmar con Jefry la referencia de partida que incorpora ambas entregas antes de trabajar; no asumir merges por el hecho de estar publicadas.

## 3. Dónde está cada cosa

| Ruta | Qué consultar o implementar | Autor |
| --- | --- | --- |
| `contracts/` | Protocolo, tipos, validadores, esquemas y ejemplos compartidos. | P1 |
| `firmware/SentinelNode/src/sentinel/`, `firmware/SentinelNode/src/sentinel_api/`, `firmware/SentinelNode/SentinelNode.ino` y configuración global | Supervisor, radio, transportes, journal y registro de módulos. | P1 |
| `firmware/SentinelNode/src/escudo/` | Descubrimiento, captura y detectores. | P1 |
| Futuro `firmware/SentinelNode/src/sanitizador/` | Auditoría, pasarela, DNS y DoH. | P2 |
| Futuro `firmware/SentinelNode/src/cebo/` | Servicios ficticios y clasificación. | P3 |
| Futuro `notifications/` | Componente de alertas con package/lockfile y Compose propios. | P3 |
| `app/`, `package.json` y `package-lock.json` raíz | App y dependencias; excepción: app-protocolo-v2 y diagnóstico P1 corresponden a P1. | P4; P1 mantiene también diagnóstico/infraestructura común entregada y coordina sus cambios |
| `broker/` | Mosquitto, TCP/WS anónimos y preparación LAN. | P4 |
| `simulator/`, futuros `docs/lab/` y `docs/evidence/` | Reproducción de mensajes, procedimientos e informes conjuntos. | P4 |
| Arquitectura, decisiones, roadmap y seguimiento OpenSpec | Coherencia y estado de los acuerdos del equipo. | P1 |

Las carpetas indicadas como futuras se crean dentro de su encargo, no como una estructura paralela improvisada. Los tests y documentos particulares del módulo pertenecen a su autor, según la ficha. P1 integra los cambios globales solicitados por P2/P3; no reimplementa sus módulos.

## 4. Usar contratos sin inventar mensajes

Un contrato acuerda qué datos envía cada componente y cómo se confirman. No conecta la radio ni ejecuta una orden por sí mismo.

1. Buscar la operación en [module-protocol.md](../../contracts/module-protocol.md).
2. Abrir su ejemplo en [fixtures/v2/commands](../../contracts/fixtures/v2/commands/), el resultado en [acks](../../contracts/fixtures/v2/acks/) y el evento/estado/métrica asociado en [fixtures/v2](../../contracts/fixtures/v2/).
3. Consultar nombres, enums y unidades en el protocolo, [v2-types.ts](../../contracts/src/v2-types.ts) y [schemas/v2](../../contracts/schemas/v2/). Un fixture es un ejemplo, no todos los casos permitidos.
4. En TypeScript, consumir `@sentinel/contracts` y sus validadores; no duplicar interfaces o usar `JSON.parse` como única validación. En C/C++, implementar la adaptación contra los mismos esquemas/fixtures mediante la API del núcleo: el paquete TypeScript no se ejecuta en ESP32.
5. Compilar y validar la salida real. Si falta un dato, solicitar a P1: operación, campo, tipo, unidad, límite, motivo y ejemplo ficticio. No añadir propiedades libres a los objetos cerrados.

Reglas que todos deben conservar:

- La app negocia v1/v2 y conserva eventos históricos v1. El firmware actual anuncia v2 y rechaza órdenes v1 con `unsupported_version`; no mandar operaciones v2 a un equipo v1.
- Mensaje ESP completo: máximo **2048 bytes UTF-8**; Metadata: **256 bytes**. Fragmentar BLE no permite mensajes lógicos mayores. Aplicar paginación y límites del contrato.
- Eventos v2: `payload.origin` obligatorio, `real` para observaciones físicas y `simulated` para reproducción de prueba. Conservarlo durante el replay. El historial v1 permanece intacto.
- Identidad de evento: `(device_id,message_id)`. Retransmitir conserva identidad; no crear un incidente nuevo por cada llegada. El push usa ese `message_id` como `event_id`.
- ACK `accepted` indica aceptación de trabajo, no finalización. Respetar `applied`/`rejected` y, en trabajos asíncronos, resultado posterior correlacionado.
- BLE exige vínculo autenticado/cifrado; MQTT no tiene cuentas ni roles. Nodo, boot, revisión, vigencia, duplicados y retained se comprueban en el receptor, sin autenticar al emisor MQTT. Un schema válido no autentica al emisor. Seguir el orden de admisión/caché del protocolo.
- Escudo se controla por BLE sin asociarse a la red auditada ni publicar MQTT directo. Sanitizador usa BLE para control. Cebo publica MQTT y admite sus canales definidos.
- Sin contraseñas capturadas, tokens, cookies ni cuerpos completos en eventos, estado, logs, fixtures o informes. Las credenciales propias se configuran fuera de Git.

P3/P4 también leen [notifications.md](../../contracts/notifications.md): es el contrato HTTPS de registro/baja/estado y push. La biblioteca valida datos; implementar autenticación, persistencia y entrega corresponde a los encargos del servidor/app.

## 5. Preparar tu copia y tu rama

Leer [CONTRIBUTING.md](../../CONTRIBUTING.md) y [AGENTS.md](../../AGENTS.md). Usa un clon propio; los cuatro no editan una misma carpeta de trabajo. Si aún no tienes la documentación o el contrato revisado, pide a Jefry la referencia publicada antes de empezar.

En este trabajo la referencia de integración usada por la base y los contratos es `develop`; `main` conserva la versión estable. **Confirmar con Jefry la base vigente y que contiene las dependencias**: CONTRIBUTING permite elegir la base adecuada y menciona normalmente `main`. No se añade aquí un proceso completo de releases/hotfixes ni se asume que todos los PR anteriores estén integrados.

Ejemplo para una copia limpia, con dependencias ya revisadas e integradas en `origin/develop`:

```sh
git status --short
git branch --show-current
git fetch origin
git switch -c feat/sentinel-sanitizador-auditoria origin/develop
```

Sustituye la rama por la de tu ficha. Si ya existe, revisa su contenido y vuelve a ella; no la recrees. Si hay trabajo pendiente, no cambies de rama automáticamente ni hagas stash/descarte masivo: acuerda cómo conservarlo. Si una dependencia no está integrada, pide una referencia explícita a Jefry; no mezcles ramas ajenas por tu cuenta.

Una spec por rama/PR y un autor por spec. Trabaja las specs sucesivas de una misma carpeta en orden. Solicita actualización explícita de dependencias si necesitas continuar antes de integrar el PR anterior.

## 6. Rutina por tarea y entrega

1. Leer ficha, diseño, spec y tarea numerada.
2. Implementar un incremento pequeño en tus archivos; pedir al dueño cualquier cambio compartido.
3. Compilar, hacer una comprobación funcional breve y corregir errores bloqueantes. Mantener tests focalizados de lógica crítica; sin benchmarks previos ni infraestructura compleja.
4. Revisar `git diff`, `git status --short` y archivos nuevos. Si preparas un commit, revisar además `git diff --cached`. No incluir cambios ajenos, secretos o artefactos.
5. Entregar PR pequeño con Conventional Commits, ficha/spec enlazadas y evidencia. Al menos otro integrante revisa. Codex necesita autorización explícita para commit, push, merge y eliminación de ramas; esta guía no la otorga.
6. Informar a Jefry con los números originales de tarea. Él contrasta la evidencia y actualiza OpenSpec; una casilla marcada no sustituye el resultado real.

Formato que puedes copiar a la descripción del PR o al informe del encargo:

```text
Encargo y rama:
Tareas cubiertas: 1.1, ...
Archivos/componentes cambiados:
Dependencias y referencia probada (commit si existe):
Comandos de compilación/tests y resultados:
Comprobación funcional: entorno, pasos, esperado y obtenido:
Evidencia real o fixture; IDs sin secretos:
Limitaciones y tareas PENDIENTES:
Solicitud a otro dueño, si existe:
```

Usar PASS/FAIL/PENDIENTE por caso. No cerrar una funcionalidad real con una pantalla aislada o un mock. P4 consolida los informes; cada autor produce y corrige la evidencia de su componente. Si solo hay una ESP32, acordar turnos y registrar qué versión se grabó.

La preparación de fixtures puede avanzar mientras llega hardware/API. El cierre requiere la integración pertinente. Las campañas extensas de rendimiento, autonomía, desgaste y compatibilidad quedan después de la entrega funcional, sin retirar funciones de ninguno de los tres módulos.

[Volver al reparto](README.md)
