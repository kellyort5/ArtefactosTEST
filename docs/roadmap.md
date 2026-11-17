# Sentinel Node: roadmap de desarrollo acelerado

**Política aprobada vigente:** [persistencia operativa y USB](persistence-policy.md), definida en [sentinel-mvp-standard-persistence](../openspec/changes/sentinel-mvp-standard-persistence/design.md). Conservar identidad, bonds, configuración e historial confirmado en uso normal/reinicio/corte; USB permite reinicialización sin migración. Arduino oficial primero; justificar cada excepción mínima. La tabla nueva/cuatro claves/configuración persistente se verifican en la [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); los builds anteriores son históricos y no cierran pruebas físicas.

## Estado y objetivo confirmado

**Actualización confirmada el 2026-11-10:** desarrollar rápidamente una versión funcional sobre ESP32-S3-DevKitC-1 N16R8 con **Escudo Pasivo, Auditor/Sanitizador y Cebo completos**, aplicación Android e infraestructura real de comunicación y notificación. Los tres módulos se desarrollan en paralelo con un responsable por cambio.

La implementación incremental determina el orden, no una reducción del alcance. Evil Twin, indicios Karma pasivos, Telnet, almacenamiento persistente, sincronización, webhooks y push con la app cerrada no son ampliaciones posteriores al MVP.

**Sanitizador desde la primera ronda:** su responsable implementa auditoría, pasarela IPv4, filtrado DNS y DoH conforme a R-08 mientras avanzan Escudo, Cebo y app. La [asignación por cambio](encargos/README.md) concreta el orden y las dependencias.

Referencias: [arquitectura](architecture.md), [R-11 e hipótesis](technical-decisions.md), [brief](brief.md) y PDF identificado en arquitectura. Las decisiones funcionales de R-03, R-06, R-07 y R-10 se conservan; cambia la programación de su implementación y validación.

Los incrementos se concretan en propuestas OpenSpec y fichas de trabajo para los cuatro integrantes. Los artefactos describen el trabajo pendiente y sus comprobaciones funcionales.

## Clasificación del trabajo

| Categoría | Contenido | Tratamiento |
| --- | --- | --- |
| Funcionalidad obligatoria de la entrega | Escudo, Sanitizador y Cebo completos, app Android, comunicaciones seguras, registro, alertas e integración física. | Debe funcionar en el equipo antes de declarar terminada la entrega conjunta. |
| Decisiones arquitectónicas confirmadas | Arduino IDE Windows + paquete ESP32 Sentinel con C/C++/FreeRTOS y componentes ESP-IDF; supervisor central; exclusión de modos; BLE en Escudo; MQTT en Cebo; contratos y componente de notificaciones de R-10. | Implementarlas directamente, reutilizando componentes oficiales adecuados. |
| Detalles por concretar | Versiones compatibles, pines efectivos, límites iniciales, esquema concreto, almacenamiento sencillo, proveedor/alojamiento push. | Resolverlos en el cambio que los necesita, con decisiones concretas y revisión solo si son arquitectónicas importantes. |
| Validaciones pendientes H-01 a H-08 | Comportamiento real de hardware, captura/BLE, detectores, persistencia y notificaciones. | Comprobaciones breves durante implementación; sin fase de investigación previa. |
| Mejoras posteriores | Benchmarks, autonomía, carga prolongada, optimización y campañas extensas de estabilidad/compatibilidad. | Después del MVP, salvo problemas que impidan funcionar. No incluyen funcionalidades prioritarias sin terminar. |
| Sanitizador | Auditoría, STA+AP/NAT, DNS/Sinkhole y DoH. | Línea paralela desde la primera ronda con pruebas breves en cada incremento. |

## Dependencias y flujo

