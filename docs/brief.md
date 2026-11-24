

# Sentinel Node

Sistema de seguridad de redes basado en ESP32.

## Componentes

- Firmware ESP32: C/C++
- Broker: Eclipse Mosquitto en Docker.
- Aplicación: React, Vite, Tailwind CSS, Zustand y Capacitor.
- Comunicación online: MQTT con mqtt.js.
- Configuración offline: BLE, @capacitor-community/bluetooth-le u otra alternativa moderna.

## Modos

1. Escudo Pasivo: monitoreo de tramas Wi-Fi y alertas.
2. Auditor y Sanitizador: auditoría de red, STA+AP, NAT y filtrado DNS.
3. Cebo: honeypot local con servicios simulados y alertas.

Solo un modo puede estar activo a la vez.

## Requisitos generales

- Máquina de estados para gestionar los modos.
- Telemetría y comandos con contratos compartidos.
- Dashboard de estado, métricas e incidentes.
- Funcionamiento degradado cuando no hay conexión.
- Pruebas de integración y de hardware.
- Desarrollo incremental: simular antes de integrar hardware.

## Restricciones

- No asumir características no verificadas del ESP32.
- Documentar los límites reales de rendimiento.
- Probar actividades de seguridad únicamente en redes autorizadas.
- No almacenar secretos ni credenciales reales en el repositorio.
