# Guía de Persona 1 — Jefry: coordinación, núcleo y Escudo

Tu resultado es entregar las interfaces que consumen los compañeros, el Escudo completo y la integración del núcleo. También mantienes OpenSpec para todos. Los compañeros implementan sus encargos y aportan evidencia; tú no debes absorber sus tareas para actualizar las specs.

## 1. Tu lectura y punto de partida

Leer la [guía del equipo](guia-equipo.md), el [reparto](README.md), la [arquitectura](../architecture.md), [R-02 a R-07 y R-11, con H-01 a H-08](../technical-decisions.md) y el [roadmap](../roadmap.md).

Para los contratos, consultar [protocolo v2](../../contracts/module-protocol.md), [notificaciones](../../contracts/notifications.md), [evidencia](../../contracts/evidence-v2.md) y [spec principal](../../openspec/specs/module-protocol/spec.md). El usuario aprobó el contrato y el cierre de 2.2; el cambio está [archivado con 7/7 tareas](../../openspec/changes/archive/2026-11-11-sentinel-module-contracts/closure.md) y publicado en el PR #2. No repetir su implementación ni confundir publicación con merge.

Revisar [cierre de la base](../../openspec/changes/archive/2026-11-10-sentinel-core-foundation/closure.md) y [prueba física](../core-foundation-test-guide.md). Las pruebas físicas asumidas al archivar la base siguen pendientes de evidencia.

## 2. Tus siete encargos en orden

Cada enlace abre una ficha con diseño, specs y tareas oficiales. La rama prevista es `feat/<nombre completo del cambio>`.

| Orden | Encargo propio | Trabajo concreto | Entrega que desbloquea |
| --- | --- | --- | --- |
| 1 — completado | [sentinel-module-contracts](sentinel-module-contracts.md) | Contrato v2 aprobado, publicado y archivado; conservar compatibilidad v1, 25 operaciones, ACK/páginas, origen, privacidad y contrato HTTP/push. Consultar su cierre y reutilizar la biblioteca. | P2/P3 conocen mensajes; P4 puede adaptar app/broker; todos comparten fixtures. |
| 2 — software entregado; física pendiente | [sentinel-firmware-module-api](sentinel-firmware-module-api.md) | Publicar `sentinel_api`, hooks/lifetime, registro opcional, dispatcher v2, recursos prestados, eventos/métricas y supervisor. Integrar flags globales de forwarding/NAPT y controles físicos según diseño revisado. | P2/P3 pueden integrar módulos sin editar el núcleo; P4 prueba comandos/capacidades reales. |
| 3 — software archivado; física pendiente | [sentinel-escudo-descubrimiento](sentinel-escudo-descubrimiento.md) | Escaneo pasivo 2.4 GHz, catálogo limitado/paginado con SSID/BSSID/canal/RSSI/seguridad, selección y cambio de objetivo por BLE. | P4 conecta catálogo y selección de red. |
| 4 | [sentinel-escudo-captura](sentinel-escudo-captura.md) | Modo promiscuo, cola limitada, parser Beacon/Deauth/Disassoc y metadatos Probe, prioridad de objetivo, recorrido de canales, cobertura y start/stop. | Flujo de observaciones para todos los detectores y métricas de captura. |
| 5 | [sentinel-escudo-detectores](sentinel-escudo-detectores.md) | Ráfagas Deauth, desconexión sospechosa, Evil Twin contextual e indicios Karma pasivos. Conectar los cuatro al parser, eventos y métricas v2. | P4 puede integrar todos los incidentes; P4 recibe casos positivos/negativos para laboratorio. |
| 6 | [sentinel-escudo-recuperacion](sentinel-escudo-recuperacion.md) | Journal compartido, recuperación tras BLE/reinicio, cursor/gap y cobertura. Intentar captura+BLE concurrentes; si fallan, pausa controlada con cobertura visible y todas las funciones conservadas. | Cierre del Escudo y prueba conjunta de historial sin duplicados. |
| 7 | [sentinel-integracion-tres-modos](sentinel-integracion-tres-modos.md) | Registrar módulos entregados, verificar parada/exclusión, OLED/botón y resumen de la alerta más reciente, reconciliación de estado/transportes/journal y recorrido final de tres modos. | Entrega conjunta de ESP32/app/alertas con evidencia de todos. |

El encargo 7 espera las dependencias enumeradas en su ficha. No es una autorización para programar las pantallas o reparar el código Sanitizador/Cebo de otro autor.

La **tarea 1.5 del encargo 7 es tuya**: conectar las alertas del núcleo a la OLED y comprobar su resumen físicamente, sin depender de abrir la app. Cubrir ausencia de alertas, sustitución por una nueva, módulo de origen, tiempo disponible y prioridad del código de emparejamiento. Las demás diferencias respecto al PDF se documentan en la [comparación de alcance](../pdf-scope-comparison.md); no se añaden nuevas funciones por esa revisión.

## 3. Qué hacer primero

