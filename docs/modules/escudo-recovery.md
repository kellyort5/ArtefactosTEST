# Escudo: historial, recuperación y coexistencia

Responsable: Persona 1. Cambio: [sentinel-escudo-recuperacion](../../openspec/changes/archive/2026-11-12-sentinel-escudo-recuperacion/proposal.md). Reutiliza [captura](escudo-capture.md), [detectores](escudo-detectors.md), journal NVS, identidad, supervisor y BLE comunes. La recuperación no crea un segundo almacén ni inicia Wi-Fi/MQTT en Escudo.

## Qué se conserva

Los eventos aceptados del módulo pasan por la cola de salida común, se serializan con identidad/tiempo del núcleo, se escriben en el journal y solo entonces se intenta enviarlos en vivo. La falta de conexión BLE no impide el procesamiento ni la persistencia. Un retorno exitoso de `emit_event` significa admisión a la cola, no escritura ya confirmada: un corte antes del commit puede perder eventos pendientes.

El journal de producción conserva **64 eventos de hasta 2048 bytes**, compartidos con los demás modos, en la partición NVS `event_log`. La entrada siguiente reemplaza la más antigua al llenarse; no hay plazo de retención fijo. Cobertura periódica, cambios de objetivo y eventos del sistema también ocupan entradas. No prometer historial ilimitado ni retención indefinida de incidentes.

Se almacenan eventos tipados, sin tramas crudas ni credenciales. Las métricas periódicas son efímeras; los incidentes y los eventos de cobertura sí pasan por el journal. Conservar SSID/BSSID, evidencia y tiempo/calidad originales de los eventos. Si la hora era desconocida, el evento mantiene `timestamp:null,time_quality:"unknown"`; sincronizar la hora después no reescribe el pasado.

## Recuperación BLE v2

Usar cliente BLE v2 autorizado, framing/UUID y [contratos comunes](../../contracts/module-protocol.md). Estos JSON son requests de `makeModuleCommand`, no paquetes GATT completos. La adaptación y persistencia de la app corresponden a Persona 4; la base de app v1 no basta para consumir todos los eventos v2 de Escudo.

1. Reconectar, completar autenticación/cifrado y suscripción a indicaciones. Leer metadata nueva: device, boot, revisión y uptime actuales. No reutilizar comandos vencidos ni destinados al boot anterior.
2. Consultar el estado actual antes de confundir un evento histórico con captura activa. Incluye objetivo, `capture_status` y cobertura vigente:

```json
{"operation":"get_state","args":{}}
```

3. Consultar eventos desde el **cursor durable confirmado** del cliente para ese nodo; usar cero en una primera sincronización. Se recomienda comenzar con páginas pequeñas si hay presión en el transporte:

```json
{"operation":"read_events","args":{"after_cursor":0,"limit":2}}
```

`limit` admite 1–8. El firmware entrega hasta ese número de mensajes de evento separados y después el ACK aplicado, cuyo `result.kind` es `events`. El ACK informa `next_cursor`, `retained_from`, `retained_to`, `more` y `gap`. Su boot corresponde al nodo actual; los eventos recuperados pueden conservar un boot anterior.

4. Guardar y deduplicar cada evento por `eventIdentity(event)` —nodo/message_id—; el firmware retransmite los bytes originales, incluidos versión, boot, seq, identidad y `journal_seq`. No recrear el evento con una identidad nueva por la reconexión.
5. Confirmar y avanzar el cursor durable solo al recibir el ACK correlacionado **y** los eventos de esa página. ACK/indicación BLE no sustituyen la escritura del historial en la app. Si falta un fragmento, evento o ACK, el cliente mantiene el cursor y solicita de nuevo la página con un comando válido. Los duplicados no añaden filas al historial.
6. Si `more` es true, usar el `next_cursor` recibido, sin fabricarlo. Ejemplo ilustrativo para un ACK que devolvió 12:

```json
{"operation":"read_events","args":{"after_cursor":12,"limit":2}}
```

7. Volver a consultar el estado para reconciliar cobertura actual. El replay no detiene, inicia, selecciona ni reactiva módulos por sí mismo. Después de un reinicio, el nodo arranca en Idle; entrar en Escudo, escanear/seleccionar una red observada e iniciar captura explícitamente.

```json
{"operation":"set_mode","args":{"mode":"escudo"}}
```

```json
{"operation":"escudo.scan","args":{}}
```

```json
{"operation":"escudo.capture_start","args":{}}
```

La última orden exige seleccionar antes una tupla real según [descubrimiento](escudo-discovery.md). El cursor durable del journal continúa tras reinicio recuperando el anillo; el contador de mensajes del boot y los contadores de captura/detectores comienzan un boot nuevo. No comparar sus deltas entre boots.

## Huecos, cobertura y fallos

