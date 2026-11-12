# Spec Delta

## Purpose

El perfil Escudo actual solo declara estado; necesita catálogo pasivo y selección real antes de capturar una red.

## ADDED Requirements

### Requirement: Catálogo pasivo

Escudo SHALL descubrir redes 2.4 GHz sin asociación ni probes activos y entregar SSID, BSSID, canal, RSSI y seguridad observada por BLE.

#### Scenario: Catálogo pasivo verificable

- **WHEN** se escanea una LAN propia
- **THEN** la red aparece con esos datos sin conexión Wi-Fi del nodo

### Requirement: Objetivo explícito

Escudo SHALL seleccionar y cambiar objetivo por SSID/BSSID observado y rechazar selecciones inconsistentes.

#### Scenario: Objetivo explícito verificable

- **WHEN** se envía una selección válida y luego un BSSID no observado
- **THEN** se confirma la primera y se rechaza la segunda sin perder el objetivo válido

### Requirement: Control durante descubrimiento

El sistema SHALL permitir consultar estado y detener el modo durante un escaneo sin iniciar MQTT.

#### Scenario: Control durante descubrimiento verificable

- **WHEN** la app pide salir mientras escanea
- **THEN** el scan termina o se cancela y el modo libera radio sin publicar MQTT
