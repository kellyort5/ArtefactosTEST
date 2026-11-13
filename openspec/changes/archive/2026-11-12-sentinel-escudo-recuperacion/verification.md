# Verificación de sentinel-escudo-recuperacion

Responsable: Persona 1. Fecha: 2026-11-12. Rama: `feat/sentinel-escudo-recuperacion`, creada desde `origin/develop` en `f82514e`, merge del [PR #9](https://github.com/Jefry-P/ArtefactosProyecto/pull/9).

## Alcance y estado

Se reutiliza el journal NVS circular de 64 eventos y el cursor de la base. Los eventos de Escudo continúan por la cola común, serialización/identidad, commit y entrega en vivo. La extracción aprobada `persist_module_event` permite que las pruebas utilicen la misma rutina del runtime; no añade otro almacén, protocolo, UUID, tarea o instancia MQTT. Solo se amplían los tres archivos del núcleo autorizados en [design.md](design.md).

Estado: **3/6 tareas verificadas en software**, con **1.3, 2.1 y 2.2 pendientes** por instrucción del usuario de continuar sin pruebas físicas. El cambio se publica para revisión con esos límites, no se declara terminado el Escudo físico ni la app v2. No se introduce pausa automática sin evidencia de un fallo real de coexistencia.

## Matriz de evidencia

| Tarea | Resultado y alcance |
| --- | --- |
| 1.1 Eventos/journal | Productor real de Escudo → cola común → rutina compartida del runtime → Journal. Un proceso escribe un incidente con transporte desconectado; otro proceso abre archivos nuevos sobre el almacenamiento del host y recupera sus bytes/cursor/identidad originales. Nuevo boot añade evento con cursor siguiente sin reescribir el anterior. Prueba de lógica y archivos del host; no NVS físico. |
| 1.2 Cobertura/gap | Anillo de tres entradas, generado con eventos de módulo/cobertura, muestra sobrescritura y rango disponible. La paginación real informa gap/more/next; repetición conserva identidad. Leer historial no muta selección/captura. Fallo de escritura no confirma persistencia ni entrega en vivo, y el control de parada sigue disponible. |
| 1.3 Coexistencia | Pendiente en ESP32/teléfono. Se mantiene concurrencia primero. Solo se implementará pausa acotada si se observa un fallo bloqueante; no se afirma que exista una política automática nueva. |
| 1.4 Documentación | [Guía de recuperación](../../../../docs/modules/escudo-recovery.md), retención/commit, replay por páginas, identidad, cobertura/gap, fallos y política de pausa. Seis requests cotejados con makeModuleCommand v2. |
| 2.1 Reconexión real | Compilación se comprueba por software; desconexión/reconexión BLE, NVS/reinicio e historial sin duplicados en Android quedan pendientes. El consumidor de contrato de host no acredita la app real. |
| 2.2 Funciones posteriores | Prueba de lógica conserva captura/objetivo y permite parar, iniciar, cambiar objetivo y detectar tras leer historial. Falta comprobarlo en la placa y adjuntar esa evidencia. Casilla abierta. |

## Comprobaciones reproducibles

Con Node 24 en PATH y el entorno PlatformIO del proyecto:

```sh
pio run -d firmware
python3 firmware/test/check_config.py
SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
python3 firmware/test/check_contracts.py
openspec validate sentinel-escudo-recuperacion --strict
openspec validate --specs --strict
```

Primera ejecución ASan/UBSan: PASS para descubrimiento, captura, detectores y las fases `write`, `recover` y `checks` de recuperación. Codec v2/ACK: PASS para los tres recorridos anteriores y recuperación; 19 requests de documentación válidos. Regresión de núcleo/contratos: PASS, incluidos 69 fixtures v2. La revisión final pasó ASan/UBSan en las cuatro suites y los procesos independientes. Codec v2/ACK y páginas completas: PASS para 70/72/30/18/7 mensajes en los cinco recorridos independientes; eventIdentity conserva 6 eventos únicos frente a 6 repeticiones. Los 19 requests de documentación son válidos. Compilación ESP32-S3 N16R8: PASS, RAM estática 154072/327680 bytes y flash de aplicación 1309077/4194304 bytes; no son medidas de ejecución ni benchmarks. Configuración efectiva, OpenSpec del cambio y 12 specs principales en modo estricto, enlaces de documentos afectados y git diff --check: PASS.

`FileStore` pertenece únicamente a las pruebas y escribe en un directorio temporal. En firmware permanece `NvsStore`, con `nvs_set_blob` y `nvs_commit` antes de permitir entrega en vivo. No se llenó flash repetidamente ni se realizaron pruebas prolongadas, benchmarks o ataques activos.

## Archivo anterior

[Detectores](../2026-11-12-sentinel-escudo-detectores/closure.md) se sincronizó a la spec principal `escudo-detection` y archivó con autorización del usuario. Purpose, cuatro requisitos/scenarios, YAML y casilla física 2.1 se conservan. El archivo no acredita las pruebas pendientes de detectores, captura, descubrimiento o API.
