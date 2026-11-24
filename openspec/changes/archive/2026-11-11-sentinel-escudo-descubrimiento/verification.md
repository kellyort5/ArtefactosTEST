# Evidencia de sentinel-escudo-descubrimiento

Fecha: 2026-11-11. Responsable: Persona 1. Rama: `feat/sentinel-escudo-descubrimiento`, creada desde `origin/develop` en `759f9cd` (incluye PR #6).

## Software verificado

- Componente `escudo` compilado y enlazado mediante su factory y `CONFIG_SENTINEL_MODULE_ESCUDO=y`. Declara descubrimiento/objetivo mediante el subestado v2; mantiene `capture` y `detectors` falsos.
- `pio run -d firmware`: PASS con ESP-IDF 5.5.3, PlatformIO espressif32 6.13.0 y configuración ESP32-S3 N16R8. La compilación incluye la lectura de estado/total de scan y el método de lectura que separa error de catálogo vacío.
- `python3 firmware/test/check_config.py`: PASS; configuración efectiva y particiones de la base. Inspección adicional: opción de Escudo habilitada; drivers y NAPT conservan dueño único en `network.cpp`.
- `PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py`: PASS. Compila el componente real con supervisor/gate/wire y ASan/UBSan. Cubre SSID oculto/binario, duplicado BSSID, overflow a 32, cursores emitidos/obsoletos, límites de página/sobre, selección válida e inválida sin pérdida del objetivo, ACK y rechazo de captura ausente, fallos SDK, timeout, avisos acotados, Idle y scan posterior.
- Mensajes producidos por el componente y sink de host: validación con `decodeModuleMessage` y `checkModuleAckForCommand`. Los ejemplos de requests de la guía se cotejan con `makeModuleCommand`. Los eventos del sink de host se marcan `simulated`; el firmware de producto emite los eventos de radio real sin ese reemplazo de pruebas.
- `python3 firmware/test/check_contracts.py` (Node24): regresiones del núcleo, fixtures v1/v2 y wire de la API conservados. No se modificaron app, broker, contratos ni módulos de otros integrantes.
- Doc: [operaciones, límites y recorrido](../../../../docs/modules/escudo-discovery.md). El ajuste compartido del gestor/API y activación global es necesario para integrar la tarea 1.1 y está descrito en el diseño.

La inspección de fuentes confirma que Escudo solicita `scan_passive()` por su concesión; no llama a `esp_wifi_connect`, no inicia drivers, no publica MQTT y no escribe NVS. Esto es evidencia de implementación, no una observación de tráfico de la placa.

## Pruebas físicas pendientes

El usuario pidió continuar conservándolas pendientes. **No se han ejecutado upload, BLE, recepción Wi-Fi ni comprobaciones de tráfico/recursos reales de este incremento.**

- **2.1:** compilación realizada; falta catálogo y selección por BLE en ESP32 con LAN propia, datos observados y evidencia de ausencia de asociación/IP/MQTT.
- **2.2:** salida a Idle durante scan y repetición cubiertas por host; falta ejecutar y registrar ese recorrido en el hardware real.

No cerrar estas casillas con mocks, una interfaz, compilación o suposiciones. La [API archivada](../2026-11-11-sentinel-firmware-module-api/closure.md) conserva su propia tarea física 2.2 abierta; el archivo aceptado no la convierte en prueba realizada.

## Alcance conservado

Este incremento no implementa captura promiscua, detectores, reconexión/historial específico de Escudo ni UI v2. Los respectivos encargos siguen completos y pendientes. PMF, SSID no expuesto por el SDK y hora individual de Beacon se documentan como no observables por este scan; no se fabrican. Se requiere revisión de otro integrante antes de integrar el PR.
