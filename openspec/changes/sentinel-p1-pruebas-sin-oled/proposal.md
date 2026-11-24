# Proposal

> Actualización 2026-11-20: el panel temporal P1 fue retirado por petición del usuario en `sentinel-app-escudo`. Sus tareas y resultados siguientes son evidencia histórica, no instrucciones para reinstalarlo. Se retiró su delta mobile-control para que un archivado posterior no lo restaure. La salida serie, las correcciones BLE y los pendientes físicos de firmware se conservan.

## Why

Persona1 necesita ejecutar su guía física sin disponer todavía de OLED. El firmware bloquea nuevos vínculos sin pantalla y la app común no ofrece las órdenes específicas necesarias para las pruebas del Escudo. El usuario autoriza salida serie temporal y un panel diagnóstico BLE.

## What Changes

- Modo de visualización serie explícito y desactivado por defecto, para estado y código BLE temporal con ventana física; mantener la OLED como hardware real ausente.
- Panel P1 optativo en Android sobre transporte/contratos existentes: escanear, paginar catálogo, seleccionar, capturar, recuperar eventos y observar ACK/estado/métricas reales. Casos negativos públicos controlados y reenvío explícito para pruebas.
- Instrumentación serie acotada del paso real a detectores, sin tramas crudas ni alertas simuladas.
- Guía completa con activación, comandos, APK y matriz T01–T15; OLED física, ataques en laboratorio y módulos pendientes no se declaran aprobados.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `device-runtime`: visualización serie de pruebas y diagnóstico coherente sin simular OLED.
- `device-connectivity`: excepción explícita local para mostrar passkey temporal por serie en modo de pruebas, sin cambiar seguridad BLE.
- El delta histórico `mobile-control` del panel fue sustituido por su retirada en `sentinel-app-escudo`; no se vuelve a sincronizar desde este cambio.

## Impact

Firmware, app de diagnóstico, pruebas focalizadas y guía P1. Sin nuevos proveedores, SDK, protocolos ni dependencias. No cambiar MQTT: otra tarea lo está modificando en un checkout distinto. La excepción de passkey solo aplica a visualización local de pruebas: nunca contraseñas, material de bonds, MQTT, historial, logs persistentes o evidencia pública. No eliminar OLED del producto ni el botón de autorización. Sin commits/push/merge ni pruebas activas en redes ajenas.
