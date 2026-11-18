# Design

**Acuerdos vigentes:** [alcance y reparto](../../../docs/mvp-policy.md) y [persistencia](../../../docs/persistence-policy.md). MQTT anónimo TCP/WS; BLE protegido y HTTPS/DoH conservan sus requisitos. Identidad, bonds, configuración aceptada e historial confirmado sobreviven a reinicio/corte normal; USB puede reinicializarlos. Los pendientes físicos requieren evidencia propia.

## Context

P4 consume la base broker TCP1883/WS9001 anónima entregada por P1. El usuario retiró TLS/cuentas/roles/ACL. Ver docs/mvp-policy.md y broker/README.md. No reimplementar preparación de cuentas ni certificados.

## Goals / Non-Goals

Comprobar despliegue en LAN del equipo, v1/v2 y consumidor real P3. No acreditar radio por mocks ni añadir login, rol notify, ACL o cambios al router.

## Decisions

Reutilizar imagen/Compose/volumen y helper Windows/WSL existentes. Usar IP del PC y verificar firewall/aislamiento de clientes. P3 suscribe los topics acordados sin cuenta y valida/deduplica eventos; esa validación no demuestra autenticidad del emisor. P4 documenta resultados reales y endpoints para sus compañeros. Mantener transporte de eventos históricos v1 y no exponer HTTP/Telnet.

## Risks / Trade-offs

Broker sin autenticación/cifrado: cualquier cliente con alcance puede leer/publicar. Uso solo en LAN propia/autorizada de confianza sin exposición Internet. El software no elimina aislamiento de Wi-Fi.

## Migration Plan

Partir de develop integrado, configurar endpoint/topología local, repetir smoke y comprobar teléfono/ESP32/P3 cuando existan sus componentes. Conservar volumen y evidencias; no generar ni rotar cuentas. No cerrar pruebas físicas por el smoke del PC.
