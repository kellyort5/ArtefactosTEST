# Verificación de sentinel-firmware-module-api

Fecha: 2026-11-11. Rama: `feat/sentinel-firmware-module-api`, creada desde `origin/develop` en `7be9042`. Responsable: Persona 1.

## Evidencia de software

- Consumidor mínimo compilado incluyendo solo `module_api.hpp`, sin dependencia del runtime. Registro de módulos opcionales deshabilitado por defecto.
- Pruebas focalizadas de supervisor y gate: exclusión, parada a 5000 ms, destino sin arrancar al fallar limpieza, error visible en estado, autorización antes de caché, expiración, revisión, ID en conflicto, capacidad ausente y rotación de perfiles registrados.
- Concesiones de recursos: un módulo inactivo no usa radio; NAPT solo se solicita por la concesión de Sanitizador y se libera al detener la red.
- Salidas acotadas: rechazo de detalles ajenos al schema, cola de ocho entradas con pérdida contabilizada, validación de métricas y eventos, conservación de `origin` y recuperación del journal con registros v1/v2.
- `PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH SENTINEL_SANITIZE=1 python3 firmware/test/check_contracts.py`: PASS. Incluye host con AddressSanitizer/UndefinedBehaviorSanitizer, regresiones v1, 69 fixtures v2 y salidas del serializador C++ aceptadas por el codec AJV compartido.
- `pio run -d firmware` con PlatformIO espressif32 6.13.0 y ESP-IDF 5.5.3: PASS. Configuración ESP32-S3 N16R8, sin componentes funcionales ni fixtures de host enlazados.
- `python3 firmware/test/check_config.py`: PASS. Configuración efectiva de seguridad, memoria, periféricos, `CONFIG_LWIP_IP_FORWARD` y `CONFIG_LWIP_IPV4_NAPT`; particiones dentro de los 16 MiB configurados.
- Inspección de fuentes: `esp_wifi_init/start/stop` y `esp_netif_napt_enable/disable` solo aparecen en `network.cpp`. El gestor conserva los netifs y desactiva captura/NAPT/DHCP al liberar la red.
- Documentación: enlaces de `docs/development.md` y `git diff --check` revisados.

Estos resultados prueban lógica y compilación. No acreditan capacidad física de memoria, concurrencia Wi-Fi/BLE, comportamiento real de NAPT, pantalla ni funcionamiento de los módulos todavía no implementados.

## Pendiente físico — tarea 2.2

`pio device list` solo mostró puertos serie genéricos de WSL. La enumeración de Windows mostró COM3 y COM4, ambos «Standard Serial over Bluetooth link»; no se identificó una ESP32 accesible. No se ejecutaron upload, emparejamiento ni transiciones físicas. La casilla 2.2 queda pendiente.

Para completarla, seguir [la guía de integración y verificación](../../../docs/development.md#verificar-este-cambio), registrar la placa/puerto y probar Idle y una transición BLE de ida/vuelta con un consumidor de prueba registrado (sin anunciar servicios completos) o un módulo real. Comprobar ausencia de servicios residuales y registrar ACK/revisión/OLED/botón. No considerar pruebas de host, una simulación ni una aceptación de supuestos como evidencia física. Un consumidor de prueba en la placa solo acredita el recorrido básico de la API, no funcionalidades de detección ni servicios del producto.

## Integración con el equipo

Los componentes futuros conservan las carpetas asignadas `escudo`, `sanitizador` y `cebo`. Cada autor exporta su factory y habilita su opción al integrar una implementación real. La app necesita el adaptador v2 de su encargo: esta rama no modifica la app v1 para ocultar la migración del protocolo. Antes de integrar el PR se requiere revisión de otro integrante; Personas 2 y 3 deben revisar el contrato público que van a consumir. Este cambio no se archiva ni se fusiona automáticamente.
