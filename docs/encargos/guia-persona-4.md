# Guía de Persona 4 — App, broker LAN y laboratorio

Tu entrega es la aplicación de los tres módulos, su integración Android/BLE/WSS/push, el broker accesible en LAN y la consolidación de evidencia del equipo. Los otros tres autores implementan firmware y P3 envía alertas desde servidor. Tú consumes esas entregas; no programas sus detectores, NAT o servicios Cebo.

El usuario reasignó únicamente `sentinel-app-protocolo-v2` a P1 el 2026-11-12. Tú consumes esa entrega y conservas las demás specs de app, broker y laboratorio. Adelantar broker y procedimientos de laboratorio permite integrar cada módulo conforme llega. Reutilizar navegación, tablas, gráficos y estados comunes; no rehacer el transporte para cada pantalla.

## 1. Qué leer antes de programar

1. [Guía del equipo](guia-equipo.md) y [reparto](README.md): propiedad de archivos, ramas, dependencias y evidencias.
2. [Arquitectura](../architecture.md), [decisiones R-03/R-04/R-06/R-07/R-08/R-10/R-11 y H-08](../technical-decisions.md), [roadmap](../roadmap.md).
3. [Desarrollo](../development.md), [app/README.md](../../app/README.md), [broker/README.md](../../broker/README.md) y [guía de pruebas de la base](../core-foundation-test-guide.md).
4. [Protocolo v2](../../contracts/module-protocol.md), [notificaciones](../../contracts/notifications.md) y ejemplos de los tres módulos.
5. [Ficha app-protocolo-v2](sentinel-app-protocolo-v2.md), con su diseño, spec y tareas; repetir la lectura para cada encargo.

Las specs son Markdown que puedes seguir sin instalar OpenSpec. Informa tareas numeradas y evidencias a Jefry; él actualiza las casillas oficiales. La app común de P1 admite estado/control/historial v1/v2; no completa tus pantallas específicas de los tres módulos.

## 2. Qué archivos son tuyos

- `app/`: UI, controller, store, journal, transportes, tests y proyecto Android fuente. Las pantallas nuevas se organizan en `app/src/features/<modulo>/` según diseño.
- `package.json` y `package-lock.json` raíz: dependencias del workspace `app`/`contracts`. Los otros integrantes te piden cambios aquí; evitar ediciones concurrentes del lockfile.
- `broker/`: Mosquitto, TLS/WSS, ACL y documentación de LAN.
- `simulator/`, futuros `docs/lab/` y `docs/evidence/`: reproductor de mensajes, procedimientos e informes sanitizados.

P1 mantiene contratos/núcleo/Escudo; P2 Sanitizador; P3 Cebo y `notifications/` con dependencias propias. Un fallo del componente ajeno se devuelve a su autor con reproducción; no lo corriges modificando su carpeta dentro de tu PR.

## 3. Tus seis encargos y orden recomendado

Las ramas previstas son `feat/<nombre completo del cambio>`. Sigue un encargo por rama/PR. Las dependencias siguientes son para integrar/cerrar; puedes preparar UI con fixtures revisados antes de que llegue el módulo físico.

| Prioridad de trabajo | Encargo | Dependencias principales |
| --- | --- | --- |
| Temprano | [sentinel-broker-lan](sentinel-broker-lan.md) | Contratos; desbloquea integración MQTT y servidor de alertas. |
| Preparar temprano, cerrar al final | [sentinel-laboratorio-pruebas](sentinel-laboratorio-pruebas.md) | Contratos para fixtures; evidencia real de todos para el cierre. |
| Según llegada de firmware | [sentinel-app-escudo](sentinel-app-escudo.md) | App común y descubrimiento/detectores/recuperación Escudo. |
| Según llegada de firmware | [sentinel-app-sanitizador](sentinel-app-sanitizador.md) | App común, auditoría y ruta completa DNS/DoH. |
| Según llegada de firmware/servidor | [sentinel-app-cebo](sentinel-app-cebo.md) | App común, eventos/historial Cebo y estado de alertas P3. |
| Después de Cebo y servidor push | [sentinel-app-push](sentinel-app-push.md) | App común, app Cebo y envío push de P3. |

