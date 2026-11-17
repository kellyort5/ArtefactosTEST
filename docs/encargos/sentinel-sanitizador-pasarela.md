# Encargo: Sanitizador: STA+AP, DHCP y NAT

Aplicar la [política aprobada de persistencia](../persistence-policy.md): identidad, bonds, configuración aceptada e historial confirmado sobreviven al uso/reinicio/corte normal; reinstalación/actualización USB puede reinicializarlos sin migración. Primera carga de tabla nueva limpia desde IDE. Los módulos, responsables y pendientes físicos se conservan; el estado del build final se registra en la [verificación vigente](../../openspec/changes/sentinel-mvp-standard-persistence/verification.md).

P2 extenderá la persistencia oficial a configuración upstream/AP/DNS/DoH/reglas aceptadas cuando implemente cada encargo. Validar límites, guardar/commit antes de confirmar, conservar anterior ante fallo y restaurar en Idle sin activar STA/AP/NAT/DoH ni confiar en UTC viejo. Sustituye RAM-only para configuración; caché DNS y clientes observados siguen siendo datos transitorios. NVS estándar no promete cifrado físico. No crear otro gestor/journal ni dar módulos futuros por implementados.

**Responsable único:** Persona 2 — firmware Auditor/Sanitizador. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-sanitizador-pasarela`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-13):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

La protección necesita un cliente conectado a Sentinel_Secure con salida IPv4 y estado/contadores reales.

Cliente real con DHCP/NAT y métricas verificadas; cierre del modo sin recursos residuales.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-sanitizador-pasarela/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-sanitizador-pasarela/specs/sanitizer-gateway/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-sanitizador-pasarela/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-sanitizador-auditoria](sentinel-sanitizador-auditoria.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/SentinelNode/src/sanitizador/ (gateway, clients y traffic)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-gateway.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Pasarela y métricas

1. **1.1** Declarar requisitos de configuración de la integración Arduino a P1 y configurar netifs mediante sentinel_api; verificar build con forwarding/NAPT y sin drivers duplicados.
2. **1.2** Implementar AP autenticado, DHCP y detección de solapamiento; probar validaciones de clave/subred y documentar configuración en docs/modules/sanitizador-gateway.md.
3. **1.3** Integrar NAPT y parada/reconexión; comprobar que no hay port forwarding ni protección IPv6 anunciada.
4. **1.4** Implementar lista de clientes, RSSI y contadores IPv4 por dirección; probar cálculo/delta y páginas v2 con casos focalizados.
### Comprobación del incremento

1. **2.1** Compilar y conectar un teléfono o portátil al AP real; verificar DHCP, salida IPv4 y subida/bajada con endpoint del equipo.
2. **2.2** Cambiar modo y desconectar upstream; registrar cierre completo y degradación visible sin campaña de rendimiento.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Cliente real con DHCP/NAT y métricas verificadas; cierre del modo sin recursos residuales. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-sanitizador-pasarela/proposal.md) · [Tareas](../../openspec/changes/sentinel-sanitizador-pasarela/tasks.md)
