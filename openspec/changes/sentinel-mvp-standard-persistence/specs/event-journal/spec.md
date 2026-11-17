# Spec Delta

## MODIFIED Requirements

### Requirement: Registro persistente circular limitado
El dispositivo SHALL persistir eventos reales de arranque, estado o transición con identificador estable y cursor ordenado, dentro de una capacidad y tamaño por evento documentados. No SHALL registrar credenciales, tokens o cuerpos de configuración.

#### Scenario: Reinicio tras evento confirmado como persistido
- **WHEN** se reinicia el dispositivo después de completar una escritura
- **THEN** el evento mantiene su identificador y puede recuperarse por el canal autorizado
- **AND** la nueva identidad de arranque no provoca colisiones con los registros anteriores

#### Scenario: Corte de alimentación sin reinstalación
- **WHEN** se recupera alimentación después de completar la persistencia de un evento
- **THEN** el registro sigue recuperable dentro de la capacidad circular documentada
- **AND** una escritura interrumpida no se anuncia como persistida ni autoriza borrar el almacén completo

#### Scenario: Reinstalación o actualización USB
- **WHEN** se reinstala o actualiza el firmware mediante USB
- **THEN** está permitida la reinicialización del historial sin migración entre versiones
- **AND** esa excepción no se aplica a reinicios ordinarios ni a reconexiones de transporte
