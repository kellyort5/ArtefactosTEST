# Escudo: detectores y evidencia pasiva

Responsable: Persona 1. Cambio: [sentinel-escudo-detectores](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/proposal.md). Consume la [captura real](escudo-capture.md), el parser y el supervisor existentes. Las cuatro familias están conectadas al mismo flujo del firmware; las pruebas de host usan fixtures y no acreditan recepción física.

## Activación e integración

Seguir [descubrimiento y selección](escudo-discovery.md): entrar en Escudo, realizar escaneo pasivo y elegir una tupla real. Iniciar captura con el cliente BLE v2 autorizado:

```json
{"operation":"escudo.capture_start","args":{}}
```

No hay orden adicional para activar los detectores. `capabilities.detectors=true` indica que están implementados; el procesamiento solo ocurre durante captura activa con objetivo. Un inicio detenido, una captura con error o ausencia de objetivo no producen observaciones ficticias. `get_state` muestra el subestado y cobertura:

```json
{"operation":"get_state","args":{}}
```

El flujo es: callback SDK → cola compartida → parser → observación válida de la generación vigente → detectores → `ModuleContext.emit_event` → cola/journal/identidad/tiempo/BLE comunes. El callback no ejecuta detección, JSON, almacenamiento ni transporte. El observador opcional `Escudo::observe` sigue disponible y no sustituye los detectores internos. No hay tarea, driver, journal ni instancia MQTT adicional. No se asocia ni transmite pruebas Wi-Fi.

## Heurísticas iniciales

Estos valores concretan el diseño, son ajustables y **no representan precisión medida**. Todos los resultados son indicios con sospecha `medium`, severidad `warning` y evidencia `passive_only`; ninguno identifica a un atacante.

| Familia | Condición inicial | Evidencia y límite |
| --- | --- | --- |
| `deauth_burst` | ≥10 Deauthentication del BSSID seleccionado en los últimos 1000 ms. | `deauth_rate`; tramas observadas, no desconexiones verificadas. |
| `suspicious_disconnect` | ≥5 Deauthentication/Disassociation del BSSID seleccionado en 5000 ms. | `disconnect_sequence` y `disassoc_rate` si se observó Disassociation; en otro caso `deauth_rate`. No demuestra pérdida de conectividad del cliente. |
| `evil_twin` | Otro BSSID anuncia el mismo SSID y presenta degradación de auth/cifrado/PMF, **o** canal diferente **junto con** diferencia absoluta de RSSI ≥20 dB. | `same_ssid`, `different_bssid` y solo las diferencias observadas. SSID, canal o RSSI aislados no disparan el indicio. Comparación con referencia de hasta 30000 ms. |
| `karma_indication` | Un BSSID responde a ≥3 SSID no vacíos distintos en 10000 ms; cada respuesta tiene solicitud compatible observada en ≤2000 ms, del mismo cliente/SSID/canal. Al menos una respuesta incluye el SSID seleccionado o procede del BSSID seleccionado. | `multiple_ssids`, `passive_only`. Respuestas no correlacionadas, wildcard, cliente/canal distinto o evidencia insuficiente no generan alerta. |

Las ventanas incluyen el instante exactamente en su borde; una observación anterior al último tiempo de su generación se ignora. Deauth/desconexión requieren BSSID seleccionado y que transmisor o destinatario sea ese AP. Se pueden observar cabeceras de tramas protegidas, pero no se descifran ni se valida criptográficamente su emisor o razón. Retransmisiones con bit retry y mismo transmisor/destino/BSSID/subtipo/secuencia observado en el último segundo no vuelven a contarse.

Evil Twin usa la red seleccionada como referencia inicial de escaneo; PMF es `unknown` hasta observar un Beacon/Probe Response válido del objetivo. Una nueva observación del objetivo actualiza su seguridad anunciada, sin autenticarla. La referencia y los candidatos deben anunciar el mismo SSID no vacío; no se cambia silenciosamente el SSID seleccionado. El candidato puede llegar antes de actualizar la referencia: se conserva brevemente y se compara al recibir esta. El canal comparado es el canal recibido, no una deducción sobre cobertura continua.

Para auth personal se compara el mínimo anunciado: open < WEP < WPA/WPA-mixto < WPA2/WPA2-WPA3 < WPA3. Enterprise solo se considera degradado a open/WEP, sin ordenarlo frente a familias personales. Para cifrado: none < WEP < TKIP < CCMP/GCMP; mixed/unknown no prueban degradación. PMF: disabled < capable < required. Valores desconocidos no se completan. Una diferencia puede corresponder a configuración legítima, roaming, nodos mesh, distancia o balanceo; la app debe mostrar evidencia y limitaciones.

En Karma, las tres respuestas pueden dirigirse a uno o varios clientes, pero cada una exige su solicitud observada correspondiente y se consume esa correlación una vez. Un AP autorizado con varias redes puede producir un patrón similar. No se inventan SSID, no se envían solicitudes y no se usa la ausencia de solicitud capturada como prueba de Karma. El campo SSID del incidente es el de la respuesta que completa el patrón; `related_bssid`, cuando existe, identifica el objetivo relacionado.

## Límites, enfriamiento y reinicios