Esta secuencia concreta el reparto, sin poner fechas ni cambiar sus dependencias. Como eres el único autor de app, termina/integrar sus cambios sucesivos antes de avanzar sobre los mismos archivos; no mantengas tres ramas divergentes editando controller/store a la vez. Broker y preparación documental de laboratorio están separados de las pantallas.

### A. Consumir app común entregada por P1 (no es un encargo tuyo)

1. Revisar la entrega de `sentinel-app-protocolo-v2`, sus pruebas y pendientes, y [app/README.md](../../app/README.md).
2. Reutilizar `send`, `recover`, `nodeStore`, journal y transportes v1/v2; desarrollar pantallas en `app/src/features/<modulo>/` y su propia spec. No volver a implementar el adaptador común.
3. Coordinar con P1 la integración antes de editar controller/store/transportes. Tus cambios posteriores quedan en tu rama y PR, sin coimplementar la spec reasignada.

### B. Broker: sentinel-broker-lan

1. Revisar preparación existente y soporte SAN LAN: hay generación local con `--san`, pero aún debes comprobar su uso real y completar ACL/rol v2 del encargo. No recrear un laboratorio con secretos ya preparados.
2. Configurar confianza CA, DNS/IP y bind de LAN propios para ESP32 y teléfono. Mantener TLS 8883/WSS 9001 y autenticación; no habilitar 1883 anónimo ni desactivar validación TLS.
3. Completar ACL exactas v1/v2 y rol `notify` de lectura mínima para P3. Conservar replay histórico v1. Probar denegación de otro nodo/rol, además de conexión permitida.
4. Documentar host/firewall y comprobar TLS/WSS desde ESP32/teléfono reales. `localhost` en el teléfono es el teléfono, no el PC del broker.
5. Entregar a P3 endpoints, CA pública y configuración de rol por canal privado para los secretos. No publicar credenciales en Git, capturas, chat o variables `VITE_*`.

### C. Laboratorio: sentinel-laboratorio-pruebas

1. Preparar topología autorizada, turnos de placa y matriz de casos R-03/R-08/R-10 en `docs/lab/`.
2. Crear en `simulator/` la reproducción de mensajes validada, separada del firmware. Todo evento de prueba v2 lleva `origin:"simulated"`.
3. Preparar casos benignos/sospechosos y plantilla PASS/FAIL/PENDIENTE con pasos, esperado, obtenido, tipo real/fixture, versión y evidencia sin secretos.
4. Probar interacciones HTTP/Telnet normales y acotadas con datos ficticios; DNS/DoH de laboratorio; BLE/MQTT y push. Para Escudo, usar Beacons reales y fixtures de lógica positivos/negativos según la spec.
5. Consolidar evidencia entregada por cada autor; devolver fallos a su dueño. No crear generadores de deauth, inyección radio o explotación: este encargo es de reproducción de mensajes y validación funcional.

No hace falta generar ataques en el aire para probar parsers/heurísticas con fixtures y demostrar que reciben observaciones físicas. Eso tampoco acredita haber detectado un ataque real: el informe debe distinguir ambas cosas. La ejecución de cada módulo sigue siendo responsabilidad de su autor; tú organizas y consolidas.

### D. UI Escudo: sentinel-app-escudo

1. Catálogo paginado de redes con SSID/BSSID/canal/RSSI/seguridad, selección y controles BLE de inicio/detención/cambio.
2. Gráficos de actividad/volumen, tabla de incidentes y detalle para los cuatro detectores, con lenguaje de indicio. Un SSID duplicado aislado no se presenta como atacante confirmado.
3. Historial, recuperación/dedup, gap y periodos sin cobertura visibles. Presentar origen real/simulado sin convertirlo en una garantía criptográfica.
4. Probar vistas con fixtures, después selección/captura reales con ESP32 por BLE; desconectar/reconectar sin duplicar incidentes. Documentar el flujo en `docs/modules/app-escudo.md`.