- `gap:true` significa discontinuidad del historial solicitado. Comparar `retained_from` con el cursor solicitado para identificar el prefijo perdido: su número de cursores es `max(0, retained_from - after_cursor - 1)`. Ese cálculo no cuantifica huecos interiores no observados; el rango no garantiza que cada cursor interior sea legible, pues puede haber huecos recuperados. Mostrar la pérdida, conservar lo recibido y continuar la paginación. No presentar el historial como completo ni inventar registros faltantes.
- `retained_from`/`retained_to` describen el journal al formar la página, no una reserva permanente. Si un slot se sobrescribe o falla su lectura mientras se entrega una página, no avanzar el cursor por una entrega incompleta; repetir la consulta desde el cursor confirmado. La nueva consulta puede señalar un gap mayor.
- `escudo.coverage` conserva canal, motivo, inicio/duración monotónicos y descartes. `get_state` aporta la cobertura presente. El replay permite ver los intervalos registrados, pero no fabrica cobertura continua durante BLE, cambios de canal, pausas o un reinicio.
- Hueco de historial y periodo sin captura son condiciones distintas. `gap` no mide pérdida de tramas RF. `coverage_ms` mide tiempo del controlador en captura; no demuestra recepción continua. `dropped_events` informa pérdidas de salida contabilizadas por el productor; no equivale a todos los registros sobrescritos del anillo.
- Una cola llena se contabiliza y no bloquea el callback. Un fallo NVS devuelve degradación/error sin borrar automáticamente la partición ni confirmar persistencia. Mantener control físico y comandos que no dependan del almacenamiento; registrar el fallo y corregirlo.
- No se promete cero pérdidas ante cualquier corte: solo eventos cuyo commit terminó pueden recuperarse, dentro de la retención disponible. No hay microSD, base de datos extra ni almacenamiento privado de Escudo.

## Política de coexistencia

Primero mantener captura/BLE concurrentes sobre la radio y colas existentes. Ni desconectar BLE ni leer el historial justifican parar la captura automáticamente. Selección, captura, recorrido y las cuatro detecciones conservan su alcance.

La coexistencia real está **pendiente de comprobar**. Si una prueba breve revela un fallo que impide controlar o sincronizar, registrar primero el caso y aplicar una pausa acotada únicamente al recuperar la página, con `capture_status:"paused"`, motivo `ble_sync`, cierre de la generación anterior y reanudación controlada. Registrar inicio/duración y resultado, sin prometer recepción durante la pausa. Si el usuario detiene/cambia el modo, o falla la reanudación, no reactivar una captura obsoleta.

Esta es la alternativa condicionada del diseño; no se anuncia una pausa automática implementada ni se introduce sin evidencia del fallo. La tarea 1.3 permanece abierta hasta comprobar concurrencia real y, solo si hace falta, integrar/verificar esa alternativa. No se requiere un benchmark previo ni una campaña prolongada.

## Verificación y entrega

```sh
pio run -d firmware
python3 firmware/test/check_config.py
SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
python3 firmware/test/check_contracts.py
openspec validate sentinel-escudo-recuperacion --strict
```

Usar Node 24 y PlatformIO según [desarrollo](../development.md). Los casos de software deben recorrer el productor Escudo, la cola/serialización/journal comunes, recuperación de identidad/cursor, repetición y un anillo pequeño con sobrescritura. Las pruebas de host acreditan lógica y persistencia en su adaptador; no NVS, radio ni deduplicación de la app Android física.

## Recorrido físico pendiente

1. Conectar ESP32-S3 N16R8 con OLED/botón y teléfono BLE v2 según [cableado/carga](../firmware.md). Registrar firmware/build, modo, identidad y cursor inicial.
2. Entrar en Escudo, elegir red propia e iniciar captura. Comprobar Beacons y lectura/control BLE concurrentes. Mantener 1.3 pendiente si no se ejecutó; registrar un fallo bloqueante antes de optar por pausas.
3. Desconectar BLE; comprobar que la captura continúa y se registra un evento real benigno de estado/cobertura. Esperar su commit y conservar identidad/cursor como evidencia, sin fabricar ataques.
4. Reconectar y recuperar desde el cursor confirmado. Guardar eventos y ACK, repetir una página y comprobar una sola entrada por identidad en el historial de la app.
5. Reiniciar brevemente después del commit, leer metadata y repetir recuperación. El evento conserva boot/ID/tiempo/cursor original. El nodo nuevo conserva device y journal, pero inicia en Idle y con boot distinto.
6. Volver a seleccionar/iniciar Escudo y comprobar captura y cuatro detectores disponibles. Los fixtures cubren patrones sospechosos; no hace falta inyectar ataques para este recorrido benigno.
7. Registrar resultados por tarea: 1.3 coexistencia; 2.1 reconexión/NVS/historial real; 2.2 funciones después de recuperar. Las pruebas físicas pendientes de incrementos anteriores siguen abiertas hasta que exista su evidencia.
