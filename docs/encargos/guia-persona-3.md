# Guía de Persona 3 — Cebo y servidor de alertas

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P3 persistirá configuración aceptada de Cebo e historial sanitizado mediante recursos centrales, con límites, commit antes de confirmar y recuperación tras reinicio/corte sin USB; no iniciar servicios automáticamente. Mantener eventos/IDs/cursores y dedup dentro del ring. No guardar credenciales recibidas por HTTP/Telnet/honeypot o terceros. NVS estándar no promete cifrado físico; secretos propios solo por canal autorizado, nunca Git/log/event/status. Webhook y push mantienen sus encargos completos.

Entregas el Módulo 3 completo y el componente que envía webhook y push. Persona 4 hace la UI y recibe el push en Android; tú implementas el envío en servidor. El Cebo es un servicio ficticio seguro en una LAN autorizada, sin administración real ni ejecución de comandos.

## 1. Qué leer antes de programar

1. [Guía del equipo](guia-equipo.md) y [reparto](README.md): carpetas, dependencias, ramas, contratos y evidencias.
2. [Arquitectura](../architecture.md), especialmente Cebo/persistencia/notificaciones; [R-04, R-06, R-10, R-11 y H-06/H-07/H-08](../technical-decisions.md).
3. Tu [ficha HTTP](sentinel-cebo-http.md) y su diseño/spec/tareas; repetir esta lectura para cada encargo posterior.
4. [Protocolo v2](../../contracts/module-protocol.md), [contrato de notificaciones](../../contracts/notifications.md) y [diseño de la API de módulos](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/design.md).
5. [Firmware](../firmware.md), [broker](../../broker/README.md) y [guía física](../core-foundation-test-guide.md).

No tienes que aprender OpenSpec ni generar cambios: las propuestas están preparadas. Sigue las tareas numeradas de las fichas y entrega evidencia a Jefry para el seguimiento oficial.

## 2. Tus zonas y límites

- Firmware: futuro `firmware/SentinelNode/src/cebo/`, `firmware/test/cebo/` y documentación `docs/modules/cebo-*.md` de tus fichas.
- Alertas: futuro `notifications/`, sus tests, package/lockfile y Compose propios, más sus documentos. No añadir sus dependencias al workspace raíz sin coordinar con P4.
- P1 mantiene supervisor, radio, transportes, serialización y journal. Usar sus hooks y almacenamiento compartido; no crear otra instancia MQTT, otro gestor Wi-Fi o un journal firmware alternativo.
- P4 mantiene `app/`, Android, broker/ACL y lockfiles del workspace. Entregarle necesidades de rol, endpoints y ejemplos; no editar sus archivos para completar tu entrega.

Para integrar HTTP necesitas la API de P1; para alertas completas necesitas broker LAN de P4 y eventos reales del Cebo. Puedes preparar lógica/servicio con fixtures revisados mientras llegan, distinguiendo preparación de integración real.

## 3. Tus cinco encargos en orden

Una rama `feat/<nombre completo del cambio>` y un PR por encargo. No abrir dos implementaciones simultáneas sobre el mismo archivo de `cebo/` o `notifications/`.

### A. sentinel-cebo-http

[Ficha, diseño, spec y tareas](sentinel-cebo-http.md).

1. Crear módulo/hooks y bus interno acotado de interacciones; compilar sin modificar núcleo ni drivers.
2. Implementar panel administrativo ficticio, rutas y formulario con `esp_http_server`. Registrar conexión, acceso e intento de autenticación con metadatos permitidos.
3. Limitar sesiones, tamaño y tiempos; descartar cuerpos/credenciales recibidos sin volcarlos a logs o eventos. Probar con un marcador de contraseña ficticio que no aparezca en salidas.
4. Documentar rutas/límites y comprobar desde teléfono/PC en LAN propia.
5. Al detener Cebo o cambiar modo, verificar puerto HTTP cerrado y recursos liberados. El panel nunca permite administrar el ESP32 o ejecutar comandos.

### B. sentinel-cebo-telnet

[Ficha, diseño, spec y tareas](sentinel-cebo-telnet.md). Depende del HTTP anterior.

