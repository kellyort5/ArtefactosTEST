# Encargo: Escudo: descubrimiento y objetivo

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** implementación de software y build verificados (4/6 tareas); pruebas físicas 2.1 y 2.2 pendientes. Ver [guía](../modules/escudo-discovery.md). **Rama prevista:** `feat/sentinel-escudo-descubrimiento`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

El perfil Escudo actual solo declara estado; necesita catálogo pasivo y selección real antes de capturar una red.

Catálogo y objetivo reales por BLE; selección confirmada; ausencia de asociación/MQTT.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-escudo-descubrimiento/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-escudo-descubrimiento/specs/escudo-discovery/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-escudo-descubrimiento/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-firmware-module-api](sentinel-firmware-module-api.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/escudo/ (module, discovery, target y CMake)`
- `firmware/test/escudo/`
- `docs/modules/escudo-discovery.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Catálogo y selección

1. **1.1** Registrar hooks Escudo y scan pasivo mediante gestor; compilar y verificar callback de fin sin asociación.
2. **1.2** Implementar catálogo limitado y páginas v2; comprobar casos de SSID oculto, duplicado BSSID y overflow con fixtures focalizados.
3. **1.3** Implementar selección/cambio y ACK; verificar rechazo de BSSID/canal inconsistente sin mutar objetivo.
4. **1.4** Documentar operaciones y datos no observables en docs/modules/escudo-discovery.md; cotejar ejemplos con contratos.
### Comprobación del incremento

1. **2.1** Compilar pio run -d firmware y comprobar catálogo/selección por BLE en ESP32 con LAN propia; registrar datos y ausencia STA/MQTT.
2. **2.2** Comprobar salida a Idle durante scan y repetir un scan; registrar comportamiento y errores corregidos.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Catálogo y objetivo reales por BLE; selección confirmada; ausencia de asociación/MQTT. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-escudo-descubrimiento/proposal.md) · [Tareas](../../openspec/changes/sentinel-escudo-descubrimiento/tasks.md)