1. Confirmar con el equipo la referencia que incorpora PR #2 de contratos y PR #3 de planificación; ambos están publicados hacia develop, sin merge automático. El contrato ya está aprobado y archivado. Mantener fuera los cambios ajenos existentes en tu copia.
2. Revisar con P2/P3 el [diseño de API](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/design.md), en particular hooks, parada, callbacks y préstamo de netifs. Solicitar revisión de decisiones arquitectónicas nuevas importantes; resolver detalles rutinarios dentro del diseño acordado.
3. Entregar la API en un incremento pequeño con build y consumidor mínimo de prueba. Documentar las firmas reales en `docs/development.md` dentro de ese encargo; la API real ya está documentada allí y fusionada en develop mediante PR #6.
4. Comunicar a P2/P3 la referencia de integración, headers reales, ejemplo de ciclo de vida, flags disponibles y límites. A P4, comunicar cómo leer capacidades y enviar un comando v2 confirmado.
5. Comenzar descubrimiento Escudo mientras P2 avanza auditoría, P3 HTTP y P4 app/broker. Mantener revisiones acotadas para no convertirte en una cola de aprobaciones de cada línea.

## 4. Límites de tu implementación

Eres autor de `contracts/`, núcleo `sentinel`, futuro `sentinel_api`, `main`, configuración global y futuro componente `escudo`, con sus tests/documentación. Las rutas específicas se detallan en cada ficha.

- Un solo supervisor y gestor de radio. Los módulos solicitan recursos; no crean drivers/supervisores alternativos. Solo un modo activo por ESP32.
- El núcleo crea identidad/cursor, valida y persiste eventos. Reutilizar el journal NVS de la base; P2/P3 no implementan otros journals en paralelo.
- En callbacks, copiar datos a colas acotadas; evitar flash o trabajo bloqueante. Respetar lifetime y parada antes de conceder recursos al siguiente modo.
- Escudo nunca se asocia a la red auditada ni usa MQTT directo. BLE sigue siendo su control y salida.
- SSID duplicado no es prueba de malicia. Los cuatro detectores publican indicios, evidencia y límites; no prometen certeza o cobertura continua no demostrada.
- P4 controla `app/`, `broker/`, simulador y lockfile raíz. P3 controla `notifications/`; P2 controla Sanitizador. Solicitar cambios a su dueño.

## 5. Cómo usar OpenSpec en tu día a día

Las 23 propuestas ya existen. No volver a proponerlas ni aplicar todas juntas. Antes de cada cambio, revisar sus artefactos y dependencias. En una sesión con las skills del repositorio puedes pedir, por ejemplo:

```text
$openspec-apply-change sentinel-escudo-detectores
Lee AGENTS.md, su ficha, diseño, specs y tareas. Implementa solo este cambio,
preserva trabajo ajeno y verifica compilación y comportamiento básico.
No hagas commit, push, merge ni archivo sin autorización explícita.
```

Después, repetir con cada nombre de la tabla cuando corresponda. Esa línea es una instrucción para el asistente, no un comando de shell. Para consultar desde terminal:

```sh
openspec status --change sentinel-escudo-detectores --json
openspec instructions apply --change sentinel-escudo-detectores --json
openspec validate sentinel-escudo-detectores --strict --json
```

Para P2/P3/P4: pedir tarea numerada, resultado, referencia y evidencia. Leer la línea original de `tasks.md`, contrastarla con el comportamiento y marcar únicamente lo demostrado. Una aceptación de pendientes se documenta como aceptación, sin convertirla en prueba realizada. Si cambia alcance/interfaz, mantener ficha, diseño, spec y contrato coherentes antes del consumidor.

La sincronización/archivo se gestiona después de implementar y revisar el cambio por su flujo OpenSpec autorizado. No copiar deltas pendientes a specs principales como si estuvieran terminados.

## 6. Comprobaciones y coordinación

Desde raíz, con dependencias preparadas según [desarrollo](../development.md):

```sh
npm run check:contracts
npm run build -w contracts
python3 firmware/test/check_contracts.py
pio run -d firmware -e sentinel_s3
```

Ejecutar las comprobaciones que corresponden al cambio y añadir tests focalizados de exclusión, admisión/duplicados, parsers, detectores o límites. Compilar no demuestra BLE/captura/NVS físicos. Para grabar y observar la placa, seguir la [guía física](../core-foundation-test-guide.md) y acordar el turno de hardware.

| Con quién | Lo que tú entregas | Lo que necesitas recibir |
| --- | --- | --- |
| P2 | Contrato, hooks, netifs y flags de núcleo. | Registro/requisitos de Sanitizador, resultados NAT/DNS/DoH y errores reproducibles. |
| P3 | Contrato, hooks, journal y publicación central. | Registro Cebo, requisitos de recursos y evidencia de servicios/persistencia/alertas. |
| P4 | Operaciones, capacidades y observaciones Escudo reales. | UI/transportes integrados, broker LAN y matriz de evidencia; él mantiene los cambios de app. |

Tu cierre requiere Escudo completo y los tres modos integrados sin recursos residuales, OLED/botón reales, recuperación, app y canales externos comprobados. Devolver cada fallo al dueño de su encargo y conservar el alcance completo. Usa el [formato de entrega común](guia-equipo.md#6-rutina-por-tarea-y-entrega).

[Volver al reparto](README.md) · [Guía del equipo](guia-equipo.md)
