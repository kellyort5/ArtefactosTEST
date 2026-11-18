# Spec Delta

## Purpose

Las alertas internas y WS no cumplen recepción con la aplicación cerrada.

## ADDED Requirements

### Requirement: Registro móvil seguro

Android SHALL solicitar permisos, registrar/renovar su token con el servidor y permitir baja sin exponer secretos.

#### Scenario: Registro móvil seguro verificable

- **WHEN** se concede permiso y el token cambia
- **THEN** el registro propio se actualiza y no se conserva el anterior como destino activo

### Requirement: Recepción con app cerrada

La solución SHALL mostrar una notificación móvil de incidente Cebo con la app cerrada por recientes en un teléfono compatible y con permisos.

#### Scenario: Recepción con app cerrada verificable

- **WHEN** un incidente real ocurre con la app cerrada y red disponible
- **THEN** el teléfono muestra push sin depender de una sesión MQTT de la app

### Requirement: Apertura de incidente

Al tocar push, la app SHALL abrir el módulo y resolver el ID de evento o mostrar recuperación/retención pendiente sin fabricar datos.

#### Scenario: Apertura de incidente verificable

- **WHEN** se toca una alerta cuyo evento aún no está local
- **THEN** abre Cebo, intenta sincronizar y comunica el resultado real
