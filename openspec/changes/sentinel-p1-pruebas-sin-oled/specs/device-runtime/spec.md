# Spec Delta

## MODIFIED Requirements

### Requirement: Estado físico coherente con el supervisor
La pantalla SHALL mostrar perfil, fase, conectividad y errores básicos; el botón SHALL solicitar acciones al supervisor sin activar recursos por su cuenta.

#### Scenario: Uso físico del dispositivo
- **WHEN** se pulsa el botón según el patrón documentado
- **THEN** la solicitud se refleja en el estado y la pantalla muestra el resultado real
- **AND** la pulsación destinada a autorizar BLE no se interpreta también como cambio de perfil

#### Scenario: Pantalla ausente durante arranque
- **WHEN** la pantalla configurada no responde
- **THEN** el dispositivo informa el error por consola y conserva control seguro
- **AND** fuera del modo serie de pruebas explícitamente habilitado, no autoriza nuevos emparejamientos que dependan de esa pantalla

#### Scenario: Visualización serie de pruebas
- **WHEN** se compila con la opción serie de pruebas y no hay OLED
- **THEN** muestra perfil, fase, conectividad y errores del supervisor por la consola local, manteniendo OLED ausente en el estado
- **AND** no inventa una pantalla conectada ni convierte sus pruebas físicas en aprobadas

#### Scenario: Evidencia del camino real a detectores
- **WHEN** se reciben observaciones reales durante captura con el modo serie habilitado
- **THEN** la consola informa de forma acotada el contador del paso real a los detectores
- **AND** no inyecta tramas ni publica incidentes simulados como reales
