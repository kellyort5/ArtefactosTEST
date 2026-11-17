# Encargo: Cebo: Telnet simulado

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P3 persistirá configuración aceptada de Cebo e historial sanitizado mediante recursos centrales, con límites, commit antes de confirmar y recuperación tras reinicio/corte sin USB; no iniciar servicios automáticamente. Mantener eventos/IDs/cursores y dedup dentro del ring. No guardar credenciales recibidas por HTTP/Telnet/honeypot o terceros. NVS estándar no promete cifrado físico; secretos propios solo por canal autorizado, nunca Git/log/event/status. Webhook y push mantienen sus encargos completos.

**Responsable único:** Persona 3 — firmware Cebo y servidor de notificaciones. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-cebo-telnet`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-13):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

R-10 incluye puerto 23 y autenticación simulada además de HTTP; ambos son obligatorios.

HTTP y Telnet reales simultáneos en Cebo con respuestas seguras y sin secretos.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-cebo-telnet/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-cebo-telnet/specs/cebo-telnet/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-cebo-telnet/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-cebo-http](sentinel-cebo-http.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/SentinelNode/src/cebo/ (telnet_service y límites compartidos)`
- `firmware/test/cebo/`
- `docs/modules/cebo-telnet.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Telnet simulado

1. **1.1** Implementar listener/sesiones en puerto23 con estados e IAC acotado; probar entradas fragmentadas y línea excesiva.
2. **1.2** Integrar bus de interacciones y datos no sensibles; verificar que un marcador ficticio no sale a logs/eventos.
3. **1.3** Unificar límites HTTP/Telnet y parada; probar timeout y cierre del par de servicios.
4. **1.4** Documentar interacción esperada y límites en docs/modules/cebo-telnet.md; comprobar instrucciones con cliente LAN.
### Comprobación del incremento

1. **2.1** Compilar y conectar cliente Telnet real en red propia junto a HTTP; observar eventos y respuestas simuladas.
2. **2.2** Salir de Cebo y comprobar puertos80/23 cerrados y ninguna tarea/sesión residual.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** HTTP y Telnet reales simultáneos en Cebo con respuestas seguras y sin secretos. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-cebo-telnet/proposal.md) · [Tareas](../../openspec/changes/sentinel-cebo-telnet/tasks.md)