```text
[Base compartida + contratos e interfaces]
                    |
       +------------+------------+-------------+
       v            v            v             v
    [Escudo]   [Sanitizador]    [Cebo]         [App]
       |            |            |             |
       +------------+------------+-------------+
                    v
       [Integracion de los tres modos]

[Simulador separado] --> apoyo puntual, sin bloquear hardware

Despues del MVP: mediciones extensas y optimizacion
segun necesidad, sin convertirlas en una etapa previa.
```

Las líneas de Escudo, Sanitizador, Cebo y aplicación pueden desarrollarse en paralelo al disponer de los contratos y componentes que necesiten. En cada ESP32 solo se ejecuta un modo. La base se construye en incrementos útiles; no se exige terminar toda la infraestructura antes del primer flujo real app–ESP32.

La regla temporal anterior de simular antes de integrar hardware queda sustituida por la instrucción actual de implementación real desde el inicio. El simulador se mantiene separado como apoyo ligero, sin infraestructura de pruebas compleja.

## Etapa 1 — Base compartida

**Objetivo:** ejecutar firmware y conectar la app al ESP32 desde las primeras iteraciones.

| Incremento integrable | Entrega real | Comprobación breve |
| --- | --- | --- |
| 1.1 Arranque S3 | Arduino IDE Windows + core oficial ESP32 3.3.12 y dependencias mantenidas, FreeRTOS y N16R8; build completo oficial PASS y regresiones host conservadas; hardware pendiente. Las cuatro claves mínimas y dos bibliotecas se preparan una vez por equipo, retirando cinco overrides propios antiguos. Consultar el estado del build de tabla nueva en la [verificación vigente](../openspec/changes/sentinel-mvp-standard-persistence/verification.md); primera carga limpia desde IDE, USB sin garantía de conservar datos. | Compilar, grabar, arrancar y comprobar detección de memoria/pines usados. Sin estudio previo de consumo o memoria. |
| 1.2 Estado y controles | Supervisor central, estados básicos, gestión Wi-Fi/BLE, OLED y botón conectados al estado. | Orden física o de app y cambio de estado confirmado; error básico visible. |
| 1.3 BLE y Android | React/Vite/Tailwind/Zustand/Capacitor Android, autorización/configuración BLE y contrato versionado inicial. | Teléfono real conecta, intercambia estado/comando y recibe confirmación. |
| 1.4 MQTT seguro | Mosquitto/Docker, TLS/WSS, identidades/ACL, mqtt.js y adaptador del nodo para modos con conexión. | Evento y comando reales con acceso permitido; rechazo de identidad o permisos incorrectos. No activar STA/MQTT en Escudo. |
| 1.5 Registro compartido | Identificadores, marca temporal/calidad, persistencia acotada, pendientes y recuperación/deduplicación. | Registrar tras commit, desconectar, reiniciar/cortar sin upload y recuperar identidad/bonds/configuración/historial sin duplicar; Idle sin conexión ni UTC viejo. Separar USB con pérdida permitida y reconfiguración. |
| 1.6 Base de alertas | Contratos para eventos/notificaciones, separación del componente de R-10 y configuración propia fuera de repo/logs. | Concretar entrega e infraestructura al abordar 3.4/3.5; no esperar que todo push esté listo para empezar Escudo/Cebo. |

**Dependencias:** specs acotadas de cada incremento y hardware para las comprobaciones que lo requieran. H-01/H-02 se atienden aquí, no mediante una etapa 0 de investigación.

**Salida funcional:** base capaz de ejecutar y conectar incrementos de los módulos con contratos compartidos. No requiere resolver NAT, DoH, rendimiento, autonomía ni compatibilidad exhaustiva. Reutilizar el registro y transportes, evitando sistemas propios complejos.

## Etapa 2 — Escudo Pasivo completo

**Objetivo obligatorio:** implementar todo R-03 y el alcance expresamente solicitado. Las filas son iteraciones del mismo módulo, no opciones para recortar su cierre.

