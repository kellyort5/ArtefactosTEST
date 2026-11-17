# mobile-control Specification

## Purpose

Permitir al equipo instalar una aplicación Android que controle y observe la base real de Sentinel Node por el canal disponible, sin presentar módulos pendientes como terminados.

## Requirements

### Requirement: Aplicación web compilable y Android instalable
La aplicación SHALL compilar para web y empaquetarse para Android, conservando configuración y código nativo necesarios para reproducir la instalación.

#### Scenario: Preparación de Android
- **WHEN** se compila la app y se sincroniza el proyecto nativo siguiendo la documentación
- **THEN** puede instalarse en un teléfono real y solicitar los permisos necesarios
- **AND** si BLE no está disponible en el entorno informa esa limitación en lugar de simular conexión

### Requirement: Adaptadores reales separados de la interfaz
La aplicación SHALL ofrecer adaptadores BLE y MQTT que compartan contratos y normalicen estado, órdenes, confirmaciones y eventos. Las credenciales de operación SHALL suministrarse en ejecución y no quedar incorporadas al paquete o historial.

#### Scenario: Configuración de conexión
- **WHEN** el usuario configura el nodo BLE o endpoint y cuenta MQTT autorizados
- **THEN** la app establece el canal protegido real o presenta el error verificable sin mostrar secretos

### Requirement: Estado y control coherentes con el canal
La interfaz SHALL mostrar nodo, arranque, perfil, fase, revisión, conectividad y capacidades reales, y solicitar cambios mediante el canal disponible. No SHALL confirmar éxito hasta conocer el resultado aplicado.

#### Scenario: Cambio que interrumpe el transporte
- **WHEN** una transición deja de usar el canal por el que se inició
- **THEN** la app mantiene el resultado pendiente y reconcilia por estado o confirmación del canal disponible
- **AND** no presenta el perfil solicitado como aplicado por mera publicación del comando

### Requirement: Diagnóstico y recuperación de eventos
La aplicación SHALL mostrar eventos reales de la base, su tiempo/calidad y discontinuidades; recuperar pendientes por cursor y deduplicar entregas repetidas sin inventar incidentes de seguridad.

#### Scenario: Reconexión con registros previos
- **WHEN** vuelve la conexión y el nodo reenvía eventos disponibles
- **THEN** el historial local conserva una entrada por evento e indica datos antiguos o pérdidas reportadas

### Requirement: Capacidades futuras explícitamente no disponibles
La app SHALL mantener puntos de extensión para las capacidades de Escudo, Cebo y notificaciones todavía pendientes, sin presentar pantallas o datos simulados como funciones completas.

#### Scenario: Uso de la base inicial
- **WHEN** se abre la aplicación y falta la implementación del módulo o de su pantalla
- **THEN** permite las funciones de diagnóstico, configuración y control disponibles
- **AND** muestra la capacidad pendiente sin datos ficticios ni éxito no confirmado
