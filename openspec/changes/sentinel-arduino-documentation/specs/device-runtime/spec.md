# Spec Delta

## MODIFIED Requirements

### Requirement: Supervisor exclusivo y confirmación de transiciones
El dispositivo SHALL serializar solicitudes de app y botón, mantener como máximo un perfil operativo y confirmar el resultado aplicado o rechazado. SHALL liberar los recursos del perfil anterior antes de activar los del siguiente.

#### Scenario: Transición de Cebo a Escudo de base
- **WHEN** se acepta una solicitud válida al perfil Escudo desde Cebo
- **THEN** se detienen MQTT y STA antes de confirmar el nuevo perfil
- **AND** el estado identifica la disponibilidad y el estado reales de captura y detección del módulo Escudo registrado

#### Scenario: Fallo o solicitudes concurrentes
- **WHEN** una transición falla o llega otra orden con revisión obsoleta
- **THEN** no hay dos perfiles activos y la orden incompatible se rechaza explícitamente
- **AND** un fallo de liberación deja estado de error sin iniciar recursos incompatibles

### Requirement: Recursos condicionados al perfil
El dispositivo SHALL impedir asociación Wi-Fi, MQTT directo y pasarela en Escudo; permitir conectividad STA/MQTT en Cebo cuando su módulo esté disponible; y rechazar modos u operaciones no implementados o no registrados.

#### Scenario: Solicitud de una función excluida
- **WHEN** se pide una operación o un modo no implementado o no registrado
- **THEN** responde con operación no disponible sin simular detección ni iniciar servicios ausentes
