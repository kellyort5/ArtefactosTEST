# Evidencia de sentinel-escudo-captura

Fecha: 2026-11-11. Persona 1. Rama: `feat/sentinel-escudo-captura`, creada desde `origin/develop` en `ce7e16b`, que incorpora PR #7. El archivo previo de descubrimiento conserva 4/6 tareas y sus pruebas físicas abiertas por autorización del usuario.

## Software verificado

- Firmware compilado con PlatformIO espressif32 6.13.0 / ESP-IDF 5.5.3 para la configuración ESP32-S3 N16R8. Se compilan y enlazan `capture.cpp`, `frame_parser.cpp` y `channel_scheduler.cpp`, no solo sus headers. La primera compilación reutilizó un listado CMake antiguo y falló por fuentes ausentes; se corrigió el registro/dependencia del listado de fuentes y el build posterior terminó correctamente.
- `python3 firmware/test/check_config.py`: PASS. Configuración de memoria/seguridad/periféricos/forwarding/NAPT de la base conservada. Esto no detecta memoria física ni activa una pasarela.
- `PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py`: PASS. Reutiliza el componente, supervisor, gate y wire reales con recursos de host; cubre ingreso acotado, saturación, generaciones cerradas, Beacon/Deauth/Disassoc/Probe, IE/RSN válidos y truncados, WPA/PMF/cuerpo protegido, scheduler por país, cobertura/pausas, start/stop, rescan, cambio de objetivo y parada fallida sin activar otro modo.
- Los dos recorridos producen 70 y 72 mensajes respectivamente, contrastados con `decodeModuleMessage` y `checkModuleAckForCommand`. Los 11 requests de las dos guías se cotejan con `makeModuleCommand`. Los sinks de host marcan sus eventos como `simulated`; el componente de firmware consume radio real y no tiene generador de observaciones ficticias.
- `PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH python3 firmware/test/check_contracts.py`: PASS. Conserva las regresiones del núcleo/API, fixtures v1 y 69 fixtures v2; serialización C++ validada por el codec compartido.
- El callback SDK solo adapta FCS/estado y llama a copia/cola sin espera. Inspección: no hay JSON, flash, logs ni parser en esa ruta; solo el gestor inicia/detiene el driver. El módulo no llama a `esp_wifi_connect`, no transmite probes y no publica MQTT.
- Los contadores se contrastan con reloj controlado: prioridad 800/200 ms, suma real de tiempo del controlador, pausa sin captura y rechazo de generaciones antiguas. No es una medición de RF, convivencia, carga sostenida o estabilidad de hardware.
- [Guía](../../../docs/modules/escudo-capture.md), enlaces y `git diff --check` revisados. OpenSpec valida el cambio y las diez specs principales.

La API compartida se amplió únicamente para generaciones, descartes y máscara de canales, según las necesidades de la tarea 1.1. Conserva interfaces anteriores y dueño único de radio, sin añadir tecnologías, procesos ni formatos de mensajes.

## Pendiente físico

Se mantienen las comprobaciones físicas pendientes conforme a la estrategia indicada por el usuario. **No se ha grabado ni probado este incremento en una ESP32/teléfono; no se atribuyen al hardware los resultados del host.**

- **2.1:** compilación comprobada; falta observar Beacons reales de objetivo y otra red propia, recibir por BLE canales/contadores y registrar la correlación con las observaciones.
- **2.2:** control/generaciones/parada/reanudación comprobados por host; falta ejecutarlos desde teléfono y observar ausencia real de asociación/probes y recursos residuales.

Seguir el recorrido de la guía y mantener las casillas abiertas hasta disponer de esa evidencia. Los pendientes de API y descubrimiento archivados siguen vigentes, sin completarse por suposición.

## Alcance y entrega

La captura queda disponible; los detectores siguen deshabilitados hasta el siguiente encargo. No se implementa app ni se modifican contratos, broker, Sanitizador o Cebo. La recuperación específica, compatibilidad extensa y rendimiento no se presentan como validados. El usuario autorizó commits y push y reservó la creación del PR: no se crea ni fusiona un PR automáticamente.
