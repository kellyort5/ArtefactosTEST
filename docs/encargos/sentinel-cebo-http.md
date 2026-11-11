# Encargo: Cebo: panel HTTP ficticio

**Responsable único:** Persona 3 — firmware Cebo y servidor de notificaciones. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-cebo-http`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

La base Cebo solo conecta MQTT; falta un servicio HTTP real seguro que genere interacciones normalizadas.

Panel ESP32 accesible y eventos sanitizados; recursos acotados y cierre real.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-cebo-http/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-cebo-http/specs/cebo-http/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-cebo-http/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-firmware-module-api](sentinel-firmware-module-api.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/cebo/ (module, http_service y CMake)`
- `firmware/test/cebo/`
- `docs/modules/cebo-http.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Servicio y privacidad

1. **1.1** Crear módulo y bus interno de interacciones Cebo; compilar con hooks sin editar núcleo ni controladores de red.
2. **1.2** Implementar panel/rutas/formulario con esp_http_server; verificar accesos y auth_attempt desde cliente LAN propio.
3. **1.3** Añadir límites, sanitización y descarte de datos; probar marcador ficticio de secreto ausente en todas las salidas.
4. **1.4** Documentar rutas, límites y datos registrados en docs/modules/cebo-http.md; cotejar evento con v2.
### Comprobación del incremento

1. **2.1** Compilar pio run -d firmware y comprobar HTTP real y eventos desde teléfono/PC en LAN autorizada.
2. **2.2** Detener Cebo/cambiar modo y comprobar puerto 80 cerrado, sin ejecución ni exposición a Internet.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Panel ESP32 accesible y eventos sanitizados; recursos acotados y cierre real. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-cebo-http/proposal.md) · [Tareas](../../openspec/changes/sentinel-cebo-http/tasks.md)
