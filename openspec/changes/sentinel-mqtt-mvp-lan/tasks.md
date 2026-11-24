# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## 1. Contratos y configuración

- [x] 1.1 Cambiar configure_connectivity v1/v2 a mqtt:// sin CA; regenerar tipos/validador y documentar contrato; verificar fixtures y pruebas de contratos.
- [x] 1.2 Adaptar validación/persistencia del nodo y conectar sin UTC; probar rechazo de formato anterior, guardado, reinicio y errores NVS; actualizar guía.

## 2. Transportes y acceso móvil

- [x] 2.1 Sustituir TLS por NetworkClient oficial y retirar ESP_SSLClient exclusivo de MQTT; verificar worker real con wolfMQTT y compilación Arduino; actualizar dependencias/guías.
- [x] 2.2 Configurar broker TCP/WS en LAN sin certificados conservando cuentas/ACL/volumen; actualizar scripts y guía; comprobar flujos reales, rechazos y reinicio.
- [x] 2.3 Ajustar app/Android/formularios a WS/IP sin CA y conservar BLE/HTTPS; verificar pruebas app, navegador, sync y compilación APK; documentar instalación y acceso LAN.

## 3. Coherencia y comprobación conjunta

- [x] 3.1 Actualizar specs principales, decisiones, roadmap y planificación vigente; identificar evidencias históricas; comprobar enlaces y ausencia de instrucciones TLS obsoletas en flujo vigente.
- [x] 3.2 Revisar diff/secretos y ejecutar comprobaciones conjuntas; registrar evidencia y límites físicos sin afirmar pruebas de teléfono/placa no realizadas.
