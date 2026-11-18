# Preparación independiente de sentinel-arduino-documentation

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

Fecha: 2026-11-13, America/Guatemala. Fase inicial del cuarto agente, tarea `task_df0a69d45168`, rama compartida `feat/sentinel-arduino-migration`. Esta entrega prepara documentación y auditoría; no cierra el cambio ni la integración oficial de dependencias.

## Contexto y límites aplicados

Se leyeron AGENTS.md, brief, arquitectura/decisiones/roadmap, la skill local openspec-apply-change, y todos los contextFiles de `openspec instructions apply --change sentinel-arduino-documentation --json`: proposal, design y tasks. Estado CLI `ready`, esquema `spec-driven`, 0/4 tareas marcadas. Por instrucción de asignación, el coordinador conserva todas las casillas; no se editó ningún tasks.md.

Se respetó propiedad por archivo: solo README.md, CONTRIBUTING.md, docs/** y este informe/verification.md del cambio. AGENTS.md no necesitó cambios. No se editaron fuentes, tools/test, app/broker/contracts/notifications/simulator, README del sketch, instalaciones ni evidencias OpenSpec históricas. Tampoco se modificaron el ZIP/mapa protegidos, ni hubo operaciones Git de publicación o cambio de rama.

La documentación actual distingue:

- Destino confirmado: Arduino IDE Windows, core oficial ESP32 3.3.12 y dependencias mantenidas, conservando comportamiento, seguridad, contratos y datos.
- Evidencia anterior: SDK propio y firmware completo compilados con CLI Windows; no acredita el core oficial sin esas adaptaciones.
- Integración pendiente: bibliotecas/versiones, configuración efectiva, receta NVS/upload y build oficial completo.
- Actualización del coordinador `msg_847465c41905`: usuario aprobó core oficial, tres teléfonos recordados y configuración mínima de nueve claves para proteger datos. wolfMQTT2.2.0 y ESP_SSLClient3.1.3 ya compilaron/enlazaron en prueba aislada oficial; todavía no prueban el firmware completo ni la instalación final. Las recetas mínimas aprobadas no exigen reconstruir un SDK ni Python del proyecto. Cuatro vínculos antiguos se conservan: detectar incompatibilidad y fallar antes de sobrescribir.
- Mantenimiento opcional: Node24 y runners .mjs; Arduino no exige Node, WSL, Python ni generación manual para compilar el validador local.

## Archivos preparados

| Grupo | Resultado |
| --- | --- |
| [README](../../../README.md), [contribución](../../../CONTRIBUTING.md) | Destino oficial en curso, límites de evidencia y comandos opcionales de mantenimiento; no restaurar PIO/CMake/runners Python retirados. |
| [Arquitectura](../../../docs/architecture.md), [decisiones](../../../docs/technical-decisions.md), [roadmap](../../../docs/roadmap.md), [comparación PDF](../../../docs/pdf-scope-comparison.md) | R-01/C-05/H-02 reflejan la prioridad oficial y preservan el antecedente; R-02/R-04–R-07 distinguen acuerdos entregados y validaciones pendientes en supervisor, contratos, journal y BLE. Se aclaran contratos MQTT5/v2 y journal/IndexedDB ya existentes, sin añadir capacidades. |
| [Firmware](../../../docs/firmware.md), [migración](../../../docs/firmware-arduino-migration.md) | Se retiran las instrucciones vigentes de instalar el SDK propio. Se conservan evidencia, versiones/hashes anteriores, particiones, invariantes y los cuatro puntos mínimos del relevo. |
| [Conectividad](../../../docs/connectivity.md) | Límite aprobado de tres teléfonos, preservación/fallo seguro ante cuatro vínculos anteriores; protocolo v2 actual y situación física real, sin atribuir prueba al transporte nuevo. |
| [Desarrollo](../../../docs/development.md) | Mapa real de responsabilidades, incluidos runtime_storage.h y runtime_time.h. Validador distribuido y mantenimiento Node, sin prebuild Python. |
| [Persona1 desde cero](../../../docs/guia-pruebas-persona-1.md), [guía física común](../../../docs/core-foundation-test-guide.md) | Cableado original, COM real, UI común v2 frente a pantalla Escudo pendiente, T01–T15 y criterios del conjunto. Verificar/Subir esperan el relevo oficial; mantenimiento Linux/Node es opcional. |
| [Reparto](../../../docs/encargos/README.md), guías P1/P2/P3 y fichas de firmware | Propiedad/API/rutas actuales y estado explícito de dependencias; no se reasignan módulos ni se anuncia el SDK propio como destino. |
| [Guías Escudo](../../../docs/modules/escudo-discovery.md), [captura](../../../docs/modules/escudo-capture.md), [detectores](../../../docs/modules/escudo-detectors.md), [recuperación](../../../docs/modules/escudo-recovery.md) | Comandos Node disponibles, mismo protocolo/limitaciones/pruebas físicas pendientes. |

La sección histórica de firmware y docs/core-foundation-check.md conserva sus comandos Python como registro de lo ejecutado. Los comandos Python actuales del broker siguen perteneciendo al broker: no se confunden con los runners de firmware retirados. Un alias de ancla conserva el enlace histórico al mapa del movimiento, sin editar el informe histórico.

## Auditoría de specs faltantes y sincronización

Las **14 capacidades de cambios archivados tienen su spec principal**. Se cotejó el inventario de `openspec/changes/archive/*/specs/*/spec.md` con `openspec/specs/*/spec.md`: ninguna capacidad archivada falta en el principal. La app v2 ya tiene [app-module-protocol](../../specs/app-module-protocol/spec.md); no se duplica.

Hay 18 capacidades declaradas en cambios activos sin principal: `push-delivery`, `notification-dispatch`, `app-cebo`, `app-escudo`, `app-push`, `app-sanitizer`, `arduino-firmware-runtime`, `arduino-sdk`, `broker-module-access`, `cebo-events`, `cebo-http`, `cebo-telnet`, `system-integration`, `module-validation`, `sanitizer-audit`, `sanitizer-dns`, `sanitizer-doh`, `sanitizer-gateway`. Son deltas de cambios activos, no ausencias que deban llenarse copiándolos como entregados. En particular `arduino-sdk` pertenece al objetivo sustituido y no se sincroniza automáticamente.

El delta `project-workspace/Compilación autónoma del firmware` de dependencias tampoco se copia todavía: se espera su implementación/evidencia y revisión. Este cambio de documentación declara `skip_specs:true`; no autoriza inventar requisitos de clases/archivos ni dar por realizadas pruebas físicas.

### Contradicciones semánticas para revisión del coordinador

No se modificó ningún requisito principal. Para evitar cambiar un contrato por interpretación propia, estos son los reemplazos exactos propuestos para la fase final; se apoyan en module-protocol, module-dispatch y las cuatro specs de Escudo ya aprobadas. El coordinador debe revisar el delta antes de aplicarlo.

1. [device-runtime](../../specs/device-runtime/spec.md), escenario «Transición de Cebo a Escudo de base», última línea:

   Antes: `- **AND** el estado identifica que captura y detección todavía no están implementadas`

   Después propuesto: `- **AND** el estado identifica la disponibilidad y el estado reales de captura y detección del módulo Escudo registrado`

   En «Recursos condicionados al perfil», reemplazar el requisito por:

   > El dispositivo SHALL impedir asociación Wi-Fi, MQTT directo y pasarela en Escudo; permitir conectividad STA/MQTT en Cebo cuando su módulo esté disponible; y rechazar modos u operaciones no implementados o no registrados.

   En «Solicitud de una función excluida», reemplazar las dos líneas por:

   > - **WHEN** se pide una operación o un modo no implementado o no registrado
   > - **THEN** responde con operación no disponible sin simular detección ni iniciar servicios ausentes

   Motivo: la base inicial no tenía captura; el dispatcher y los incrementos posteriores sí la especifican. No implica habilitar Cebo/Sanitizador antes de sus entregas.

2. [device-connectivity](../../specs/device-connectivity/spec.md), «Escudo sin conectividad Wi-Fi activa», reemplazar solo su última frase:

   Antes: `Esta base SHALL preparar el acceso a radio para futuras funciones pasivas sin iniciar detección.`

   Después propuesto: `Las funciones pasivas disponibles SHALL operar mediante el gestor compartido conforme a las especificaciones de descubrimiento, captura, detección y recuperación de Escudo.`

   Motivo: conserva las prohibiciones de asociación/escaneo activo/MQTT y elimina la prohibición heredada que contradice Escudo.

3. [message-contracts](../../specs/message-contracts/spec.md), «Mensajes versionados y acotados», sustituir `mensajes v1 validados` por `mensajes validados de la versión soportada y negociada conforme a module-protocol`. Añadir a ese requisito: `Los eventos históricos v1 SHALL conservar formato e identidad originales.`

   En «Estado y extensiones sin capacidades ficticias», sustituir `Las operaciones futuras de detección, honeypot, alertas externas y Sanitizador SHALL aparecer no disponibles en esta base.` por `Las operaciones de detección, honeypot, alertas externas y Sanitizador que todavía no estén implementadas SHALL aparecer no disponibles.`

   Motivo: module-protocol define v2 y recuperación v1; anunciar capacidad depende de la implementación real, no de una prohibición permanente de detección.

4. [project-workspace](../../specs/project-workspace/spec.md), «Separación entre base y módulos funcionales», reemplazar el requisito por:

   > La base SHALL identificar como no implementadas las capacidades de módulos todavía ausentes, conservando sus puntos de extensión y anunciando únicamente las funcionalidades realmente disponibles.

   Reemplazar la primera consecuencia del escenario por:

   > - **THEN** distinguen conectividad y perfiles de recursos de las capacidades de detección y honeypot realmente disponibles

   Motivo: coincide con module-dispatch sin afirmar que una factory o el sketch por sí solos implementan un servicio.

5. [mobile-control](../../specs/mobile-control/spec.md), título «Capacidades futuras explícitamente no disponibles» y requisito: mantener título y reemplazar su frase por:

   > La app SHALL mantener puntos de extensión para las capacidades de Escudo, Cebo y notificaciones todavía pendientes, sin presentar pantallas o datos simulados como funciones completas.

   Reemplazar el escenario completo por:

   > #### Scenario: Capacidad todavía pendiente
   > - **WHEN** se abre la aplicación y falta la implementación del módulo o de su pantalla
   > - **THEN** permite las funciones de diagnóstico, configuración y control disponibles
   > - **AND** muestra la capacidad pendiente sin datos ficticios ni éxito no confirmado

   Motivo: la app común v2 ya está entregada en software, la pantalla Escudo sigue pendiente y no corresponde excluir permanentemente futuras entregas válidas.

6. Texto de Purpose sin cambiar requisitos: module-dispatch → `Integrar módulos mediante el supervisor y recursos compartidos, anunciando únicamente capacidades reales.`; escudo-discovery → `Descubrir redes pasivamente y seleccionar el objetivo real del Escudo por BLE.`; app-module-protocol → `Adaptar versiones, control confirmado e historial común de la app antes de integrar las pantallas de cada módulo.`

Estos deltas no agregan operaciones, campos, detectores, clases ni funciones; tampoco acreditan hardware. Aunque corrigen contexto heredado, se dejan sin aplicar por el requisito de revisión previa si cambia el contrato escrito. La validación sintáctica estricta no detecta estas contradicciones de significado.

## Bloqueos y mínimo de la fase final

1. **Dependencias aún en curso:** recibir bibliotecas/versiones definitivas, instalación/opciones y build oficial completo del responsable. Distinguir la dirección/configuración mínima aprobada y la prueba aislada de bibliotecas del firmware completo todavía pendiente.
2. **NVS/upload aún en integración:** la configuración mínima de nueve claves y recetas están aprobadas en dirección; falta documentar únicamente el procedimiento final concreto probado. No ejecutar una instalación improvisada ni borrar datos/instalaciones; detectar los cuatro vínculos antiguos antes de sobrescribir.
3. **Limpieza final:** recibir señal de retirada de los seis scripts SDK y firmware/arduino-package; entonces retirar enlaces/instrucciones residuales si los hubiera. Este agente no edita esos archivos.
4. **README del sketch:** sigue siendo propiedad de dependencias hasta su liberación; no se tocó ni se declara reconciliado.
5. **Specs principales:** resolver los deltas exactos anteriores con el coordinador antes de editar requisitos. Si se incorpora el límite recientemente aprobado al principal device-connectivity, el texto exacto previsto después del requisito de control BLE es: `El dispositivo SHALL recordar hasta tres teléfonos sin borrar automáticamente vínculos existentes; si detecta cuatro vínculos de una instalación anterior incompatibles con el nuevo límite, SHALL informar el problema y fallar de forma segura antes de sobrescribir datos.` La configuración de nueve claves se conserva como decisión técnica aprobada, sin convertir sus nombres aún pendientes de entrega en requisitos funcionales. No copiar automáticamente el SDK antiguo ni deltas activos futuros.
6. **Hardware:** placa/teléfono/UI específica y módulos futuros conservan sus pendientes reales. No bloquean esta fase de preparación, ni se cierran por un build o documentación.

Los puntos mínimos por completar en docs/firmware.md y guía Persona1 están concentrados en instalación/configuración/build/subida; el resto del recorrido de cableado, cuatro detectores, recuperación y app v2 ya está preparado. La misma sesión puede reutilizarse para la fase final `task_dc277faeef60` cuando se resuelvan las dependencias.

## Verificación de esta fase

Ver [verification.md](verification.md) para comandos ejecutados, auditoría de enlaces y límites. No se repitieron builds, suites host, pruebas físicas ni benchmarks ya cubiertos por los otros responsables.