| Incremento integrable | Funcionalidad obligatoria | Comprobación breve |
| --- | --- | --- |
| 2.1 Descubrimiento y selección | Escaneo pasivo 2.4 GHz; SSID, BSSID, canal, RSSI y seguridad; catálogo BLE en dashboard; objetivo por SSID/BSSID; iniciar/detener/cambiar desde app. | Red propia aparece en teléfono, selección y órdenes se aplican sin asociación Wi-Fi. |
| 2.2 Captura real | Modo promiscuo, análisis Beacon/Deauthentication/Disassociation, prioridad a objetivo y recorrido de canales para anomalías relacionadas. | Captura real integrada al parser y estado visible; casos de entrada breves para tramas válidas/malformadas. |
| 2.3 Detecciones | Ráfagas de desautenticación, desconexiones sospechosas, Evil Twin por BSSID/canal/RSSI/seguridad del mismo SSID e indicios Karma observables pasivamente. | Casos benignos/sospechosos breves; verificar que SSID duplicado solo no se clasifica malicioso y que el lenguaje expresa indicios. |
| 2.4 Resultados completos | Eventos, métricas y alertas BLE; gráficos de actividad y volumen, tabla de incidentes con red/canal/tiempo e historial; estado OLED y botón. | Dato real del ESP32 aparece en dashboard; los detectores consumen el flujo real. |
| 2.5 Desconexión y coexistencia | Registro y pendientes BLE, sincronización y deduplicación; concurrencia captura/BLE primero, sincronización periódica o pausas si aparecen fallos. | Desconectar/reconectar BLE, recuperar eventos y mostrar pausas/cobertura. Corregir bloqueantes sin benchmark comparativo previo. |

**Dependencias:** capacidades mínimas de supervisor, BLE, registro y app de etapa 1. Puede avanzar junto con Cebo una vez disponibles los contratos pertinentes.

**Restricciones:** sin asociación, MQTT directo, NAT ni auditoría DNS/HTTPS. No generar pruebas activas en redes ajenas. No convertir coincidencias de SSID o una MAC observada en atribución concluyente. Karma conserva los límites de observación exclusivamente pasiva; no se elimina su heurística por no disponer de benchmarks.

**Salida funcional:** todas las filas integradas en ESP32 y app. Fixtures focalizados permiten comprobar parsers y clasificación sin exigir generar ataques en vivo; no sustituyen la captura real ni la conexión de cada detector al flujo del dispositivo. H-03/H-06 se resuelven funcionalmente y se mantienen limitaciones observadas documentadas.

## Etapa 3 — Cebo de Red Local completo

**Objetivo obligatorio:** implementar todo R-10, persistencia de R-06 y todos los canales de alertas dentro del MVP.

| Incremento integrable | Funcionalidad obligatoria | Comprobación breve |
| --- | --- | --- |
| 3.1 Servicios reales simulados | STA en LAN autorizada; panel HTTP ficticio y Telnet simulado puerto 23; conexiones, accesos e intentos de autenticación. | Acceder a ambos desde un cliente de prueba; no hay comandos ejecutables ni administración real. |
| 3.2 Eventos y clasificación | IP, puerto, protocolo, fecha/hora/calidad, tipo, cantidad y frecuencia; patrones repetitivos y nivel de sospecha. Límites de sockets/tiempos/tamaños y exclusión de datos sensibles. | Interacciones aisladas/repetidas generan clasificación esperada sin guardar contraseñas, tokens ni cuerpos completos. |
| 3.3 Persistencia y dashboard | Búfer circular persistente limitado, pendientes, MQTT, identificación/deduplicación, métricas, historial e incidentes en tiempo real; alertas visibles. | Caer/restablecer MQTT y reiniciar brevemente; registros persistidos se recuperan y sincronizan sin duplicación. Comprobar el límite con pocos registros. |
| 3.4 Webhooks | Componente separado de notificaciones, entrega webhook, control de alertas repetitivas y secretos propios de integración fuera de código/logs. | Recibir una alerta real en el webhook y comprobar supresión/agrupación de repeticiones según política inicial. El push obligatorio se integra en 3.5. |
| 3.5 Push Android real | Infraestructura necesaria de registro/envío/recepción, integrada con el componente y Capacitor; recepción con la app cerrada. | En teléfono real, cerrar la app y disparar un incidente real del Cebo; comprobar recepción push. Una alerta solo en primer plano no satisface el requisito. |
| 3.6 Conjunto Cebo | HTTP/Telnet, clasificación, registro, MQTT, historial, webhook y push funcionando juntos; estado y alertas recientes en OLED, control físico y reconexiones. | Recorrido funcional completo y estado de canales visible cuando no haya conectividad. |

