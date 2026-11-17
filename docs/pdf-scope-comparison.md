# Sentinel Node: comparación del alcance con la propuesta original

Revisión del **2026-11-11**. Fuente: PDF «Propuesta proyecto de Programación de artefactos (1).pdf», 12 páginas, identificado en [arquitectura](architecture.md#estado-y-fuentes). El PDF no se incorpora al repositorio. Referencias vigentes: [decisiones](technical-decisions.md), [roadmap](roadmap.md) y [reparto](encargos/README.md).

El usuario pidió corregir la asignación de alertas recientes en OLED conforme al PDF y conservar el resto del plan, documentando las diferencias. **Esta revisión cambia únicamente ese requisito funcional y su asignación.** Las otras filas describen adaptaciones, ampliaciones o limitaciones existentes; no añaden campos a contratos ni nuevas tareas de implementación.

## Cómo leer la comparación

- **Contemplado:** existe un encargo/spec para desarrollar la función; no significa implementado o probado.
- **Adaptación:** el plan vigente difiere del texto del PDF; se identifica la decisión o justificación ya documentada.
- **Limitación registrada:** el plan o contrato actual no garantiza literalmente ese detalle del PDF. El usuario indicó conservar el resto del plan; esta revisión no lo convierte en una nueva función obligatoria ni afirma que esté demostrado.
- **Validación pendiente:** requiere comprobación durante implementación. Los resultados asumidos al archivar la base no son pruebas físicas realizadas.

## Funciones principales y responsables

| Propuesta original | Páginas | Asignación vigente | Estado de alcance |
| --- | --- | --- | --- |
| ESP32, OLED SSD1306, botón, alimentación USB portátil y montaje modular | 4, 7–8 | P1: base, periféricos e integración | Contemplado; placa S3 aprobada y verificaciones físicas pendientes. La base usa OLED 128×32, coherente con la lista de componentes. |
| C/C++, PlatformIO/FreeRTOS, Mosquitto/Docker y React/Vite/Tailwind/Zustand/Capacitor | 4–5 | P1: firmware/contratos; P4: app/broker | Stack histórico del PDF/base. El destino vigente del firmware es Arduino IDE Windows y core oficial ESP32 3.3.12 con dependencias mantenidas, con build completo oficial Windows PASS según [R-01](technical-decisions.md#r-01-framework-y-entorno-de-firmware); los otros componentes conservan su stack. |
| Escudo: captura pasiva, canales, Deauth/Evil Twin/Karma, gráficos e incidentes | 6, 9 | P1: cuatro cambios Escudo; P4: app Escudo | Contemplado, con heurísticas e indicios; no se promete identificación concluyente de atacantes. |
| Auditor/Sanitizador: DNS/HTTPS/portal, STA+AP/NAT, DNS filtrado/DoH, clientes y métricas | 6–7, 9–10 | P2: cuatro cambios; P4: app Sanitizador | Contemplado; NAT+DoH y su comportamiento conjunto aún requieren pruebas funcionales. |
| Cebo: HTTP80/Telnet23, interacciones, historial, almacenamiento offline y alertas | 7, 10–11 | P3: cinco cambios; P4: app Cebo y recepción push | Contemplado, con las adaptaciones y límites indicados abajo. |
| Cambios de modo, BLE/MQTT, app instalable, desconexión y recuperación | 5, 11–12 | P1: núcleo/integración; P4: app; cada autor comprueba su módulo | Contemplado. Un modo operativo por placa y pruebas reales durante implementación. |

## OLED: corrección confirmada y asignada

El PDF, página 4, pide mostrar modo activo, conexión y alertas recientes sin abrir la app. **Persona 1** debe implementar ese comportamiento en [sentinel-integracion-tres-modos](encargos/sentinel-integracion-tres-modos.md), **tarea 1.5**, con aceptación en su [spec](../openspec/changes/sentinel-integracion-tres-modos/specs/system-integration/spec.md).

La OLED mostrará un resumen compacto de la alerta más reciente: módulo de origen, tipo y referencia temporal disponible. Debe indicar ausencia de alertas, actualizar el resumen ante una nueva y funcionar sin teléfono conectado. El código de emparejamiento mantiene su prioridad temporal. Se reutilizan eventos/journal del núcleo; el detalle histórico permanece en la app. La asignación está documentada, **todavía no implementada ni físicamente verificada**.

P2/P3 entregan los eventos de sus módulos mediante la interfaz central acordada. No reciben tareas adicionales sobre el núcleo u OLED ni comparten la autoría de esta spec. P4 consolida la evidencia; P1 realiza la integración y prueba física de su cambio.

## Adaptaciones y ampliaciones conservadas

| Tema | PDF | Plan vigente y explicación |
| --- | --- | --- |
| Placa | WROOM-32 en la descripción; WROVER en el presupuesto, págs. 4 y 7. | ESP32-S3-DevKitC-1 N16R8, por selección explícita del usuario. No exige otra radio ni modificar el montaje básico. |
| MQTT | TCP1883 y WebSockets9001, pág. 5. | TLS8883/WSS9001, identidades y ACL según R-04. La protección del transporte es una adaptación de seguridad al proyecto. |
| Transporte del Escudo | MQTT para operación online en el planteamiento general, pág. 5. | Captura sin asociación y resultados/control por BLE, según R-03. El Escudo no usa MQTT directo. Sanitizador se controla por BLE en el diseño vigente. |
| Telegram | Webhook **o** Telegram, págs. 2 y 11. | Se conserva webhook y se retira Telegram por instrucción del usuario. La app mantiene alertas internas y push. No se exige implementar ambos destinos externos alternativos del PDF. |
| Datos recibidos por Cebo | Historial con usuarios/contraseñas, incluidos datos de prueba, págs. 7 y 11. | R-10 conserva metadatos y excluye credenciales recibidas y cuerpos completos. Los casos de laboratorio usan entradas ficticias; el producto no almacena diccionarios de credenciales. Es una diferencia respecto de la tabla histórica literal del PDF. |
| Identificación de amenazas | La tabla usa el término MAC del atacante; Cebo presupone pocas falsas alarmas, págs. 6–7. | La app presenta observaciones, evidencia y nivel de sospecha. Una conexión, SSID duplicado o MAC observada no prueba compromiso ni identidad de atacante, conforme a R-09. |
| Respuesta DNS bloqueada | Dirección nula, pág. 6. | El diseño DNS responde NXDOMAIN y contabiliza el bloqueo. Es una elección técnica distinta para impedir la resolución del dominio. |
| Métricas por cliente | Listado IP/MAC, tráfico global y RSSI hacia el router, págs. 6–7. | El plan añade contadores y RSSI observado por cliente, con valor desconocido explicado cuando corresponda. Es una ampliación respecto del PDF que se conserva; no equivale al RSSI del enlace de salida. |
| Push y servicio de alertas | Push nativo y webhook, págs. 7 y 11, sin detallar su infraestructura. | Un componente pequeño separado consume eventos y entrega webhook/push. P3 implementa envío y P4 recepción Android. La biblioteca de contratos no sustituye ese servicio ni acredita entrega real. |
| Pruebas y simulación | Revisión de límites, memoria, estabilidad y alimentación; demostración conjunta, págs. 2, 10 y 12. | Comprobaciones funcionales breves durante cada incremento, pruebas focalizadas de lógica crítica y simulador como apoyo. Las campañas extensas se realizan después; no se omiten fallos que impidan funcionar ni se presentan fixtures como hardware. |

## Límites de cobertura que permanecen documentados

| Detalle original | Situación del plan conservado | Referencia |
| --- | --- | --- |
| RSSI entre ESP32 y router público, pág. 7 | El contrato expone RSSI por cliente, pero no una métrica explícita de RSSI del enlace de salida. No se afirma cobertura literal de ese indicador. Esta revisión no amplía el contrato. | [Tipos de métricas y clientes](../contracts/src/v2-types.ts). |
| Push ante el primer intento de escaneo, pág. 7 | La clasificación distingue accesos informativos y repetición; la entrega externa depende de incidentes/severidad/configuración. No hay garantía explícita de push para todo primer acceso. Solo se instrumentan interacciones de HTTP/Telnet, no escaneos arbitrarios de cualquier puerto. | [Clasificación Cebo](../openspec/changes/sentinel-cebo-eventos-historial/design.md), [push Android](../openspec/changes/sentinel-app-push/specs/app-push/spec.md). |
| MAC observada además del BSSID, págs. 6 y 9 | El evento de detección lleva BSSID y BSSID relacionado; no incluye un campo separado para toda dirección emisora observada. La distinción queda registrada, sin modificar ahora el mensaje. | [Detalles de detección](../contracts/src/v2-types.ts). |
| AP oculto Sentinel_Secure, pág. 6 | El diseño define el AP autenticado, pero no garantiza expresamente ocultamiento del SSID. Esta revisión no incorpora esa opción ni afirma que se haya probado. | [Pasarela](../openspec/changes/sentinel-sanitizador-pasarela/design.md). |
| Monitoreo desde cualquier parte del mundo, pág. 5 | El despliegue y las pruebas definidos para el broker son de LAN autorizada. No acreditan control desde otra red o Internet. El acceso remoto queda fuera de lo demostrado por ese encargo, sin exponer por ello HTTP/Telnet del Cebo. | [Broker LAN](encargos/sentinel-broker-lan.md). |
| Detección de manipulación DNS, págs. 2 y 10 | La auditoría prevé evidencia, diferencias e inconclusos; la referencia concreta de comparación se documentará al implementar la prueba. Resolver un nombre no equivale a demostrar ausencia de manipulación o MITM. | [Auditoría](../openspec/changes/sentinel-sanitizador-auditoria/design.md). |

Estas diferencias no se ocultan bajo una declaración de cumplimiento total. Si se necesitara equivalencia literal con alguno de esos puntos, requeriría una revisión de alcance posterior; esta instrucción solo corrige funcionalmente la OLED.

## Límites para evitar sobreingeniería

- Mantener un supervisor y gestor de radio, journal del ESP32 y contrato compartido; los módulos consumen esas interfaces.
- Mantener el componente de alertas acotado al webhook y push previstos, con límites de recursos; no convertirlo en una plataforma histórica o de administración adicional.
- Reutilizar vistas y transportes de la app. El detalle e historial se consultan allí; la OLED presenta un resumen breve.
- Mantener el simulador como reproducción de mensajes/fixtures. Los autores entregan pruebas reales de su componente y P4 consolida resultados.
- Las 23 asignaciones originales organizan responsabilidades y cambios pequeños; no representan 23 servicios ni autorizan añadir tecnologías. Se conservan los responsables y el resto del alcance.

Esta comparación documenta el acuerdo de trabajo y sus límites. No convierte las hipótesis H-01 a H-08 en resultados, no completa tareas pendientes ni modifica artefactos históricos archivados.
