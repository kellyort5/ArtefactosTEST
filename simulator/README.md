# Límite del simulador

Este directorio reserva un apoyo futuro para reproducir mensajes de los [contratos v1](../contracts/README.md). No contiene un nodo simulado ni un sustituto de firmware, radio, pairing, OLED, botón o broker.

Las comprobaciones pequeñas de codec/framing y fixtures viven en `contracts/`; las de store/registro en `app/test/`. Una extensión futura debe validar los mismos esquemas y declarar expresamente que sus datos son de prueba. No iniciar captura, honeypot, notificaciones o NAT en esta base ni reclamar evidencia de hardware a partir de fixtures.

El desarrollo real y los comandos de preparación están en [docs/development.md](../docs/development.md). Un simulador completo no es prerrequisito para integrar app y ESP32.