### E. UI Sanitizador: sentinel-app-sanitizador

1. Formularios BLE de upstream/AP/DoH, auditoría asíncrona y motivos de resultado/inconcluso; limpiar claves al terminar.
2. Controles/estado de pasarela y clientes con RSSI/tráfico de ambas direcciones, métricas y gráficos. Diferenciar desconocido, degradado y funcionando.
3. Reglas DNS paginadas, bloqueo/excepción, métricas y cobertura explicada sin prometer filtrado universal.
4. Probar con P2 ESP32 y cliente AP reales: auditar, configurar, navegar, bloquear, fallar/restaurar DoH y detener. Documentar `docs/modules/app-sanitizador.md`.

### F. UI Cebo: sentinel-app-cebo

1. Controles confirmados, estado de HTTP/Telnet y métricas/gráficos por MQTT/WSS.
2. Tabla, filtros, historial y alertas internas; dedup, gap y ausencia de datos sensibles.
3. Consumir API HTTPS de estado de notificaciones de P3; mostrar fallos y aceptación por proveedor sin inventar confirmación de lectura.
4. Con Cebo real, observar interacciones HTTP/Telnet; cortar/restaurar MQTT y un canal externo, comprobar recuperación y documentar `docs/modules/app-cebo.md`.

### G. Android push: sentinel-app-push

1. Revisar con P3/Jefry proveedor/plugin compatibles y fijar dependencia; configurar Firebase Android localmente según diseño. P3 conserva claves Admin del servidor fuera de la app.
2. Implementar permisos, registro/renovación/baja HTTPS con identidad autorizada, cambio de token y caso permiso denegado.
3. Recibir notificación y abrir Cebo por `event_id`; consultar historial si hay canal y mostrar pendiente/fuera de retención cuando corresponda.
4. Compilar/sincronizar e instalar en teléfono real. Cerrar desde recientes, generar incidente Cebo real con P3 y comprobar recepción/tap. Documentar las limitaciones de force-stop por separado; no prometer recepción bajo cualquier estado del sistema.
5. Registrar dispositivo/permiso/hora/ID y la evidencia de recepción real, sin tokens ni configuración privada.

MQTT y una alerta interna con la app abierta no sustituyen push. Una respuesta FCM aceptada del servidor tampoco demuestra por sí sola recepción en Android.

## 4. Cómo usar los contratos desde la app

El paquete `@sentinel/contracts` ya define tipos y validación. Consultar [protocolo y negociación](../../contracts/module-protocol.md); no crear un formato distinto para cada pantalla.

| Necesidad | Referencia |
| --- | --- |
| Negociar v1/v2 y leer eventos de ambas versiones | `decodeAnyMetadata`, `decodeAnyMessage`; discriminar `version`. |
| Crear comando funcional y validar su ACK | `makeModuleCommand`, `checkModuleAckForCommand` y resultados tipados; para base v1 conservar sus APIs. |
| BLE | [ble.ts](../../contracts/src/ble.ts), UUID/framing compartidos; no inventar otro GATT. |
| MQTT | `mqttTopic`, `MQTT_POLICY` y tabla de topics/replay del protocolo. |
| Historial | `eventIdentity`, `read_events`, páginas y gap; comprobar recepción de eventos antes de avanzar el cursor durable. |
| Datos de módulos | [states](../../contracts/fixtures/v2/states/), [metrics](../../contracts/fixtures/v2/metrics/), [events](../../contracts/fixtures/v2/events/), [commands](../../contracts/fixtures/v2/commands/), [acks](../../contracts/fixtures/v2/acks/). |
| Notificaciones | [Contrato HTTPS/push](../../contracts/notifications.md) y [fixtures HTTP/JSON](../../contracts/fixtures/v2/notifications/). |

Los schemas rechazan propiedades desconocidas; no resolverlo quitando validaciones. Respetar 2048 bytes UTF-8 por mensaje ESP y paginación. Los gráficos calculan deltas de contadores dentro del mismo boot, sin reiniciar contadores al reconectar. Consultar unidades, estados y códigos exactos; mostrar datos desconocidos de forma honesta.

