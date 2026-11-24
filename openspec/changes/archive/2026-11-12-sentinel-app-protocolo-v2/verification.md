# Verificación de sentinel-app-protocolo-v2

Fecha: 2026-11-12. Autor único: Persona 1 — Jefry, por reasignación expresa. Rama `feat/sentinel-app-protocolo-v2`; publicación mediante PR hacia `develop`, autorizada por el usuario después de obtener esta evidencia. La publicación no acredita merge ni completa las comprobaciones físicas.

## Implementación

- BLE conserva seguridad, UUID y framing; usa Metadata y codec compartidos v1/v2. MQTT suscribe ambas versiones, valida topic/nodo/versión y publica la versión negociada por WSS, con roles y operaciones privadas restringidas a BLE.
- Controller/store correlacionan versión, nodo, boot, revisión, operación y resultado ACK. Enviar o recibir accepted no equivale a aplicar. Un retained no negocia, confirma ni hace fresco el estado. Un cambio de modo rechazado consulta estado; una interrupción deja el resultado pendiente y consulta por otro canal conectado; no reenvía configuraciones automáticamente. Cambiar de nodo durante lectura de Metadata cancela la intención anterior.
- Journal conserva la misma base IndexedDB, 256 eventos/nodo e identidad original v1/v2. Registra familias Escudo/Sanitizador/Cebo; solo completa páginas correlacionadas tras persistir eventos, con dedup/cursor/gap. Un resultado ACK que no sea página no se interpreta como página.
- Navegación común por capacidades, estado confirmado y métricas del mismo arranque. Historial distingue origen real/simulado de v2; v1 no inventa origen. Un origen declarado no es autenticación criptográfica del evento.
- Formularios limpian claves al enviar y se restablecen al terminar, incluso ante error. Correlación de órdenes privadas sin argumentos; IndexedDB admite solo eventos validados. MQTT elimina credenciales en desconexión, cierre y fallo de conexión; sin persistencia de claves ni logs de payloads.

## Comprobaciones del incremento antes del archivo

Entorno: WSL/Linux, Node24.13.1/npm11.8.0, JDK21, SDK Android36 y Build Tools36, versiones existentes del repositorio. Sin dependencias nuevas.

| Comprobación | Resultado / alcance |
| --- | --- |
| `npm run check:contracts` | PASS, 154 tests; biblioteca/fixtures compartidos intactos. |
| `npm run test:app` | PASS, 26 tests focalizados de store, controller, journal y adaptador MQTT. Los dobles de transporte son solo límites de prueba; no acreditan BLE/MQTT en placa. |
| `npm run build` (ejecutado por `npm run android:sync`) | PASS, TypeScript y Vite. Se conserva el aviso de bundle mayor de 500 kB; no se añadió optimización prematura. |
| Vite + `npm run check:browser -w app` | PASS, Chromium: arranque, diagnóstico BLE web, navegación/capacidades v2 y rechazo sin éxito falso con fixtures; IndexedDB real tras recarga, dedup/gap y vista móvil sin desbordamiento. |
| `npm run android:sync` | PASS; assets y plugin BLE sincronizados con el código final. Sin cambios en fuentes nativas mantenidas. |
| `npm run android:assemble` | PASS, `BUILD SUCCESSFUL`, 124 tareas (27 ejecutadas y 97 actualizadas), APK debug generado. Compilar no acredita permisos, pairing ni radio real. |
| `openspec validate --all --strict --json` | PASS, 30/30: 13 specs principales y 17 cambios activos. |

Los fixtures solo se inyectan desde tests/checks; la app de producto espera mensajes reales. Progreso al entregar el software: **5/6 tareas**, pendiente 2.1. Posteriormente el usuario autorizó sincronizar y archivar manteniendo esa prueba pendiente; ver [cierre](closure.md). El smoke de navegador también comprueba limpieza del formulario y ausencia de clave de prueba en store/localStorage tras error local de cuenta.

## Pendientes físicos y dependencias

La tarea **2.1 permanece abierta** por faltar control con ESP32/teléfono; tests, build y arranque web sí pasaron. La tarea **2.2 se cumple por la alternativa explícita de compilar y dejar evidencia física pendiente** que contempla su texto. Permisos, pairing y conexión BLE real siguen sin comprobar; el build y los fixtures no los sustituyen. No se ha realizado una nueva sesión de emulador, BLE físico ni broker real v2 en este incremento.

Detección del entorno el 2026-11-13 a las 04:36 UTC (2026-11-12 en Guatemala): `adb.exe devices -l`, desde el SDK Windows del usuario, terminó correctamente sin dispositivos. WSL no tenía `/dev/ttyUSB*` ni `/dev/ttyACM*`; `Get-CimInstance Win32_SerialPort` no devolvió puertos y `Get-PnpDevice -PresentOnly` no encontró dispositivos con VID `303A`, `10C4` o `1A86`. Esto acredita que las herramientas no detectaron los equipos en ese momento, no prueba un fallo del hardware. Para continuar 2.1 hace falta conectar y autorizar teléfono/ESP32; se solicitó al usuario. No se instaló ni grabó nada en un dispositivo.

Contraste de integración en fuentes: `ModuleSupervisor::capabilities()` agrega capacidades de todos los módulos registrados, incluso desde Reposo, y el snapshot Escudo anuncia `capture`/`detectors`. La navegación común puede habilitar Escudo sin exigir que ya esté activo. Esta inspección no sustituye el recorrido físico.

Recorrido breve cuando haya hardware:

1. Seguir el [montaje y configuración de base](../../../../docs/core-foundation-test-guide.md), con OLED y botón; compilar/grabar el firmware revisado por el flujo de Persona 1.
2. Instalar el APK de este incremento en el teléfono autorizado, conceder permisos cercanos, abrir la ventana con el botón y emparejar con código OLED. Comprobar rechazo de operaciones protegidas sin autenticación.
3. Leer versión/boot/capacidades reales, consultar estado y cambiar Reposo/Escudo; comprobar ACK y estado/revisión, incluido rechazo sin éxito aparente. La app común no añade catálogo ni controles de captura: siguen en `sentinel-app-escudo`.
4. Desconectar/reconectar BLE y reiniciar app; verificar pendientes y recuperación de eventos disponibles por cursor, mismos IDs, dedup y gap visible. Registrar versión, paso y resultado sin secretos.
5. Para MQTT v2, esperar ACL v2 de `sentinel-broker-lan` y Cebo real de P3; comprobar WSS/CA/SAN, operador/observador y retención con esos componentes integrados.

No se ejecutó `check:wss`: no había listener WSS local disponible en el puerto 9001 al comprobar el entorno.

La plantilla actual de broker solo concede v1. La app tolera ACL v1 para la base; suscribir topics v2 no demuestra permisos v2. Completar broker corresponde a P4. Pantallas/controles específicos, gráficos, reglas y notificaciones conservan sus specs y autores. No se declara terminado el MVP ni el Escudo físico por entregar este adaptador común.

## Recuperación archivada

Antes de este apply se sincronizaron los tres requisitos de [escudo-recovery](../../../specs/escudo-recovery/spec.md) y se [archivó recuperación](../2026-11-12-sentinel-escudo-recuperacion/closure.md), conforme a autorización expresa. Sus tareas físicas 1.3, 2.1 y 2.2 continúan pendientes. PR #10 y su integración se gestionan por separado; este trabajo no realizó merge.
