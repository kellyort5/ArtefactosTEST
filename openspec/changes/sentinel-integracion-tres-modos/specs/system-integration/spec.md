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

El sistema SHALL conservar historial y recuperar canales tras una desconexión/reinicio sin secretos ni incidentes duplicados.

#### Scenario: Recuperación conjunta verificable

- **WHEN** se corta BLE/MQTT y reinicia brevemente
- **THEN** los eventos retenidos reaparecen con identidad estable y pérdidas explícitas

### Requirement: Entrega funcional completa

La entrega SHALL integrar todo R-03, R-08 y R-10 en ESP32/app, incluidos los tres canales externos y push cerrado.

#### Scenario: Entrega funcional completa verificable

- **WHEN** se revisa cierre con un detector o canal ausente
- **THEN** el sistema sigue pendiente y no se recorta el alcance para declararlo terminado
