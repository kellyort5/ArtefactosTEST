# Revisión de encargos y OpenSpec — 17 de noviembre de 2026

Revisión documental contra los [acuerdos vigentes](../mvp-policy.md), [persistencia](../persistence-policy.md), contratos y fuentes de arquitectura. No implementa funciones ni acredita pruebas físicas.

## Cobertura y correcciones

Se contrastaron las **23 fichas originales**, las cuatro guías personales, la guía común y el índice con sus artefactos OpenSpec. Los **16 encargos activos** conservan su autor y sus dependencias; se reconciliaron propuesta, diseño, requisitos y tareas con la entrega actual de develop. Las siete fichas archivadas conservan el seguimiento histórico.

| Problema encontrado | Corrección |
| --- | --- |
| Configuración de SDK propio y placa retirada en planes P2/P3 | Core oficial ESP32 3.3.12 y ESP32S3 Dev Module N16R8; bibliotecas/opciones de la guía vigente. No reconstruir SDK por defecto para NAT. |
| Cuatro tareas del broker diferentes de su ficha | Misma redacción operativa para TCP/WS anónimo, acceso LAN, integración P3 y evidencia real. |
| Referencia a MQTT seguro/identidad autorizada en eventos Cebo | MQTT anónimo del núcleo; device_id/topic identifican destino y evento, sin autenticar al emisor. |
| Dos deltas anteriores aún exigían cuenta MQTT en configure_connectivity | Alineados con la spec principal y el schema actual, para que una sincronización futura no restaure campos retirados. |
| Persistencia descrita en specs P2 pero poco explícita en pasos | Tareas de AP, DNS y DoH incluyen guardado/commit y recuperación en Idle tras reinicio/corte sin USB. Auditoría conserva todos sus criterios. |
| Índice decía que la API seguía pendiente y pedía integrar PR originales | Estado actual por encargo, API disponible, app común/diagnóstico entregados y siguientes pasos por persona. |
| Enlaces de verificación apuntaban a una versión anterior | Referencia a la verificación integrada de MQTT anónimo; evidencia antigua sigue siendo histórica. |

Se mantiene todo R-03/R-08/R-10, incluidas alertas recientes OLED de P1 (integración, 1.5), gráficos/pantallas finales P4, webhook y push con app cerrada. Telegram sigue retirado. La salida serie no sustituye la aceptación física de OLED.

No hay tareas de crear usuarios, roles, ACL ni CA MQTT. BLE, permisos Android y validación HTTPS/DoH permanecen. Las claves técnicas propias del contrato HTTPS de notificaciones y de proveedores se conservan fuera de Git; no son gestión de usuarios ni credenciales MQTT.

## Comprobaciones de esta revisión

- Los números y el texto de las **98 tareas de los 16 encargos activos** coinciden con sus fichas; no se añadieron ni borraron tareas numeradas ni se alteraron casillas.
- Las 23 fichas conservan un solo responsable; el grafo de dependencias no tiene ciclos.
- Los enlaces locales y referencias a artefactos se revisan junto con las fuentes; las rutas de futura implementación siguen indicadas como futuras.
- `openspec validate --all --strict --json`: **41/41** (27 cambios y 14 specs principales). Esto valida estructura; la comparación de tareas y el examen de requisitos cubren además la coherencia documental.
- Solo documentación y planificación activa cambian. Los archivos bajo `openspec/changes/archive/` permanecen intactos.

## Excepciones históricas explícitas

Cuatro fichas P1 usan Arduino para la compilación de la tarea **2.1**, mientras su tasks.md archivado conserva el comando histórico `pio run -d firmware`:

- [API de módulos](sentinel-firmware-module-api.md).
- [Descubrimiento](sentinel-escudo-descubrimiento.md).
- [Captura](sentinel-escudo-captura.md).
- [Detectores](sentinel-escudo-detectores.md).

La ficha explica la equivalencia de herramienta y conserva sus casos de aceptación; no reescribimos el historial para presentar otro entorno como ya probado. Las otras tareas de las fichas archivadas coinciden con su fuente.

## Pendientes que no cambian

Continúan pendientes la tarea física **4.3** del diagnóstico P1 y las comprobaciones físicas indicadas en cierres anteriores. Los encargos activos no se completan por corregir su documentación. Cada autor implementa, compila y comprueba brevemente sus funciones reales antes de solicitar cierre; fixtures, emulador y mocks se identifican como tales.

[Volver al reparto y estado por encargo](README.md).
