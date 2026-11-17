# Spec Delta

## MODIFIED Requirements

### Requirement: Objetivo explícito

Escudo SHALL seleccionar y cambiar objetivo por SSID/BSSID observado y rechazar selecciones inconsistentes. SHALL conservar el objetivo aceptado durante cambios de modo, reinicios y cortes de alimentación; la programación USB permite reinicializarlo.

#### Scenario: Objetivo explícito verificable

- **WHEN** se envía una selección válida y luego un BSSID no observado
- **THEN** se confirma la primera y se rechaza la segunda sin perder el objetivo válido

#### Scenario: Objetivo recuperado sin captura automática
- **WHEN** se reinicia sin reinstalar firmware después de una selección confirmada y el usuario vuelve a Escudo
- **THEN** el estado conserva SSID, BSSID y canal seleccionados con captura detenida hasta una orden explícita
- **AND** no restaura catálogo observado, uptime anterior ni evidencias temporales de detección como si fueran actuales

#### Scenario: Fallo al guardar objetivo
- **WHEN** no puede persistirse un objetivo nuevo
- **THEN** se rechaza la selección con error visible y se conserva el objetivo aceptado anterior
- **AND** no se confirma una selección que solo existe en RAM ni se borra otro almacenamiento