**Dependencias:** supervisor, MQTT seguro, registro y app de etapa 1; la infraestructura de notificaciones se integra en 3.4/3.5. El proveedor y alojamiento se fijan en ese cambio con una solución suficiente y sencilla, sin crear una plataforma adicional innecesaria.

**Restricciones:** solo redes propias/autorizadas, sin exposición pública ni port forwarding HTTP/Telnet, sin secretos recibidos registrados ni ejecución de comandos. No confundir los servicios ficticios locales con mocks de implementación: HTTP/Telnet deben estar ejecutándose realmente en el ESP32.

**Salida funcional:** todas las filas integradas y verificadas, incluidos webhooks, push con app cerrada, persistencia y deduplicación. Las alertas externas pueden no entregarse inmediatamente sin Internet; su indisponibilidad no autoriza omitir las integraciones. H-06/H-07/H-08 se comprueban durante estos incrementos.

## Etapa 4 — Integración completa y cierre del MVP

**Objetivo obligatorio:** comprobar el sistema completo de los tres módulos y corregir errores de integración.

Trabajo:

- Cambios seguros entre Escudo, Sanitizador, Cebo e Idle mediante el canal disponible, sin modos simultáneos ni servicios residuales; cierre HTTP/Telnet al salir de Cebo, cierre AP/DHCP/NAT/DNS al salir de Sanitizador y ausencia STA/MQTT en Escudo.
- Reconciliación de comandos/estado si cambia el transporte; autorización, confirmaciones y control de duplicados.
- Gestión conjunta Wi-Fi/BLE, memoria y conexiones con límites; corregir bloqueos concretos sin optimización anticipada.
- OLED y botón físico en los tres modos; disponibilidad, errores, reconexión y resumen de la alerta más reciente visibles, también sin teléfono. Persona 1 lo integra y comprueba en la tarea 1.5 de `sentinel-integracion-tres-modos`. Conservar la prioridad del emparejamiento mediante pulsación larga de la base.
- Escudo con catálogo/objetivo, todas sus detecciones, gráficas/incidentes, pendientes y sincronización BLE.
- Cebo con HTTP/Telnet, clasificación, registro persistente e historial MQTT; webhook y push con app cerrada.
- Sanitizador con auditoría DNS/HTTPS/portal, cliente real usando AP/DHCP/NAT, reglas DNS y DoH validado, clientes/RSSI/tráfico y control BLE.
- Instalación Android y comprobación breve en teléfono real del recorrido completo; recuperación después de una desconexión y un reinicio.
- Documentar comportamiento funcional y limitaciones observadas, sin presentar garantías de rendimiento o autonomía sin mediciones.

**Dependencias:** funcionalidades completas de etapas 2 y 3 y de la línea paralela del Sanitizador. La etapa 4 comprueba y ajusta lo ya integrado; las funciones pendientes conservan su encargo y responsable.

**Criterio de cierre:** todas las funcionalidades obligatorias de Escudo, Sanitizador y Cebo implementadas en ESP32 y aplicación, con infraestructura y transportes reales. Ver la matriz de alcance inferior. Un único detector, solo HTTP, una interfaz sin integración, un mock o una demostración simulada no cierran el MVP.

No se exige una campaña de benchmarks, autonomía o estabilidad prolongada. Sí se corrigen los errores que impidan el comportamiento funcional, seguridad acordada o recuperación básica.

