# Spec Delta

## Purpose

Cambios aislados no acreditan recursos compartidos, transiciones, OLED/botón y recuperación del sistema completo.

## ADDED Requirements

### Requirement: Transiciones completas

El sistema SHALL cambiar entre Idle, Escudo, Sanitizador y Cebo liberando servicios del modo anterior y rechazando fallos de parada.

#### Scenario: Transiciones completas verificable

- **WHEN** se recorre cada par de modos disponibles
- **THEN** no hay AP/NAT/HTTP/Telnet/MQTT residual incompatible y el estado coincide

### Requirement: Controles físicos integrados

El sistema SHALL mostrar modo/subestado/error en OLED y permitir control físico coherente con órdenes remotas.

#### Scenario: Controles físicos integrados verificable

- **WHEN** se usa botón y luego una orden de app
- **THEN** OLED/app reflejan el mismo modo y se confirma la transición

### Requirement: Recuperación conjunta

El sistema SHALL conservar identidad, bonds, configuración aceptada e historial confirmado durante uso normal, reinicios y cortes sin programación USB, recuperar canales autorizados sin secretos ni incidentes duplicados y arrancar en Idle sin conexiones automáticas ni UTC obsoleto. La reinstalación/actualización USB SHALL permitir reinicialización sin migración.

#### Scenario: Recuperación conjunta verificable

- **WHEN** se corta BLE/MQTT y reinicia brevemente
- **THEN** los eventos retenidos reaparecen con identidad estable y pérdidas explícitas

### Requirement: Alertas recientes en OLED

El sistema SHALL mostrar en la OLED un resumen de la alerta más reciente con módulo de origen, tipo y referencia temporal disponible, sin secretos y sin depender de que la app esté abierta o conectada. La presentación SHALL actualizarse con nuevas alertas e indicar explícitamente cuando no exista ninguna.

#### Scenario: Alerta visible sin teléfono

- **WHEN** un módulo genera una alerta real sin teléfono conectado
- **THEN** la OLED muestra su resumen con el origen y tipo correctos mientras el módulo continúa funcionando
- **AND** si falta hora UTC, la pantalla usa una referencia relativa o indica hora desconocida sin inventar una fecha

#### Scenario: Ausencia y actualización de alertas

- **WHEN** el dispositivo no tiene alertas recientes y después recibe dos alertas sucesivas
- **THEN** muestra primero la ausencia de alertas y después actualiza el resumen con la más reciente sin mezclar sus módulos de origen

#### Scenario: Emparejamiento con alerta reciente

- **WHEN** existe una alerta reciente y se abre la ventana física de emparejamiento
- **THEN** el código de emparejamiento mantiene su prioridad y al terminar vuelve la presentación normal con la alerta reciente

### Requirement: Entrega funcional completa

La entrega SHALL integrar todo R-03, R-08 y R-10 en ESP32/app, incluidos webhook, push con la app cerrada y alertas recientes en OLED.

#### Scenario: Entrega funcional completa verificable

- **WHEN** se revisa cierre con un detector o canal ausente
- **THEN** el sistema sigue pendiente y no se recorta el alcance para declararlo terminado