La disponibilidad de una operación depende del firmware/canal real. En Escudo solo BLE, sin asociación/MQTT directo; Sanitizador usa BLE para control; Cebo usa los canales definidos. El cambio de modo puede cerrar el canal desde el que lo pediste: reconciliar estado y no confundir desconexión con éxito o fallo definitivo.

El registro push usa HTTPS y credencial propia de esa API, no la contraseña MQTT. Validar el nodo autorizado; no persistir tokens ni introducir secretos en bundle web. Consultar con P3 los endpoints reales cuando los implemente: ahora el paquete de contratos no levanta ningún servidor.

## 5. Preparar y ejecutar

Usar Node 24 y npm del repositorio; JDK21/SDK36 para Android, Docker/Compose/Python/OpenSSL para broker, según [desarrollo](../development.md). No modificar versiones fijadas solo para hacer desaparecer un error local.

Desde raíz, después de obtener la referencia revisada y crear tu rama en un clon limpio:

```sh
npm ci
npm run check:contracts
npm run test:app
npm run build
npm run dev
```

Luego, para Android:

```sh
npm run android:sync
npm run android:assemble
```

El script de assemble usa el wrapper de shell del proyecto: seguir las instrucciones de [app](../../app/README.md) para el entorno elegido; en Windows nativo está `app/android/gradlew.bat`. Las rutas locales de SDK se configuran en el entorno o `local.properties` ignorado, no en archivos públicos.

Para broker, leer primero sus instrucciones de preparación. En un laboratorio ya preparado:

```sh
python3 broker/scripts/manage.py config
python3 broker/scripts/manage.py up
python3 broker/scripts/manage.py ps
```

Estos comandos existen; no prueban por sí solos ACL v2, acceso LAN o push. Seguir las verificaciones de la ficha y del README. Un emulador sirve para UI, arranque y casos de permisos; no sustituye la evidencia BLE físico o push con el teléfono real requerida por las specs. Para periféricos y pairing de la ESP32, coordinar con P1 y seguir la [guía de base](../core-foundation-test-guide.md).

## 6. Primera sesión y cierre

1. Confirmar con Jefry la referencia publicada de contratos y revisar estado real de las dependencias.
2. Consumir la referencia revisada de app común de P1 y ejecutar tests/build antes de ampliar la app en tu propia spec.
3. Comenzar `feat/sentinel-broker-lan` para completar seguridad/ACL v2 y desbloquear P3; preparar cada pantalla en su propia rama/PR.
4. Preparar procedimientos/plantillas de laboratorio; integrar las pantallas conforme lleguen los módulos y reservar una sesión de teléfono/ESP32 para cada flujo real.
5. Coordinar push con P3 desde temprano para que cuentas/configuración no queden para el último día.

| Compañero | Lo que intercambian |
| --- | --- |
| P1 | Contratos/capacidades Escudo y núcleo ↔ comandos/UI, revisión de interfaces e informes de integración. |
| P2 | Resultados y métricas reales Sanitizador ↔ pantalla BLE y pruebas conjuntas con cliente AP. |
| P3 | Eventos y API/servidor de alertas ↔ broker/rol, UI Cebo y registro/recepción Android. |

Tu cierre exige tres pantallas integradas con hardware, broker con seguridad real, push recibido con app cerrada y matriz de evidencia completa. Los fixtures permiten preparar interfaces; no reemplazan esos cierres. Mantener pruebas focalizadas de negociación, ACK, dedup/paginación, privacidad y registro push. Los benchmarks/estabilidad prolongada quedan después.

En cada PR usar el [formato de entrega común](guia-equipo.md#6-rutina-por-tarea-y-entrega). Jefry mantiene OpenSpec; tú consolidas evidencia, sin dar por implementado lo que un autor no demostró.

[Volver al reparto](README.md) · [Guía del equipo](guia-equipo.md)
