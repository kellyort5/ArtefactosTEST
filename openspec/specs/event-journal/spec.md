# event-journal Specification

## Purpose

Conservar eventos de la base durante desconexiones y reinicios breves, ofreciendo una recuperación limitada y verificable que puedan reutilizar los módulos de seguridad.

## Requirements

### Requirement: Registro persistente circular limitado
El dispositivo SHALL persistir eventos reales de arranque, estado o transición con identificador estable y cursor ordenado, dentro de una capacidad y tamaño por evento documentados. No SHALL registrar credenciales, tokens o cuerpos de configuración.

#### Scenario: Reinicio tras evento confirmado como persistido
- **WHEN** se reinicia el dispositivo después de completar una escritura
- **THEN** el evento mantiene su identificador y puede recuperarse por el canal autorizado
- **AND** la nueva identidad de arranque no provoca colisiones con los registros anteriores

### Requirement: Recuperación paginada y pérdida visible
Un cliente autorizado SHALL poder recuperar eventos posteriores a un cursor mediante páginas limitadas. Cuando el búfer sobrescriba registros, el dispositivo SHALL informar el rango aún disponible y la pérdida correspondiente.

#### Scenario: Cursor anterior al registro disponible
- **WHEN** un cliente solicita datos ya sobrescritos
- **THEN** recibe indicación de discontinuidad y los registros disponibles sin un error que bloquee toda recuperación
- **AND** no se anuncia un historial completo inexistente

### Requirement: Entrega recuperable e idempotente
El dispositivo SHALL reintentar eventos pendientes al recuperar el transporte y conservar los identificadores al repetir entregas. La app SHALL deduplicar por identidad de nodo y evento; un acuse de broker no SHALL considerarse confirmación de recepción final por la app.

#### Scenario: Reentrega después de desconexión
- **WHEN** un evento recibido vuelve a entregarse durante sincronización
- **THEN** el historial lo conserva una sola vez y actualiza el cursor sin perder eventos nuevos

### Requirement: Fallos acotados del almacenamiento
Un fallo de escritura o lectura SHALL informarse como degradación sin borrar silenciosamente toda la partición ni bloquear el supervisor o los controles físicos.

#### Scenario: Escritura no disponible
- **WHEN** no puede persistirse un evento
- **THEN** el dispositivo muestra el fallo y no confirma persistencia inexistente
- **AND** conserva las funciones de control de la base que no dependan de esa escritura
