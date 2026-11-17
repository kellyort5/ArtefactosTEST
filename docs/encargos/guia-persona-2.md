# Guía de Persona 2 — Auditor y Sanitizador

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.

Tu responsabilidad es el firmware completo del Módulo 2: auditar una red y ofrecer una pasarela IPv4 con DNS filtrado y DoH. Empiezas en paralelo con Escudo y Cebo; el módulo ya forma parte de la entrega conjunta. Persona 4 hace su pantalla y Jefry integra tu componente en el núcleo.

## 1. Qué leer antes de programar

1. [Guía del equipo](guia-equipo.md): entorno de colaboración, contratos, ramas y formato de entrega.
2. [Arquitectura](../architecture.md), especialmente Sanitizador, recursos y transportes; [R-08, R-11 y H-04/H-05](../technical-decisions.md); [línea paralela del roadmap](../roadmap.md#línea-paralela--auditor-y-sanitizador).
3. [Ficha de auditoría](sentinel-sanitizador-auditoria.md), su diseño, spec y tareas enlazadas. Hacer lo mismo con cada ficha antes de abordar el siguiente incremento.
4. [Protocolo funcional](../../contracts/module-protocol.md), sus comandos `sanitizador.*`, estados/métricas y ejemplos de eventos. Leer también el [diseño de API del firmware](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/design.md).
5. [Preparación del firmware](../firmware.md) y [prueba física de la base](../core-foundation-test-guide.md).

No necesitas instalar OpenSpec ni generar propuestas. Lee los Markdown como requisitos de tu trabajo e informa resultados por los números de tarea de cada ficha; Jefry mantiene el seguimiento oficial.

## 2. Tu carpeta y dependencias

Creas `firmware/SentinelNode/src/sanitizador/`, sus tests en `firmware/test/sanitizador/` y los documentos `docs/modules/sanitizador-*.md` indicados en cada ficha. Usas el mismo sketch Arduino y la integración oficial que P1/P3 (dependencias todavía en curso); no crear un firmware separado.

Necesitas el contrato revisado y `sentinel-firmware-module-api` de P1 para integrar. La API real ya está fusionada en develop mediante PR #6: usa [module_api.hpp](../../firmware/SentinelNode/src/sentinel_api/include/module_api.hpp) y la [guía de integración](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3). Se archivó con 6/7 tareas y la prueba física 2.2 pendiente; no confundir compilación con validación en la placa.

Solicitas a P1 cambios en el sketch, `SentinelConfig.h`, configuración de la integración Arduino, radio, supervisor, serializador o journal. Solicitas a P4 cualquier pantalla o cambio del workspace npm. No inicialices/inicies/detengas Wi-Fi por tu cuenta: usa el gestor del núcleo y las netifs prestadas conforme a la API entregada.

## 3. Tus cuatro encargos, uno por rama y PR

Las ramas son `feat/<nombre completo del cambio>`. Dentro de cada encargo, seguir los números y verificaciones de su ficha.

### A. sentinel-sanitizador-auditoria

[Ficha, diseño, spec y tareas](sentinel-sanitizador-auditoria.md). Depende de la API del núcleo.

1. Crear el módulo con sus hooks y solicitud STA al gestor; conseguir un build integrable.
2. Recibir configuración upstream por BLE protegido, persistir configuración propia validada en NVS oficial según la política, manteniendo secretos fuera de mensajes/logs y permitir cancelación. Rechazar el canal MQTT para estas operaciones.
3. Ejecutar trabajos de DNS, HTTPS/certificados y portal cautivo con timeout. Distinguir resultados válidos, fallos e inconclusos con sus motivos.
4. Publicar resultado/evento v2 correlacionado y documentar destinos/criterios en `docs/modules/sanitizador-audit.md`.
5. Comprobar auditoría real con endpoint HTTPS válido e inválido controlados; salir de modo durante una auditoría y verificar cancelación/liberación sin arrancar AP.

**Entrega a P4:** ejemplos validados de resultado, errores e inconclusos; cómo iniciar trabajo y consultar resultado. Entrega a P1 cualquier requisito faltante del hook STA.

### B. sentinel-sanitizador-pasarela

[Ficha, diseño, spec y tareas](sentinel-sanitizador-pasarela.md). Continúa desde auditoría revisada.

1. Pedir al núcleo los flags forwarding/NAPT y acceso a netifs; verificar que el build dispone de ellos. P1 mantiene la configuración global.
2. Configurar AP autenticado, DHCP y subred separada; detectar solapamiento con upstream y rechazar configuraciones inválidas.
3. Integrar NAT IPv4, desconexión/reconexión y parada. No añadir port forwarding ni anunciar protección IPv6.
4. Exponer clientes, RSSI y subida/bajada con contadores y paginación reales. Documentar significado y límites en `docs/modules/sanitizador-gateway.md`.
5. Conectar teléfono o portátil al AP físico, obtener IP y comprobar salida IPv4 y tráfico en ambas direcciones. Cortar upstream y cambiar modo: degradación y cierre deben ser visibles.

**Entrega a P4:** estados y métricas reales de pasarela/clientes. Antes de DNS/DoH, este incremento acredita conectividad, no la protección final completa.

### C. sentinel-sanitizador-dns

[Ficha, diseño, spec y tareas](sentinel-sanitizador-dns.md). Depende de la pasarela.

1. Implementar servidor/parser DNS UDP y TCP acotado; probar truncamiento, compresión malformada y límites con tests breves.
2. Implementar alta, baja y listado de reglas con normalización, frontera de subdominio y excepciones allow.
3. Añadir caché TTL e interfaz del resolvedor asíncrono. Mientras DoH no exista o falle, responder conforme a la spec, sin fallback a DNS plano.
4. Publicar eventos/métricas y documentar cobertura en `docs/modules/sanitizador-dns.md`.
5. Consultar desde cliente real del AP, comprobar bloqueo/NXDOMAIN, SERVFAIL y reglas por BLE. Verificar que no atiende clientes externos al AP y que al salir libera sockets.

**Entrega a P4:** operaciones de reglas y contadores. Explicar que el filtrado cubre el DNS atendido por este servidor; no prometer bloquear todo DNS alternativo o todo tráfico cifrado del cliente.

### D. sentinel-sanitizador-doh

[Ficha, diseño, spec y tareas](sentinel-sanitizador-doh.md). Completa la ruta DNS anterior.

1. Implementar configuración HTTPS/CA y cliente POST DoH acotado; validar contenido, tamaño e identidad de respuesta DNS.
2. Validar CA, hostname y fecha TLS; ante certificado inválido o timeout, fallar sin reenviar DNS en claro.
3. Integrar caché, reintentos limitados, métricas y estado de protección. Documentar bootstrap y límites en `docs/modules/sanitizador-doh.md`.
4. Con cliente AP real, comprobar NAT+DoH juntos y consultas permitidas/bloqueadas. Cortar/restaurar upstream/DoH: SERVFAIL, degradación, control BLE y recuperación deben funcionar.
5. Entregar evidencia conjunta de auditoría, pasarela, DNS y DoH para la integración final de P1.

**Terminas el módulo cuando los cuatro incrementos funcionan juntos**, no al conseguir solamente NAT o una respuesta DoH aislada. Resolver fallos reales durante estos incrementos; no abrir una investigación extensa de rendimiento como prerrequisito.

## 4. Cómo consumir el contrato de tu módulo

Abrir estos ejemplos y consultar el protocolo para sus límites completos:

| Necesidad | Referencias |
| --- | --- |
| Configurar red/AP/DoH | [configure_upstream](../../contracts/fixtures/v2/commands/sanitizador.configure_upstream.valid.json), [configure_ap](../../contracts/fixtures/v2/commands/sanitizador.configure_ap.valid.json), [configure_doh](../../contracts/fixtures/v2/commands/sanitizador.configure_doh.valid.json). |
| Auditoría asíncrona | [audit](../../contracts/fixtures/v2/commands/sanitizador.audit.valid.json), [audit_result](../../contracts/fixtures/v2/commands/sanitizador.audit_result.valid.json), [resultado ACK](../../contracts/fixtures/v2/acks/sanitizador.audit.valid.json). |
| Pasarela/clientes | [gateway_start](../../contracts/fixtures/v2/commands/sanitizador.gateway_start.valid.json), [gateway_stop](../../contracts/fixtures/v2/commands/sanitizador.gateway_stop.valid.json), [clients](../../contracts/fixtures/v2/commands/sanitizador.clients.valid.json). |
| Reglas | [rules.put](../../contracts/fixtures/v2/commands/sanitizador.rules.put.valid.json), [rules.remove](../../contracts/fixtures/v2/commands/sanitizador.rules.remove.valid.json), [rules.list](../../contracts/fixtures/v2/commands/sanitizador.rules.list.valid.json). |
| Salida del módulo | [Estado](../../contracts/fixtures/v2/states/sanitizador.valid.json), [métricas](../../contracts/fixtures/v2/metrics/sanitizador.valid.json), [evento DNS](../../contracts/fixtures/v2/events/sanitizador.dns.valid.json). |

Los ejemplos son simulados. Tu productor físico debe emitir eventos con `origin:"real"`. No copiar los IDs ficticios como IDs operativos ni crear sobres/journals independientes: usar el núcleo. En auditoría respetar `job_id` y correlación del contrato; un ACK de aceptación no es el resultado del diagnóstico.

No cambies nombres/enums/unidades para ajustarlos a una estructura C++ propia. Los mensajes completos tienen límite de bytes y páginas; no devolver una tabla ilimitada. Si falta una propiedad, pedir revisión a P1 con caso concreto y avisar a P4 antes de implementarla.

## 5. Primera sesión y comprobaciones

1. Obtener de Jefry el PR/referencia de contrato y API que debes consumir.
2. Preparar Arduino IDE Windows y la integración oficial (core3.3.12, dos bibliotecas y opción local documentados) siguiendo [firmware](../firmware.md); leer `SentinelConfig.h` y conservar las versiones fijadas. Agregar fuentes a `SentinelNode/src/sanitizador/`, exportar `sentinel_sanitizador_module()` y habilitar su flag solo al disponer de implementación real.
3. En tu clon limpio, crear `feat/sentinel-sanitizador-auditoria` desde la base acordada según la [guía común](guia-equipo.md#5-preparar-tu-copia-y-tu-rama).
4. Hacer funcionar el módulo mínimo, compilar y entregar la primera comprobación antes de añadir NAT/DNS/DoH.

Comandos desde raíz, cuando sus herramientas/dependencias estén preparadas:

```sh
node firmware/test/arduino_build_test.mjs
npm run check:contracts
```

Compilar además el sketch con **Verificar** y configuración oficial verificada cuando se entregue según [firmware](../firmware.md). El segundo comando valida la biblioteca compartida, no sustituye tests de tu C++ ni pruebas de hardware. Añadir tests focalizados de clasificación de auditoría, subredes, parser/reglas/caché DNS y fallos DoH en tu carpeta. Usa la [guía física](../core-foundation-test-guide.md) para upload/monitor y acuerda el turno de placa con P1/P3/P4.

Al informar una prueba, registrar tarea, versión, cliente/AP autorizados, pasos, esperado, resultado e IDs sin claves. P4 consolida tu informe; tú corriges tus fallos. La falta temporal de placa se registra PENDIENTE, sin marcar comprobación física como PASS.

## 6. Qué debes pedir y entregar al resto

| Compañero | Coordinación |
| --- | --- |
| P1 | Recibes contrato/API y soporte de núcleo. Entregas registro del módulo, requisitos globales y evidencia de parada/recuperación. |
| P3 | Compartes turnos de ESP32 y restricciones del gestor; cada uno conserva su módulo. No se ejecutan Sanitizador y Cebo simultáneamente en una misma placa. |
| P4 | Entregas estados, métricas, motivos y procedimientos antes de cerrar cada incremento; él conecta la UI BLE y consolida pruebas con cliente AP real. |

Tu entrega final incluye los cuatro PR/encargos revisados, documentación de operación y evidencia funcional breve de todo R-08. Quedan para después los benchmarks y estabilidad prolongada, no DNS, DoH, clientes ni métricas. Seguir el [formato común de entrega](guia-equipo.md#6-rutina-por-tarea-y-entrega).

[Volver al reparto](README.md) · [Guía del equipo](guia-equipo.md)
