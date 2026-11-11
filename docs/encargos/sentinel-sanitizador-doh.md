# Encargo: Sanitizador: DoH y protección completa

**Responsable único:** Persona 2 — firmware Auditor/Sanitizador. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-sanitizador-doh`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Completar la ruta real del DNS local con cifrado TLS y recuperación sin fallback plano.

Auditoría, NAT, DNS y DoH reales integrados; TLS obligatorio y fallo sin fallback plano.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-sanitizador-doh/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-sanitizador-doh/specs/sanitizer-doh/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-sanitizador-doh/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-sanitizador-dns](sentinel-sanitizador-dns.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/sanitizador/ (doh_client y estado de protección)`
- `firmware/test/sanitizador/`
- `docs/modules/sanitizador-doh.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Cliente e integración

1. **1.1** Implementar configuración HTTPS/CA y POST DoH acotado; probar respuesta válida, ID incorrecto, tamaño excesivo y tipo de contenido.
2. **1.2** Validar TLS/hora y fallos cerrados; comprobar certificado inválido y timeout sin DNS plano.
3. **1.3** Integrar caché, reintentos, métricas y estado de protección; documentar bootstrap y límites en docs/modules/sanitizador-doh.md.
4. **1.4** Compilar y probar consultas permitidas/bloqueadas desde cliente AP con NAT+DoH reales; registrar endpoint y resultado sin secretos.
### Comprobación del conjunto

1. **2.1** Cortar/restaurar upstream/DoH y consultar BLE; verificar degradación, SERVFAIL, recuperación y control disponible.
2. **2.2** Adjuntar evidencia funcional de auditoría/pasarela/DNS/DoH y pedir revisión del módulo completo sin prometer rendimiento.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Auditoría, NAT, DNS y DoH reales integrados; TLS obligatorio y fallo sin fallback plano. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-sanitizador-doh/proposal.md) · [Tareas](../../openspec/changes/sentinel-sanitizador-doh/tasks.md)
