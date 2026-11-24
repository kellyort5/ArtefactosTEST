# Encargo: Escudo: todos los detectores

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** 5/6 tareas verificadas en software; evidencia y prueba física 2.1 pendiente en [verification.md](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/verification.md). **Rama prevista:** `feat/sentinel-escudo-detectores`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.


**Flujo vigente de firmware (2026-11-17):** [Arduino IDE Windows con core oficial ESP32 3.3.12](../firmware.md), sketch único `firmware/SentinelNode/SentinelNode.ino`. El firmware completo tiene build oficial Windows PASS con las dos bibliotecas y configuración local documentadas; el SDK propio queda como antecedente. Las fuentes de módulos están bajo `SentinelNode/src/`; la [API/factories y propiedad de recursos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) se conservan. Los artefactos archivados documentan el entorno anterior; sus resultados físicos pendientes no se completan con el port. Sanitizador/Cebo continúan sin servicios implementados y sus flags permanecen en 0 hasta sus entregas reales.

## Qué debes entregar

Las observaciones necesitan detección completa y explicable; un solo detector no cumple R-03.

Cuatro detectores integrados; casos benignos/sospechosos focalizados y eventos válidos, sin confundir indicios con certeza.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/design.md) y los [casos de aceptación](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/specs/escudo-detection/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-escudo-captura](sentinel-escudo-captura.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/SentinelNode/src/escudo/ (detectors y evidence)`
- `firmware/test/escudo/`
- `docs/modules/escudo-detectors.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

**Equivalencia de herramienta (tarea 2.1):** el tasks.md archivado conserva `pio run -d firmware` como evidencia histórica. Hoy se compila el mismo firmware con **Verificar** en Arduino IDE Windows/core oficial 3.3.12, según [firmware](../firmware.md); se conservan los casos de aceptación y sus pendientes físicos. No reinstalar PlatformIO para ejecutar esta ficha.

## Pasos pequeños y comprobación

### Heurísticas y evidencia

1. **1.1** Implementar ventanas y detectores Deauth/desconexión; probar umbral, expiración y enfriamiento con reloj controlado.
2. **1.2** Implementar comparación Evil Twin; probar SSID duplicado benigno y diferencias de seguridad/canal/RSSI.
3. **1.3** Implementar correlación Karma pasiva y caso inconcluso; verificar fixtures positivos/negativos sin inyección de tramas.
4. **1.4** Conectar los cuatro detectores a capture/parser y eventos/métricas v2; verificar que cada familia tiene fixture y salida válida.
5. **1.5** Documentar umbrales, evidencia y límites en docs/modules/escudo-detectors.md; revisar que no se presentan atribuciones concluyentes.
### Comprobación del incremento

1. **2.1** Compilar desde Arduino IDE Windows y comprobar observaciones benignas reales recorriendo los detectores; adjuntar pruebas de lógica y recepción BLE.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Cuatro detectores integrados; casos benignos/sospechosos focalizados y eventos válidos, sin confundir indicios con certeza. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/proposal.md) · [Tareas](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/tasks.md)
