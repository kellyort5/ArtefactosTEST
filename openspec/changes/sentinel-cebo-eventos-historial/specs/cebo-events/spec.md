# Spec Delta

## Purpose

Las interacciones HTTP/Telnet necesitan clasificación, métricas, persistencia offline y recuperación del historial.

## ADDED Requirements

### Requirement: Clasificación de conducta

Cebo SHALL registrar origen, puertos, protocolo, tipo, tiempo/calidad, cantidad y frecuencia y clasificar patrones repetitivos con evidencia.

#### Scenario: Clasificación de conducta verificable

- **WHEN** un origen hace accesos aislados y luego varios intentos ficticios
- **THEN** los contadores y la sospecha reflejan la diferencia sin atribución concluyente

### Requirement: Registro persistente offline

Cebo SHALL conservar eventos pendientes en un búfer persistente limitado y recuperarlos tras desconexión/reinicio/corte sin programación USB con IDs estables. SHALL confirmar persistencia solo tras commit. La reinstalación/actualización USB permite reinicialización sin migración y no habilita borrado automático en reinicios normales.

#### Scenario: Registro persistente offline verificable

- **WHEN** MQTT cae, ocurre una interacción y se reinicia el nodo
- **THEN** el evento sigue disponible y se sincroniza al volver el canal

### Requirement: Historial MQTT deduplicable

Cebo SHALL publicar eventos y métricas por MQTT anónimo del núcleo en la LAN autorizada y permitir lectura paginada con detección de huecos.

#### Scenario: Historial MQTT deduplicable verificable

- **WHEN** se repite una entrega o el cursor ya fue sobrescrito
- **THEN** el consumidor reconoce la identidad repetida o recibe gap explícito

### Requirement: Configuración de Cebo persistente y privada

Cebo SHALL conservar configuración propia aceptada en NVS oficial y recursos centrales con validación, límites y confirmación después de commit; SHALL conservar la anterior ante fallo y restaurar en Idle sin activar servicios ni confiar en UTC anterior. No SHALL guardar credenciales recibidas por HTTP/Telnet/terceros ni secretos en Git, eventos, estado o logs. No se promete cifrado físico ni continuidad tras reinstalación/actualización USB.

#### Scenario: Reinicio con configuración de Cebo confirmada
- **WHEN** se reinicia sin programar USB después de confirmar configuración persistida
- **THEN** se restaura la configuración válida sin iniciar servicios y siguen disponibles los eventos retenidos con sus IDs
