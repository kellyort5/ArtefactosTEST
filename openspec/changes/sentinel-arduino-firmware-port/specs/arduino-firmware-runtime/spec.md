# Spec Delta

## Purpose

Migrar la implementación y extensión de todos los modos a Arduino IDE Windows conservando la funcionalidad real y contratos de Sentinel.

## ADDED Requirements

### Requirement: Fuente modular y runtime único

El firmware SHALL construirse desde un sketch modular con fuente única, setup/loop, un supervisor y un gestor de radio, conservando la API de todos los módulos.

#### Scenario: Cambio de sistema de construcción

- **WHEN** se compila el firmware desde Arduino IDE Windows
- **THEN** se enlaza un único app_main y las factories reales sin iniciar modos o servicios ausentes.

### Requirement: Escudo conservado íntegramente

La migración SHALL conservar descubrimiento, selección, captura, canales, cuatro detectores, cobertura, eventos y recuperación sin reescribir su comportamiento para obtener un build.

#### Scenario: Regresión antes y después

- **WHEN** se comparan fuentes y se ejecutan los tests críticos del Escudo antes/después del port
- **THEN** los casos positivos/negativos y de recuperación siguen pasando sin reducir sus aserciones ni simular evidencia física.

### Requirement: Seguridad y datos compatibles

El firmware SHALL conservar GATT/framing, contratos v2 e históricos v1, MQTT5/TCP autenticado en LAN sin certificados según sentinel-mqtt-mvp-lan, SC-only, persistencia operativa NVS de identidad/bonds/configuración/historial confirmado según sentinel-mvp-standard-persistence (USB puede reinicializar sin migración); validar comandos, resultados y datos con los esquemas existentes.

#### Scenario: Reconstrucción y recuperación

- **WHEN** el IDE construye el sketch con el core oficial, las dependencias fijadas y el validador local distribuido
- **THEN** no requiere cambios de app/broker y conserva formatos/IDs/cursores; un fallo no provoca borrado ni transporte inseguro.

### Requirement: Flujo Windows y verificación honesta

El proyecto SHALL documentar y comprobar compilación, subida y monitor de todos los módulos desde Arduino IDE Windows, separando evidencia host, IDE y placa.

#### Scenario: Dispositivo no conectado

- **WHEN** se termina la migración de software sin un COM identificado
- **THEN** la subida física permanece pendiente y nunca se presenta un mock o build como prueba de radio, persistencia o servicios reales.