1. Añadir listener puerto 23 y sesiones con estados, entradas fragmentadas, tratamiento IAC acotado y límite de línea.
2. Simular la interacción/autenticación sin almacenar credenciales ni ejecutar entradas; conectar el mismo bus de interacciones.
3. Unificar límites/parada de HTTP+Telnet y documentar respuestas esperadas.
4. Comprobar ambos servicios reales mientras Cebo está activo. Al salir, puertos 80/23 cerrados y ninguna sesión/tarea residual.

### C. sentinel-cebo-eventos-historial

[Ficha, diseño, spec y tareas](sentinel-cebo-eventos-historial.md). Depende de Telnet.

1. Clasificar ambos servicios mediante ventanas y contadores acotados: aislado, repetido, expiración y nivel de sospecha. Mantener tests con reloj controlado.
2. Publicar eventos/métricas v2 y `cebo.stats`: IP de origen, puertos, protocolo, tiempo/tipo de interacción y cantidades permitidas. Respetar la relación sospecha/severidad del contrato.
3. Integrar journal circular central, persistencia y recuperación MQTT por cursor/IDs. Conservar contadores aunque una alerta repetida sea suprimida.
4. Documentar umbrales/retención; comprobar HTTP/Telnet reales y recepción MQTT con identidad autorizada.
5. Cortar/restaurar broker y reiniciar brevemente; recuperar eventos con mismo ID y huecos visibles si se sobreescribió el anillo, sin duplicar incidentes.

### D. sentinel-alertas-webhook

[Ficha, diseño, spec y tareas](sentinel-alertas-webhook.md). Requiere contratos, broker LAN y eventos Cebo.

1. Crear componente Node mínimo con package/lockfile y Compose propios. Consumir MQTT TLS con rol de lectura limitado a nodos autorizados; pedir ACL a P4.
2. Implementar enrutado, dedup y outbox persistente acotada con reintentos/vencimiento. No convertirlo en una plataforma histórica adicional.
3. Integrar webhook HTTPS con destinos/cuentas propios configurados fuera de Git. Mantener la gestión de fallos del webhook separada del futuro canal push.
4. Implementar estado por HTTPS con autenticación por nodo y CORS explícito conforme al contrato; no devolver tokens/destinos privados.
5. Documentar configuración/arranque/retención en `notifications/README.md` y documento del módulo. Recibir una alerta de un incidente real en el webhook configurado, repetirlo y comprobar supresión y recuperación limitada.

### E. sentinel-alertas-push-servidor

[Ficha, diseño, spec y tareas](sentinel-alertas-push-servidor.md). Extiende el componente anterior.

1. Revisar la elección FCM propuesta con Jefry/P4; añadir SDK oficial compatible en tu paquete. Aprovisionar credenciales propias fuera de Git; no entregar claves Admin a la app.
2. Implementar registro/renovación/baja de instalaciones autorizadas, capacidad y TTL del contrato. Probar cruce de nodo, reemplazo, expiración y token inválido.
3. Enviar `notification+data` con identidad original y contenido mínimo; mantener privacidad, dedup y errores sanitizados.
4. Documentar HTTPS/proveedor/operación y probar envío real a la instalación Android de P4.
5. Registrar aceptación del proveedor; coordinar con P4 la evidencia separada de recepción con app cerrada y apertura del incidente.

**Los cinco encargos son obligatorios.** El Cebo no queda completo con HTTP/Telnet sin persistencia, historial o canales externos. Dividirlos en PR organiza el trabajo, no elimina funciones.

## 4. Contratos que debes consumir

| Área | Referencias concretas |
| --- | --- |
| Control | [cebo.start](../../contracts/fixtures/v2/commands/cebo.start.valid.json), [stop](../../contracts/fixtures/v2/commands/cebo.stop.valid.json), [configure](../../contracts/fixtures/v2/commands/cebo.configure.valid.json), [stats](../../contracts/fixtures/v2/commands/cebo.stats.valid.json). |
| Interacciones | [HTTP benigno](../../contracts/fixtures/v2/events/cebo.http-benign.valid.json), [HTTP repetido](../../contracts/fixtures/v2/events/cebo.http-repeated-auth.valid.json), [Telnet repetido](../../contracts/fixtures/v2/events/cebo.telnet-repeated-auth.valid.json). |
| Estado/métricas | [Estado Cebo](../../contracts/fixtures/v2/states/cebo.valid.json), [métricas](../../contracts/fixtures/v2/metrics/cebo.valid.json), [ACK stats](../../contracts/fixtures/v2/acks/cebo.stats.valid.json). |
| HTTP/push | [Contrato de notificaciones](../../contracts/notifications.md), [ejemplos JSON/HTTP](../../contracts/fixtures/v2/notifications/), [helpers](../../contracts/src/notifications.ts). |

