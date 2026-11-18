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
La aplicación SHALL reutilizar BLE protegido y MQTT/WS anónimo con contratos compartidos. SHALL conectar indicando endpoint e ID del nodo, sin login, usuarios, contraseña MQTT ni selector/estado de rol. SHALL conservar los secretos Wi-Fi fuera de historial y paquete.

#### Scenario: Configuración de conexión
- **WHEN** se configura nodo BLE o endpoint WS e ID de nodo
- **THEN** establece el canal real o informa fallo sin pedir cuentas

#### Scenario: Android en red de confianza
- **WHEN** se instala Android y configura ws://IP-DEL-PC:9001/mqtt
- **THEN** conecta sin CA ni credenciales MQTT
- **AND** conserva permisos Android necesarios, pairing BLE y confianza HTTPS del sistema

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

### Requirement: Diagnóstico P1 optativo sobre BLE real
La app SHALL ofrecer, solo al activar su opción de pruebas, un panel para operaciones públicas P1 mediante el transporte BLE existente y contrato v2. SHALL mostrar resultados reales correlacionados sin simular éxito, conservar secretos o habilitar servicios ausentes. La app de producto SHALL mantener el panel desactivado por defecto.

#### Scenario: Descubrir y controlar Escudo sin pantalla específica
- **WHEN** se conecta un nodo autorizado y se envían scan, catálogo, selección o captura desde el panel
- **THEN** genera sobre vigente con metadatos reales y framing compartido y muestra ACK, resultados paginados, estado y métricas recibidos
- **AND** solo un ACK aplicado o reconciliación válida acredita aplicación

#### Scenario: Rechazos controlados y reentrega explícita
- **WHEN** el usuario solicita una prueba pública de boot/revisión/caducidad incorrectos o reenvío exacto
- **THEN** envía únicamente por BLE protegido y muestra la respuesta real del nodo
- **AND** no repite automáticamente modificaciones ni afirma que un rechazo o un envío pendiente sea éxito

#### Scenario: Sin canal o sin opción de pruebas
- **WHEN** BLE está desconectado o la opción de diagnóstico está deshabilitada
- **THEN** impide ejecutar el panel y no utiliza MQTT como sustituto del Escudo

#### Scenario: Privacidad de diagnóstico
- **WHEN** se intenta usar el panel para aprovisionamiento o se suministra JSON inválido
- **THEN** rechaza con error sanitizado sin divulgar contenido privado
- **AND** los resultados de diagnóstico son acotados y efímeros; la recuperación de eventos usa el historial real existente

### Requirement: Control sin roles MQTT
La aplicación SHALL admitir órdenes públicas por MQTT conectado con estado vivo de Cebo, sin perfiles operador/observador. SHALL mantener comprobación de nodo, topic, boot/revisión/TTL, ACK y BLE exclusivo para operaciones privadas. Estas comprobaciones no SHALL presentarse como autenticación MQTT.

#### Scenario: Sesión anónima
- **WHEN** el canal MQTT está conectado y Cebo informa estado vivo
- **THEN** puede enviar una orden pública sin rol
- **AND** una operación BLE exclusiva no se publica en MQTT
