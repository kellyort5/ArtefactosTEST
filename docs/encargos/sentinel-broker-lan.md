# Encargo: Broker: LAN, v2 y rol de alertas

**Responsable único:** Persona 4 — app, Android, broker y coordinación del laboratorio. **Estado:** documentado, pendiente de revisión e implementación. **Rama prevista:** `feat/sentinel-broker-lan`. El nombre de persona se sustituye por el del integrante al repartir; tú eres Persona 1.

## Qué debes entregar

El broker vigente ofrece TCP1883/WS9001 autenticados y ACL v1/v2. Quedan pruebas físicas LAN y rol mínimo de notificaciones.

TCP/WS LAN real y ACL v1/v2 por nodo, con rol mínimo de notificaciones.

Lee primero esta ficha, el [diseño concreto](../../openspec/changes/sentinel-broker-lan/design.md) y los [casos de aceptación](../../openspec/changes/sentinel-broker-lan/specs/broker-module-access/spec.md). No necesitas aprender OpenSpec ni generar propuestas. Las [tareas oficiales](../../openspec/changes/sentinel-broker-lan/tasks.md) son el seguimiento del coordinador; informa tus resultados por los mismos números.

## Dependencias para integrar

- [sentinel-module-contracts](sentinel-module-contracts.md)

Puedes preparar estructura/lógica/UI con fixtures del contrato revisado mientras llegan las dependencias. Marca esos resultados como fixtures; cerrar este encargo requiere el resultado real indicado. Si falta una interfaz, avisa a su dueño; no la inventes en tu carpeta ni modifiques archivos ajenos.

## Archivos de tu encargo

- `broker/ (config, scripts, pruebas y README)`
- `docs/modules/broker-lan.md`

Un mismo autor puede tener varios encargos en su carpeta: desarrolla uno por rama/PR para evitar superposición. Otro autor revisa el PR, sin coimplementar la spec. Los cambios compartidos se solicitan al propietario de contratos (P1), núcleo (P1), app/lock raíz (P4), broker (P4) o notificaciones (P3).

## Pasos pequeños y comprobación

### LAN y ACL

1. **1.1** Verificar acceso LAN por IP a TCP1883/WS9001 sin certificados; conservar cuentas y documentar firewall local y aislamiento de clientes.
2. **1.2** Añadir ACL v2 y rol notify preservando replay v1/events; probar cada permiso y rechazo cross-node con cuentas temporales.
3. **1.3** Documentar Compose, host/firewall y cuentas en broker/README.md y docs/modules/broker-lan.md; verificar comandos sin secretos reales.
4. **1.4** Ejecutar comprobación de configuración y smoke TCP/WS; registrar resultados con broker real.
### Comprobación del incremento

1. **2.1** Conectar teléfono y ESP32 por LAN autorizada por IP sin certificados; registrar identidad/topic permitido sin credenciales.
2. **2.2** Comprobar que el broker no publica HTTP/Telnet y entregar a P3 la configuración local de rol por canal privado fuera de Git.

## Cómo entregar a tu compañero

Antes de editar, revisa `git branch --show-current` y `git status --short`; usa la rama de esta ficha. No descartes cambios previos. Implementa en incrementos del diseño, compila y prueba brevemente cada uno. Conserva tests focalizados de la lógica crítica. Reporta errores bloqueantes antes de ampliar el alcance.

El PR debe enlazar esta ficha y la propuesta, describir el cambio e incluir comandos/resultados, hardware/red autorizada cuando aplique, IDs de eventos sin secretos y pruebas pendientes explícitas. Otro integrante revisa. Los commits usan Conventional Commits. No incluir `.env`, claves, tokens, dependencias o compilaciones; conservar código/configuración fuente y lockfiles. El coordinador no ejecuta commits, push, merges ni borra ramas sin autorización explícita.

**Criterio de terminación:** TCP/WS LAN real y ACL v1/v2 por nodo, con rol mínimo de notificaciones. Una pantalla, fixture o mock aislado no acredita integración real. No marcar hardware o recepción push como comprobados si no se ejecutaron.

[Volver al reparto](README.md) · [Propuesta](../../openspec/changes/sentinel-broker-lan/proposal.md) · [Tareas](../../openspec/changes/sentinel-broker-lan/tasks.md)
