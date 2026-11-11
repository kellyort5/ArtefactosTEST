# Design

## Context

Ver [motivación](proposal.md). Base disponible: `sentinel-core-foundation` se archivó el 2026-11-10 con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario. Ver [cierre](../archive/2026-11-10-sentinel-core-foundation/closure.md). La aceptación no acredita pruebas físicas ni completa este cambio funcional. Fuente de verdad: [decisiones](../../../docs/technical-decisions.md), [arquitectura](../../../docs/architecture.md) y [encargos](../../../docs/encargos/README.md).

Responsable único: **Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware**. Dependencias para integrar: `sentinel-module-contracts`. Cada módulo puede preparar lógica y fixtures desde el contrato revisado; las comprobaciones reales esperan su dependencia pertinente, no toda la campaña de base.

## Goals / Non-Goals

**Goals:** Build ESP-IDF y pruebas de exclusión/gate pasan; API revisada por consumidores; hooks integrables sin editar módulos ajenos.

**Non-Goals:** implementar otros encargos, recortar funcionalidades acordadas o hacer benchmarks/campañas prolongadas. Los archivos ajenos se solicitan a su dueño. Esta planificación no aplica cambios ni autoriza operaciones Git.

## Decisions

Los acuerdos funcionales de R-02, R-05, R-06, R-07, R-11 son confirmados. Las elecciones concretas nuevas de este diseño son **propuestas de implementación**, no acuerdos históricos. Los límites iniciales son recomendaciones ajustables ante fallos sin retirar funcionalidad. Revisar con el usuario las elecciones arquitectónicas nuevas antes de aplicar; los detalles no estructurales se resuelven durante implementación.

### API propuesta para revisión

Crear un componente ESP-IDF de interfaz `sentinel_api` sin dependencia del runtime. Los módulos dependen de él; `sentinel` los registra y depende de sus implementaciones condicionalmente mediante opciones de compilación. Evita el ciclo de dependencias que produciría incluir el núcleo desde cada módulo. No crear tareas Wi-Fi independientes.

Publicar en `sentinel_api/include/module_api.hpp` tipos acotados: `Mode`, `ModuleCommand` (operation, args de solo lectura, command_id), `ModuleReply` (error/result/job_id) y `ModuleSnapshot` (estado, métricas y capacidades). Contrato de hooks: `init(ModuleContext&)`, `start()`, `request_stop()`, `stopped() const`, `handle(const ModuleCommand&,ModuleReply&)`, `tick(uint64_t)`, `snapshot(ModuleSnapshot&) const`. `start/handle/tick` no bloquean el supervisor: trabajo largo en cola del módulo. El contexto proporciona reloj, `emit_event`, `publish_metrics` y peticiones de recursos al gestor; copia los datos antes de retornar. El callback nunca retiene punteros a un comando ni escribe flash.

Matriz del gestor: Escudo=Wi-Fi sin asociación/promiscuo + BLE; Cebo=STA/MQTT + BLE; Sanitizador=STA para auditoría y STA+AP/DHCP/NAPT para pasarela + BLE; Idle=sin servicios. El gestor inicia/detiene drivers y despacha cambios de canal, scan y estados. El módulo solicita operaciones por una interfaz de recursos del contexto; no usa `esp_wifi_init/start/stop` por su cuenta. Exponer también netif STA/AP prestadas para NAT/DNS, sin transferir su propiedad. Habilitar en la configuración global `CONFIG_LWIP_IP_FORWARD=y` y `CONFIG_LWIP_IPV4_NAPT=y`, presentes en el ESP-IDF instalado, para que P2 pueda compilar su pasarela; no activar NAPT hasta que lo solicite el modo Sanitizador. Los callbacks copian metadatos a colas limitadas (8 por defecto); desbordamiento produce contadores, no bloqueo ni asignación ilimitada.

Timeout inicial de parada 5 s; si un módulo o red no libera recursos, mantener error y rechazar el siguiente modo. No activar parcialmente otro modo para ocultar un fallo. El supervisor crea ID/cursor y valida el evento antes de guardarlo en el journal NVS existente. Solo el núcleo serializa transportes y arbitra comandos. Registrar componentes opcionales por presencia: una integración aún no terminada no anuncia su capacidad.

Conservar `start_capture/start_honeypot` como órdenes reservadas de v1, sin convertirlas silenciosamente en v2. Actualizar el gate/wire para operaciones v2 y mantener toda autorización BLE/MQTT. Botón corto rota los modos disponibles añadiendo Sanitizador; conservar pulsación larga de 3 s que abre 120 s de emparejamiento de la base, sin disparar además una acción corta. Idle se solicita por comando confirmado. OLED muestra perfil, subestado y error sin secretos. En este cambio solo probar hooks con un módulo de prueba local, sin anunciar Escudo/Cebo/Sanitizador completos.

### Interfaces y propiedad

Contrato único: [protocolo funcional](../../../contracts/module-protocol.md) y [spec principal](../../specs/module-protocol/spec.md). API de firmware: [hooks y recursos](../sentinel-firmware-module-api/design.md). No inventar formatos locales, UUID distintos ni otro supervisor. La especificación del cambio define comportamiento; este diseño concreta su implementación.

Archivos de implementación: `firmware/components/sentinel_api/`, `firmware/components/sentinel/ (supervisor, runtime, wire, network y registro central)`, `firmware/main/`, `firmware/CMakeLists.txt`, `firmware/sdkconfig.defaults`, `firmware/test/`, `docs/development.md`. Los documentos de cada componente y sus pruebas son del mismo autor. Cualquier cambio en contratos/núcleo/app/broker ajenos se devuelve a su dueño para su propia spec.

## Risks / Trade-offs

- Interfaz nueva compartida → revisar firmas con Personas 2 y 3 antes de consumidores.
- Callbacks y parada → colas acotadas, prueba de fallo de liberación y un solo dueño de drivers.

## Migration Plan

Trabajar en rama `feat/sentinel-firmware-module-api`, incorporar dependencias revisadas mediante el flujo PR del equipo y conservar la base ejecutable. Compilar y comprobar brevemente cada incremento, manteniendo tests focalizados de lógica crítica. Si falla, corregir o mantener tarea pendiente; no activar capacidades inexistentes. Para retirar un incremento defectuoso, preparar corrección o reversión revisada, sin reset destructivo ni operaciones Git no autorizadas. No se asume ningún merge ya realizado.