## Línea paralela — Auditor y Sanitizador

**Prioridad confirmada el 2026-11-10:** comenzar en paralelo desde la primera ronda. H-04/H-05 se resuelven durante sus incrementos funcionales.

Incrementos según R-08:

1. Auditoría de conectividad, DNS, certificados HTTPS y portal cautivo, con resultados explicables e inconclusos cuando corresponda.
2. STA+AP, DHCP y NAT IPv4; subredes separadas, autenticación Wi-Fi robusta, clientes, RSSI y tráfico subida/bajada.
3. DNS local/Sinkhole, filtrado, caché, timeouts y contador de bloqueos.
4. DoH con certificados válidos, integración con DNS de clientes y política de no recurrir automáticamente a DNS sin cifrar si falla.
5. Comprobaciones funcionales del conjunto y recuperación; evaluar recursos/rendimiento cuando se necesite en esa etapa, sin convertirlo en condición previa del MVP.

No prometer protección IPv6 sin implementarla, autenticación automática de todos los portales ni bloqueo de todo DoH externo. No se selecciona todavía Cloudflare o Quad9 ni se anuncia viabilidad integrada como demostrada.

## Política de pruebas durante implementación

Para cada funcionalidad:

1. Implementar el incremento real conforme a su spec revisada.
2. Compilar.
3. Ejecutar una comprobación funcional breve con el ESP32, canal y app pertinentes.
4. Corregir errores bloqueantes y actualizar la evidencia/tareas de ese incremento.
5. Continuar con el siguiente.

Mantener pruebas automatizadas pequeñas donde haya lógica crítica: parser y heurísticas, clasificación, ventanas/contadores, límites y tratamiento de datos, arbitraje de comandos, exclusión de modos, registro y deduplicación. Reutilizar fixtures y herramientas sencillas; no diseñar una plataforma de pruebas compleja.

Las hipótesis son pendientes de observación, no prerrequisitos de investigación. Una funcionalidad solo se marca terminada por su comportamiento especificado; no por existir un mock, simulador, prueba unitaria aislada o pantalla sin integración.

**Después del MVP:** benchmarks, autonomía, carga prolongada, resistencia/desgaste, campañas extensas de estabilidad y compatibilidad/segundo plano, refinamiento estadístico y optimización. La recepción push con app cerrada necesita una comprobación mínima antes de cerrar el MVP aunque la campaña exhaustiva se haga después.

## Matriz de alcance obligatorio y trazabilidad

| Funcionalidad | Etapas | Criterio funcional antes de cerrar MVP |
| --- | --- | --- |
| Stack S3 N16R8, contratos y supervisor | 1 y 4 | Firmware real, versiones fijadas y órdenes confirmadas; exclusión de modos. |
| Descubrimiento pasivo 2.4 GHz, SSID/BSSID/canal/RSSI/seguridad | 2.1 | Redes reales transmitidas por BLE al dashboard. |
| Selección, inicio, detención y cambio de objetivo | 2.1 y 4 | Acciones app aplicadas por el ESP32 sin asociación ni MQTT en Escudo. |
| Beacon, Deauthentication, Disassociation y recorrido de canales | 2.2 | Captura real priorizada al objetivo e integrada al análisis. |
| Ráfagas y desconexiones sospechosas | 2.3 | Detectores conectados al flujo real y casos de lógica focalizados. |
| Evil Twin e indicios Karma pasivos | 2.3 | Comparación BSSID/canal/RSSI/seguridad e indicios limitados; SSID duplicado no equivale a ataque. |
| Eventos, métricas, gráficos, tabla de incidentes y alertas BLE | 2.4 | Visualización integrada con datos reales y tiempo/calidad. |
| Pendientes, sincronización BLE y cobertura | 2.5 | Recuperación/deduplicación y pausas visibles cuando sean necesarias. |
| HTTP ficticio y Telnet puerto 23 | 3.1 | Servicios locales reales sin comandos ni administración efectiva. |
| Intentos, cantidad/frecuencia, clasificación y patrones | 3.2 | Metadatos y niveles de sospecha sin datos sensibles; recursos limitados. |
| Persistencia circular y sincronización MQTT | 1.5 y 3.3 | Evento persistido sobrevive al reinicio breve y recupera canal sin duplicarse. |
| MQTT, tiempo real, métricas, historial y alertas en app | 3.3 | Datos reales visibles y recuperables según límites documentados. |
| Webhooks y control de repetición | 3.4 | Entrega real en webhook; política inicial contra saturación. |
| Push Android con app cerrada | 3.5 | Infraestructura real y recepción comprobada en teléfono. |
| Android BLE/MQTT, configuración, gráficos, incidentes, historial | 1–4 | Flujos completos por canal correspondiente, autenticados y confirmados. |
| OLED con alertas recientes, botón, errores, transiciones y recuperación | 1–4 y línea paralela | Operación física y remota coherente en los tres modos; última alerta visible sin abrir la app, con prueba física a cargo de P1. |
| Auditor/NAT/DNS/DoH y métricas Sanitizador | Línea paralela | Alcance obligatorio del responsable del Módulo 2 y de la integración de los tres modos. |

