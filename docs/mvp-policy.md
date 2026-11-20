# Acuerdos vigentes del MVP — 17 de noviembre de 2026

Este documento resume las últimas decisiones confirmadas por el usuario. Se aplica junto con arquitectura, decisiones técnicas, contratos y specs principales. Los informes antiguos y cambios archivados describen su versión, no requisitos de instalación actuales.

## Confirmado: comunicación y alcance

| Componente | Acuerdo vigente |
| --- | --- |
| MQTT ESP32 | MQTT5/TCP, mqtt://IP-DEL-PC:1883, wolfMQTT 2.2.0 y NetworkClient oficial. Sin TLS, usuario, contraseña MQTT, roles ni ACL. |
| Broker/app/notificaciones | Mosquitto Docker, TCP1883 y WS9001 anónimos; ws://IP-DEL-PC:9001/mqtt desde Android. Sin generación de cuentas/CA, selector operador/observador ni rol notify. |
| Configuración del nodo | configure_connectivity v1/v2 recibe únicamente ssid, wifi_password y mqtt_uri. NVS versión3; formatos anteriores se rechazan y requieren reaprovisionamiento BLE o carga USB limpia permitida. No borrar NVS automáticamente. |
| BLE | Ventana física 120 s, botón 3 s, SC/MITM/cifrado y bonds persistentes de hasta tres teléfonos. Conservar permisos Android necesarios. |
| Pruebas sin OLED | Opción SENTINEL_SERIAL_TEST_DISPLAY=1, UART0/115200/USB CDC On Boot Disabled. Por defecto0. Estado y código BLE temporal local, sin fingir OLED presente. Botón obligatorio; no guardar passkey en evidencia. |
| App P1 | Base v2 y pantalla Escudo entregadas por software; aceptación física pendiente. Panel temporal P1 retirado por petición del usuario. P4 reutiliza la base para las demás pantallas. |
| Sanitizador y alertas | HTTPS/DoH siguen con validación TLS. Credenciales Wi-Fi y claves propias de proveedores/webhooks/push siguen siendo necesarias y privadas. No hay login o roles MQTT nuevos en esos servicios. |

**Riesgo aceptado:** cualquier cliente que alcance el broker puede leer/publicar todos los topics, suplantar telemetría y enviar órdenes Cebo. ID de nodo, topic, schema, boot/revisión/TTL y deduplicación no autentican al emisor. Usar LAN propia/autorizada de confianza y no exponer MQTT a Internet. El aislamiento Wi-Fi/firewall puede impedir conexión; no se promete eludirlo.

Las garantías de persistencia durante uso/reinicio/corte se mantienen. USB puede reinicializar datos. Conservar volumen MQTT y no confundirlo con historial del dispositivo. El usuario Linux de Mosquitto y permisos del sistema de archivos no son cuentas/roles de la aplicación.

## Reparto actualizado: un autor por spec

| Persona | Entrega propia y siguiente trabajo |
| --- | --- |
| P1 — Jefry | Contratos, API/núcleo y Escudo implementados por software; app común v2, diagnóstico sin OLED y correcciones memoria/pila BLE integrados. Coordina OpenSpec y esta simplificación transversal. Falta comprobar físicamente la imagen final BLE (tarea4.3), los recorridos pendientes de P1 e integración de tres modos con alertas OLED cuando estén P2/P3/P4. |
| P2 | Auditor/Sanitizador completo: auditoría → pasarela/NAT → DNS → DoH, configuración persistente y pruebas con cliente real. No implementar cuentas MQTT ni cambiar la API compartida sin coordinar con P1. |
| P3 | Cebo HTTP/Telnet → detección/historial persistente → webhook → push servidor. Suscribir MQTT anónimo por topics contratados; sin cuenta/rol notify ni ACL a pedir a P4. Conservar claves propias de proveedores, privacidad, dedup y recepción push real como requisito. |
| P4 | Pantallas finales Escudo/Sanitizador/Cebo, gráficos/incidentología, recepción push Android, verificación física LAN y laboratorio/simulador. Consume transporte, controller/store/journal y diagnóstico entregados; no rehacer contratos, cuentas/ACL/TLS MQTT o BLE. Coordina cambios comunes con P1. |

Las fichas conservan un responsable y sus números de tarea. Las tareas retiradas se sustituyen por la validación de integración correspondiente, no se presentan como funcionalidades implementadas. La cuenta de23 encargos original es histórica; consultar tareas actuales y evidencia, no asumir que todos siguen por empezar.

## Pendiente de validar y de implementar

- P1 conserva la tarea física4.3 de pairing/estado/ACK/estabilidad básica desde Android. La imagen intermedia que mostró código no acredita la corrección final.
- OLED física y alertas recientes no se aprueban por usar monitor serie.
- Cebo, Sanitizador, UX final y entrega push con app cerrada siguen en sus specs. El diagnóstico P1 no completa esas pantallas.
- NAT/DoH, coexistencia Wi-Fi/BLE, cortes normales y ensayos RF autorizados necesitan su evidencia real; no se simulan como éxitos físicos.
- Cada autor compila, hace una comprobación funcional breve y entrega evidencia. No se exigen benchmarks ni campañas extensas previas.

## Lecturas y uso diario

[Reparto y fichas](encargos/README.md), [guías de cada persona](encargos/guia-equipo.md), [pruebas P1](guia-pruebas-persona-1.md), [firmware](firmware.md), [broker](../broker/README.md), [app](../app/README.md), [contratos](../contracts/module-protocol.md), [persistencia](persistence-policy.md).

La integración va a develop mediante PR; main conserva la versión estable. Los secretos propios, dependencias instaladas y artefactos compilados no se versionan. No archivar ni marcar pruebas físicas por el hecho de publicar una rama.
