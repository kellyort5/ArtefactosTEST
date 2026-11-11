# Encargo: API de módulos y supervisor

**Responsable único:** Persona 1 — tú: OpenSpec, contratos, núcleo, Escudo e integración de firmware. **Estado:** implementación fusionada en develop por PR #6 y archivada con 6/7 tareas; prueba física 2.2 pendiente. **Rama prevista:** `feat/sentinel-firmware-module-api`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

Resources y Profile solo ejecutan la base; los otros autores necesitan integrar sus componentes sin editar runtime ni crear controladores Wi-Fi duplicados.

Build ESP-IDF y pruebas de exclusión/gate pasan; API revisada por consumidores; hooks integrables sin editar módulos ajenos.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/design.md) y los [casos de aceptación](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/specs/module-dispatch/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-module-contracts](sentinel-module-contracts.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `firmware/components/sentinel_api/`
- `firmware/components/sentinel/ (supervisor, runtime, wire, network y registro central)`
- `firmware/main/`
- `firmware/CMakeLists.txt`
- `firmware/sdkconfig.defaults`
- `firmware/test/`
- `docs/development.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### Interfaz integrable

1. **1.1** Crear tipos y hooks de sentinel_api y documentar propiedad/lifetime; verificar compilación de un consumidor mínimo de prueba.
2. **1.2** Extender supervisor y despacho v2 con registro opcional; verificar pruebas de exclusión, autorización y capacidad ausente.
3. **1.3** Adaptar gestor Wi-Fi/BLE, préstamo de netifs y flags globales IP_FORWARD/IPV4_NAPT para tres perfiles; compilar y comprobar que solo el gestor inicia/detiene drivers y que NAPT permanece inactivo fuera de Sanitizador.
4. **1.4** Conectar hooks de evento/métrica al journal y serializador; verificar rechazo de detalle inválido y límite de cola.
5. **1.5** Documentar hooks y registro en docs/development.md con ejemplo de ciclo de vida; verificar la ruta de integración de P2/P3.
### Integración del núcleo

1. **2.1** Verificar parada con timeout, error visible y botón/OLED para perfiles registrados mediante prueba focalizada y compilación pio run -d firmware.
2. **2.2** En ESP32, comprobar Idle y una transición de base por BLE sin servicios residuales; registrar evidencia o mantener pendiente si falta hardware.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** Build ESP-IDF y pruebas de exclusión/gate pasan; API revisada por consumidores; hooks integrables sin editar módulos ajenos. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/proposal.md) · [Tareas](../../openspec/changes/archive/2026-11-11-sentinel-firmware-module-api/tasks.md)
