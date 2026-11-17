# Spec Delta

## ADDED Requirements

### Requirement: Persistencia operativa y programación USB estándar
El firmware SHALL conservar identidad, vínculos BLE, configuración e historial confirmado durante operación normal, reinicios y cortes de alimentación. La programación USB SHALL permitir reinicializarlos, sin exigir migración entre versiones. La preparación SHALL priorizar compilación, carga y exportación estándar de Arduino, documentando cada personalización restante.

#### Scenario: Carga con distribución de flash nueva
- **WHEN** se prepara la primera carga USB con la distribución nueva
- **THEN** las instrucciones indican inicialización limpia y posterior configuración y emparejamiento
- **AND** no prometen conservar los datos de la distribución anterior

#### Scenario: Instalación sobre configuración local anterior
- **WHEN** se ejecuta de nuevo la preparación documentada de Arduino
- **THEN** retira las recetas propias obsoletas de carga y exportación conservando claves ajenas y respaldo
- **AND** repetir la instalación correcta no mezcla configuraciones ni cambia innecesariamente el archivo

#### Scenario: Reinicio ordinario con error de almacenamiento
- **WHEN** el dispositivo encuentra un error de inicialización NVS durante un arranque sin programación USB
- **THEN** informa el fallo sin formatear automáticamente datos persistentes
- **AND** la excepción de reinstalación USB no habilita un borrado durante ese arranque