## División en cambios OpenSpec pequeños

La planificación se concreta en [23 cambios con un autor por cambio](encargos/README.md). La base existente conserva su seguimiento independiente. La división es:

- Base: arranque/configuración S3; supervisor y periféricos; contratos y BLE/Android; MQTT seguro; registro persistente.
- Escudo: catálogo/selección; captura/parser; detectores; dashboard BLE; recuperación/coexistencia.
- Cebo: HTTP/Telnet; clasificación; MQTT/historial; webhooks; infraestructura push; recuperación integrada.
- Integración: transiciones, recursos y recorrido funcional final.
- Sanitizador desde la primera ronda, conforme a la decisión actualizada y sus dependencias de contratos y supervisor.

Un cambio puede cubrir una parte integrable de una funcionalidad; las tareas solo se marcan completas por el comportamiento que especifique ese cambio. Toda parte pendiente mantiene trazabilidad al alcance final del MVP. No mover funcionalidades prioritarias al backlog posterior para declarar éxito antes.

Los encargos enlazan proposal, design, specs y tasks ya preparados para revisión. El coordinador utiliza `$openspec-apply-change` cuando se solicite implementar un cambio concreto; sus compañeros pueden seguir directamente sus fichas sin usar el CLI. No hace falta volver a investigar ni pedir aprobación del alcance o stack ya confirmados. Las decisiones arquitectónicas nuevas propuestas se identifican en el índice y se revisan antes de aplicarlas.

## Límites y pendientes

La [comparación con el PDF](pdf-scope-comparison.md) registra las diferencias que el usuario indicó conservar. La corrección funcional de esta revisión es únicamente la OLED con alertas recientes; no se añaden a este roadmap otras funcionalidades por el análisis de diferencias.

No se inventan calendario ni nombres de integrantes. Los encargos asignan roles Persona 1–4 y proponen detalles técnicos revisables; no convierten proveedores o capacidades pendientes en hechos aprobados. La base ya fija versiones y GPIO configurables, documentados en firmware. Los límites iniciales recomendados se ajustan ante fallos, sin un estudio previo.

La falta de benchmarks no elimina funciones ni acredita garantías inexistentes. La coexistencia RF y los servicios limitados mantienen sus riesgos conocidos. Los problemas básicos observados se corrigen durante desarrollo; las mejoras de rendimiento/estabilidad se mantienen aparte de la definición funcional.

No se plantea comprar una segunda radio, cambiar la placa o añadir infraestructura compleja como requisito de inicio. La infraestructura mínima de notificaciones sí es obligatoria por R-10 y el alcance confirmado del usuario.
