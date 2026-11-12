# Verificación de sentinel-escudo-detectores

Responsable: Persona 1. Rama: `feat/sentinel-escudo-detectores`. Fecha: 2026-11-12.

## Alcance y estado

Los cuatro detectores están conectados a la captura/parser y a los eventos/métricas v2 del firmware. La implementación usa el supervisor, recursos, cola de salida, journal y BLE comunes. No incorpora detecciones a través de MQTT, asociación Wi-Fi ni tráfico activo. Los ejemplos sospechosos se prueban con bytes locales, sin emitir paquetes.

La prueba física **2.1 permanece pendiente**, por instrucción del usuario de continuar conservándola. No se ha grabado la placa ni demostrado recepción BLE real en este incremento. Tampoco se completan los pendientes de [captura](../archive/2026-11-12-sentinel-escudo-captura/closure.md), descubrimiento o API por transitividad.

## Evidencia de software

| Requisito/tarea | Comprobación focalizada |
| --- | --- |
| 1.1 Deauth/desconexión | Umbral inferior/superado, expiración, enfriamiento, descarte retry, filtro de objetivo y limpieza de generación con reloj controlado. |
| 1.2 Evil Twin | Duplicado benigno, canal/RSSI aislados, combinación canal/RSSI, degradación de seguridad y PMF, unknown, referencia expirada y candidato anterior a referencia actualizada. |
| 1.3 Karma | Tres SSID correlacionados positivos; una red repetida, respuesta sin solicitud, cliente/canal distintos, expiración y patrón sin relación con el objetivo no disparan Karma. |
| 1.4 Integración | Órdenes v2 reales del componente de host; fixtures → ingreso/cola → parser → módulo → cuatro incidentes tipados, métricas y capacidades. Sink de host marca origen simulated; firmware usa origen real por contexto. Saturación no bloquea y cuenta pérdidas. |
| 1.5 Documentación | [Guía del módulo](../../../docs/modules/escudo-detectors.md): criterios, límites, ventanas, enfriamiento, retención acotada, significado de métricas, evidencia insuficiente y recorrido físico. |
| 2.1 Hardware | Solo la parte de compilación está comprobada; faltan observaciones benignas de placa pasando por detectores y recepción BLE real. Casilla abierta. |

Comandos reproducibles, desde la raíz con Node 24 y el entorno PlatformIO del proyecto:

```sh
pio run -d firmware
python3 firmware/test/check_config.py
SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
python3 firmware/test/check_contracts.py
openspec validate sentinel-escudo-detectores --strict
openspec validate --specs --strict
```

Primer build PlatformIO/ESP-IDF 5.5.3: PASS, ESP32-S3 N16R8, RAM estática 154072/327680 bytes y flash de aplicación 1309205/4194304 bytes. Estas cifras del enlazador no miden heap/pilas de ejecución ni constituyen un benchmark.

Primera comprobación ASan/UBSan de descubrimiento, captura y los cuatro detectores: PASS; validación de salidas C++ mediante codec v2/correlación ACK: PASS (70/72/29 mensajes de los tres recorridos independientes). Regresión de núcleo/contratos: PASS, incluidos 69 fixtures v2. La revisión final pasó ASan/UBSan y las tres suites con bordes temporales, conservación de enfriamiento/counters y registro de pérdidas. Codec v2/correlación ACK: PASS (70/72/30 mensajes; 13 requests de documentación válidos). Build final: PASS, mismos tamaños de RAM/flash. OpenSpec del cambio y 11 specs principales: PASS estricto. Enlaces de documentos afectados y `git diff --check`: PASS. **Estado: 5/6**, solo 2.1 física pendiente.

## Límites y entrega

No hay evidencia de precisión en aire, BLE físico, coexistencia, cobertura RF continua o carga prolongada. No se ejecutaron benchmarks ni ataques de laboratorio. Los umbrales iniciales no prueban identidad maliciosa; SSID duplicado aislado no genera incidente. Karma puede quedar sin evidencia suficiente al faltar solicitudes/respuestas; cero alertas no significa ausencia de ataques.

El archivo de captura preserva 4/6 tareas verificadas y sus dos pendientes originales; su spec principal conserva Purpose y los tres requisitos/scenarios del delta. La actualización de enlaces se limita a los documentos afectados por ese archivo y al encargo actual.

La nueva rama se creó sobre el último commit local de captura `7663fb4`, cuya fusión en develop se confirmó en el PR #8. Tras fallos transitorios de DNS se actualizó `origin/develop` a `e677bf3` (merge del PR #8). `git diff HEAD origin/develop` no muestra diferencias de árbol y el ancestro común es `7663fb4`; la rama parte del código fusionado. No se necesitó merge, reescritura ni descarte de trabajo.
