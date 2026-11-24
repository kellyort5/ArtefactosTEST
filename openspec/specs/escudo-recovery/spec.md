# escudo-recovery Specification

## Purpose

El Escudo completo debe conservar eventos cuando BLE cae y mostrar periodos sin cobertura.

## Requirements

### Requirement: Persistencia sin BLE

Escudo SHALL conservar eventos en almacenamiento persistente acotado durante desconexiones y ofrecerlos tras reconectar.

#### Scenario: Persistencia sin BLE verificable

- **WHEN** BLE cae, se registra un evento y se reinicia el nodo
- **THEN** el evento permanece y se entrega con su identidad original

### Requirement: Pérdida explícita

El sistema SHALL identificar los huecos causados por sobrescritura sin presentar el historial como completo.

#### Scenario: Pérdida explícita verificable

- **WHEN** el cursor solicitado ya salió de retención
- **THEN** la página señala gap y el rango disponible

### Requirement: Coexistencia funcional

Escudo SHALL conservar todas sus funciones si necesita pausas para BLE y mostrar los intervalos sin captura.

#### Scenario: Coexistencia funcional verificable

- **WHEN** la sincronización exige pausar captura
- **THEN** se registra la pausa, se transfieren eventos y se reanuda el monitoreo
