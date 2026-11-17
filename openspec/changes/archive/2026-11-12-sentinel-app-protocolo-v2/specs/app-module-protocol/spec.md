# Spec Delta

## Purpose

La app base solo valida v1 y no tiene control/estado funcional de tres módulos; hace falta un adaptador común antes de pantallas.

## ADDED Requirements

### Requirement: Adaptación de versiones

La app SHALL reconocer v1/v2 y mostrar solo capacidades disponibles conservando eventos históricos sin cambiar IDs.

#### Scenario: Adaptación de versiones verificable

- **WHEN** se conecta a base v1 y luego nodo v2
- **THEN** se adapta la interfaz y conserva historial sin duplicación

### Requirement: Control confirmado

La app SHALL autenticar el canal, correlacionar comandos y mostrar errores o pendientes hasta confirmar el efecto.

#### Scenario: Control confirmado verificable

- **WHEN** un cambio de modo es rechazado o se pierde transporte
- **THEN** no muestra éxito falso y consulta estado por el canal disponible

### Requirement: Historial y privacidad

La app SHALL sincronizar eventos paginados con dedup/gap visibles y no conservar secretos de formularios en su historial.

#### Scenario: Historial y privacidad verificable

- **WHEN** llegan eventos repetidos y un hueco de retención
- **THEN** muestra un registro por ID y señala el hueco sin guardar contraseñas