- Ventanas Deauth/desconexión/Disassoc: hasta 64 tiempos cada una. Al saturarse conservan los más recientes: `count` es la cantidad retenida, **un límite inferior** cuando existe más tráfico. No se promete contar todos los paquetes.
- Tablas: 32 observaciones recientes para retry; 16 candidatos del mismo SSID; 16 solicitudes Probe; 8 BSSID con hasta 3 SSID para Karma. Cada tabla sustituye la entrada más antigua al llenarse. Solo conserva metadatos acotados, sin cuerpos crudos ni credenciales.
- Enfriamiento de 30000 ms por familia/BSSID observado, con 16 claves. Se conserva al detener/reanudar, cambiar objetivo o salir/volver al modo durante el mismo boot. Una tabla saturada sustituye la clave más antigua y puede permitir una repetición anterior al enfriamiento de una clave expulsada; no es un historial ilimitado.
- Cambiar objetivo limpia referencias/ventanas pendientes. Toda generación nueva limpia ventanas, retries y correlaciones, conservando selección y referencia reciente. Las tramas de generaciones cerradas son rechazadas por captura antes de llegar al detector. Las pérdidas, pausas y saltos reducen la evidencia; no hay compensación que fabrique tramas.
- Los contadores de alertas son acumulados por arranque, incluso al cambiar de modo o reconectar BLE; tras reiniciar el boot cambian la identidad y la referencia de los contadores.

## Contratos, historial y app

Se conserva [module-protocol v2](../../contracts/module-protocol.md), sin cambiar schemas, UUID ni topics. Cada incidente `escudo.detection` incluye familia, SSID/BSSID, BSSID relacionado cuando corresponde, canal, RSSI, seguridad anunciada, `count`, `window_ms`, sospecha y códigos de evidencia cerrados. Los SSID de incidentes usan siempre `{encoding:"hex",value:...}` para conservar exactamente hasta 32 bytes, incluso NUL; el consumidor debe usar la representación SSID del contrato.

La envolvente común aporta device/boot/message ID, secuencia, journal, origen y tiempo/calidad. En el dispositivo las observaciones de radio generan `origin:"real"`; el sink de pruebas de host usa `origin:"simulated"`. Sin hora sincronizada se mantiene `timestamp:null,time_quality:"unknown"`; la app no debe inventar una fecha. No se emiten tramas crudas por BLE.

`metrics.scope="escudo"` publica los cuatro contadores `values.alerts`. Se incrementan al producir un indicio fuera de enfriamiento, aunque la cola común no lo acepte. En ese caso crece `coverage.dropped_events`: hay evidencia perdida, no entrega garantizada. El detector no bloquea ni crea otra cola/reintentos privados; el enfriamiento limita repeticiones. La cola/journal comunes realizan persistencia/identidad/recuperación; el siguiente incremento de [recuperación](../../openspec/changes/sentinel-escudo-recuperacion/proposal.md) conserva su alcance.

La app de Persona 4 consume incidentes/historial, gráficos y métricas. Debe presentar **indicio**, evidencia, cobertura y límites. Cero alertas no distingue por sí solo tráfico benigno de evidencia insuficiente para Karma: explicar esta limitación junto a la cobertura; no mostrar “red segura” ni “Karma descartado”. No se añade un evento ficticio para indicar falta de evidencia ni un dialecto nuevo del protocolo.

## Comprobación breve

```sh
pio run -d firmware
python3 firmware/test/check_config.py
SENTINEL_SANITIZE=1 python3 firmware/test/escudo/check_contracts.py
python3 firmware/test/check_contracts.py
openspec validate sentinel-escudo-detectores --strict
```

Usar Node 24 según [desarrollo](../development.md) y el entorno PlatformIO fijado. `detectors_test.cpp` comprueba umbrales, expiración, enfriamiento, ventanas/generaciones, duplicados benignos, seguridad/PMF/canal/RSSI y correlaciones Karma positivas/negativas. El recorrido integrado utiliza comandos v2 y fixtures de bytes que pasan por cola, parser y componente reales, y valida eventos de cada familia y métricas con el codec compartido. No transmite paquetes. No sustituye la prueba física.

## Prueba física 2.1 pendiente

El usuario autorizó continuar manteniendo esta comprobación pendiente. No se marca como terminada por compilar ni por pasar fixtures.

1. Seguir [cableado/carga](../firmware.md) en ESP32-S3 N16R8 y usar cliente BLE v2 autorizado con metadata/revisión actuales.
2. Escanear una red propia, seleccionarla e iniciar captura. Conservar build, ACK y estado con `capture_status:"running"` y `capabilities.detectors:true`.
3. Recibir métricas BLE, observar crecimiento de Beacons benignos y cobertura. Confirmar con depurador en `Detectors::ingest` que las observaciones reales del parser alcanzan los detectores; registrar evidencia sin bloquear prolongadamente el supervisor ni confundir una pausa de depuración con rendimiento normal.
4. Detener/reanudar desde BLE; comprobar que no se mezclan generaciones. No se requiere emitir ataques para esta comprobación: los casos sospechosos están cubiertos por pruebas de lógica; el ensayo real verifica el flujo de captura benigno y BLE.
5. Conservar resultados en [verification.md](../../openspec/changes/archive/2026-11-12-sentinel-escudo-detectores/verification.md). Siguen abiertos los pendientes físicos de captura, descubrimiento y API. El laboratorio autorizado y la integración con app completa corresponden a sus encargos.