Los ejemplos son ficticios con origen simulado. Eventos de interacciones físicas usan `origin:"real"`; no guardes el ejemplo como si hubiera ocurrido. El componente de alertas conserva `(device_id,message_id)` del incidente y el push lleva ese `message_id` como `event_id`.

En Node, validar con los helpers compartidos y resolver cómo consumir la biblioteca conforme al diseño del paquete independiente, sin copiar schemas o tipos divergentes. Los helpers de notificaciones no autentican bearer ni verifican credenciales MQTT: tú implementas esos adaptadores y obtienes el nodo autorizado de la sesión, no del cuerpo.

Implementar exactamente rutas/cuerpos/códigos de `notifications.md`, incluidos límites, aislamiento por nodo y baja idempotente. `provider_accepted` acredita aceptación del endpoint/proveedor; `read_status` permanece `unknown`. No anunciar que una persona recibió o leyó algo basándote solo en la respuesta del proveedor.

No registrar contraseñas, usuarios capturados, tokens, cookies o cuerpos completos. El detector usa solo metadatos permitidos y el codebook de rutas/evidencias; tampoco colocar el secreto en un campo de texto permitido. Si necesitas ampliar el contrato, pedir el cambio a P1 y avisar a P4 antes del consumidor.

## 5. Primera sesión y pruebas breves

1. Confirmar con P1 contrato/API y referencia revisada de partida; leer el diseño HTTP.
2. Preparar [Arduino IDE Windows y la integración oficial (core3.3.12, dos bibliotecas y opción local documentados)](../firmware.md) y abrir la rama `feat/sentinel-cebo-http` en tu clon limpio según la [guía común](guia-equipo.md#5-preparar-tu-copia-y-tu-rama).
3. Entregar primero módulo integrable/build, después panel real y bus de interacciones. Hacer pruebas breves con datos ficticios en red propia.
4. Avisar temprano a P4 del rol `notify`, endpoint de estado y necesidad de una instalación Android para probar push. No esperar al final para aprovisionar destinos propios.

Comandos existentes, desde raíz:

```sh
node firmware/test/arduino_build_test.mjs
npm run check:contracts
```

Compilar además el sketch con **Verificar** y configuración oficial verificada cuando se entregue. Agregar fuentes a `SentinelNode/src/cebo/`, exportar `sentinel_cebo_module()` y habilitar `SENTINEL_MODULE_CEBO` solo al disponer de implementación real. Para firmware, añadir tests propios de parsers, límites, sanitización, clasificación y recuperación según fichas. Para `notifications/`, definir y documentar sus scripts de build/test/arranque en sus encargos: **todavía no existe un servidor ejecutable ni un comando de arranque que esta guía pueda dar por implementado**. Seguir [broker/README.md](../../broker/README.md) para el broker y no regenerar secretos de otro laboratorio.

No exponer HTTP/Telnet a Internet ni abrir puertos del router. Limitar recursos y mantener el uso en redes propias/autorizadas. Al probar privacidad, usar marcadores ficticios y conservar evidencia sanitizada.

## 6. Coordinación y cierre

| Con quién | Entrega o solicitud |
| --- | --- |
| P1 | Recibir hooks/journal; solicitar cambios globales. Entregar registro Cebo y evidencia de parada, replay y límites. |
| P2 | Acordar turnos de ESP32; no modificar Sanitizador ni ejecutar ambos modos a la vez en una placa. |
| P4 | Recibir broker TLS/WSS y rol mínimo; entregar API de estado/registro y servidor push. Él implementa la UI, registro Android y recepción/tap. |

Tu cierre requiere servicios reales, eventos válidos, persistencia/replay sin secretos, webhook recibidos y envío push real con evidencia coordinada de recepción Android. Entregar por número de tarea, comandos y resultados; P4 consolida y Jefry mantiene OpenSpec. Usa el [formato de entrega](guia-equipo.md#6-rutina-por-tarea-y-entrega) y conserva pendientes físicos como PENDIENTE.

[Volver al reparto](README.md) · [Guía del equipo](guia-equipo.md)
