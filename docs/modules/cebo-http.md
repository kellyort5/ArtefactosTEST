# Cebo: panel HTTP ficticio

Responsable: Persona 3. Cambio: [sentinel-cebo-http](../../openspec/changes/sentinel-cebo-http/proposal.md) ([ficha](../encargos/sentinel-cebo-http.md), [diseño](../../openspec/changes/sentinel-cebo-http/design.md), [tareas](../../openspec/changes/sentinel-cebo-http/tasks.md)). Rama: `feat/sentinel-cebo-http`. Se integra con la [API de módulos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) y el [contrato v2](../../contracts/module-protocol.md).

El firmware añade `firmware/SentinelNode/src/cebo/`. El servicio Telnet se documenta en [cebo-telnet.md](cebo-telnet.md). La clasificación por ventanas, `cebo.stats`, `cebo.configure` y la persistencia del historial pertenecen al cambio de [eventos e historial](../encargos/sentinel-cebo-eventos-historial.md). Cebo **no** habilita `SENTINEL_MODULE_CEBO` en `SentinelConfig.h`; ver [cómo activarlo](#activar-y-compilar).

## Qué hace y qué no hace

El ESP32, en modo Cebo y con IPv4 en la LAN autorizada, sirve un panel de administración **ficticio** en el puerto 80 con `esp_http_server` oficial. Cada conexión, acceso e intento de autenticación se convierte en una interacción con metadatos mínimos. No hay administración real, no se ejecuta nada de lo recibido, no se cierra sesión ni se emiten cookies, y el panel no debe exponerse a Internet ni mediante puertos del router.

## Rutas y respuestas

Todas las respuestas son fijas: ningún dato del cliente se refleja. Llevan `Cache-Control: no-store`, `X-Content-Type-Options: nosniff` y una CSP restrictiva.

| Petición | Respuesta | Interacción registrada |
| --- | --- | --- |
| `GET`/`HEAD` `/`, `/admin`, `/login` | 200, formulario ficticio (`action="/login"`) | `access` con esa ruta |
| `GET` `/robots.txt` | 200, `Disallow: /admin` | `access` `/robots.txt` |
| `GET` `/favicon.ico` | 404 | `access` `/favicon.ico` |
| `GET` otra ruta | 404 | `access` `/other` |
| `POST` `/login` con cuerpo ≤ 1024 B | 200, mismo formulario con «Usuario o clave incorrectos.» | `auth_attempt` `/login` |
| `POST` `/login` con cuerpo > 1024 B | 413 y cierre, sin leer el cuerpo | `auth_attempt` `/login` |
| `POST` `/login` con cliente lento o roto | 200 fijo y cierre | `auth_attempt` `/login` (una vez) |
| `POST` en otra ruta, o cualquier otro método | 405 con `Allow` y cierre, sin leer el cuerpo | `access` con la ruta canónica |
| Conexión nueva / cierre | — | `connect` / `disconnect` (solo por el bus) |

La ruta canónica descarta consulta y fragmento antes de comparar y distingue mayúsculas. No se decodifica el porcentaje: `/%6Cogin` es `/other`. Las URI de más de 512 bytes y las cabeceras de más de 1024 bytes las rechaza el servidor oficial (414/431) antes de llegar al manejador, y solo dejan la interacción de conexión. Los orígenes que no sean IPv4 (o IPv6 con IPv4 mapeado) se rechazan, porque el contrato v2 no tiene otra representación de origen.

## Datos que se registran y datos que se descartan

Una interacción contiene solo: IPv4 y puerto de origen, puerto de destino (80), protocolo, tipo (`connect`, `access`, `auth_attempt`, `disconnect`), ruta canónica de la lista cerrada o `null`, motivo de cierre y marca de tiempo monotónica. La estructura **no tiene ningún campo** capaz de contener usuario, contraseña, cookie, cabecera `Authorization`, consulta ni cuerpo.

El cuerpo de un formulario se lee en un buffer de 128 bytes, nunca se interpreta, se borra con escritura volátil al terminar y no sale de esa función. No se leen cabeceras del cliente. La URI recibida no se conserva. La prueba de host usa un marcador ficticio como contraseña, consulta y fragmento, y comprueba que no aparece en eventos, respuestas ni en la memoria del bus.

## Límites

Valores iniciales propuestos en el diseño; son cotas de implementación, no mediciones de capacidad. Se ajustan ante fallos, sin retirar funciones.

| Límite | Valor | Efecto |
| --- | --- | --- |
| Conexiones simultáneas | 4 (`HttpMaxSockets`) | El exceso se rechaza; no se desplaza a las existentes |
| Tiempo de `recv`/`send` | 5 s | Lo aplica `esp_http_server` |
| Cuerpo procesable | 1024 B | Mayor: 413 y cierre |
| Presupuesto para vaciar un cuerpo | 3 s en total | Un cliente lento no ocupa el hilo del servidor |
| Sesión sin petición completa | 5 s (`IdleTimeoutMs`, compartido con Telnet) | `esp_http_server` no cierra conexiones inactivas por sí solo; el módulo las cierra |
| Cola de interacciones | 16 | Al llenarse se pierde la más reciente y se cuenta |
| Eventos por `tick` | 4 | Evita saturar la salida del núcleo |
| Reintento de arranque fallido | 5 s | El estado HTTP pasa a `error` entre intentos |

`esp_http_server` usa una sola tarea: un cliente lento puede retrasar a otros hasta 5 s por lectura. Es un límite conocido de esta elección.

## Eventos v2

Se publica `cebo.interaction` mediante `ModuleContext::emit_event`, el hook de la API; el módulo no escribe en el journal ni en MQTT. Hasta que el cambio de eventos e historial añada la clasificación, cada interacción de acceso o autenticación es **aislada**: `count: 1`, `window_ms: 1000`, `suspicion: "informational"`, severidad `info`, `origin: "real"`.

```json
{"source_ipv4":"192.0.2.2","source_port":45000,"destination_port":80,"protocol":"http","interaction":"auth_attempt","path":"/login","count":1,"window_ms":1000,"suspicion":"informational","evidence":["auth_attempt"]}
```

Evidencia: `canonical_access` (acceso), `auth_attempt`, `connection_observed` (conexión), `peer_closed` o `idle_timeout` (cierre). Las conexiones y cierres no generan evento todavía: cada petición ya escribe una entrada en el journal y publicarlas multiplicaría el desgaste del anillo. Esta mezcla es transitoria; el siguiente cambio agrega y suprime repeticiones.

La salida está limitada: si el núcleo no acepta un evento, se conserva y se reintenta; si pasan 2 s sin aceptarlo se descarta y se cuenta, para no bloquear la cola.

## Ciclo de vida y estado

- `init` no toma la radio ni abre sockets. `start` lo llama el supervisor tras conceder la radio; el servicio HTTP espera a tener IPv4 (lee `WIFI_STA_DEF` en modo solo lectura) y entonces abre el listener. Si la LAN se pierde, cierra y reabre al volver.
- `request_stop` cierra la cola, las sesiones y el listener (`httpd_stop`). `stopped()` solo es verdadero cuando no queda listener ni socket; un cierre fallido se reintenta en cada `tick` dentro del plazo del supervisor. Tras salir del modo, el puerto 80 queda cerrado.
- `cebo.stop` detiene los dos servicios sin salir del modo y `cebo.start` los reanuda. `cebo.configure` y `cebo.stats` devuelven `not_implemented` hasta sus cambios.
- Estado: `{"kind":"cebo","http":{"available":true,"status":"running"},"telnet":{"available":true,"status":"running"},"active_connections":N,"max_connections":6,"idle_timeout_ms":5000}`, con `honeypot_http` y `honeypot_telnet` verdaderos. HTTP y Telnet arrancan, fallan y se recuperan por separado.

### Puntos para coordinar con P1

1. El gate del núcleo (`module_runtime.cpp`, operación `cebo.start`) exige `honeypot_http` **y** `honeypot_telnet`; con ambos servicios ya lo pasa. Los servicios siguen arrancando al entrar en el modo Cebo (`AutoStartServices`), que ya es una orden explícita, porque la app aún no tiene el control de Cebo. Cuando lo tenga, basta cambiar esa constante para que solo arranquen con `cebo.start`.
2. `ModuleResources::sta_netif()` solo devuelve el handle en Sanitizador, así que Cebo lee `esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")`. Si P1 prefiere concederlo en Cebo, es un cambio acotado en `network.h`.
3. `SentinelConfig.h` es de P1; este cambio no activa el flag.

## Activar y compilar

Mientras no haya validación física, cada integrante que pruebe Cebo activa el módulo en su copia local, igual que el display serie de pruebas: en `firmware\SentinelNode\SentinelConfig.h`, cambia solo `#define SENTINEL_MODULE_CEBO 0` a `1`. No lo incluyas en el PR. Después compila el sketch con **Verificar** y la [configuración oficial del firmware](../firmware.md).

Pruebas de host (solo un compilador C++17, sin cJSON ni OpenSSL):

```sh
node firmware/test/cebo/run_host.mjs
```

Comprueban rutas, límites, respuestas fijas, ausencia del marcador secreto, el formato de `details` contra los fixtures v2 compartidos y el ciclo de vida con dobles. No sustituyen la compilación con Arduino IDE ni las pruebas en LAN.

Con las dependencias del workspace instaladas, este script pasa los eventos y estados que produce el código C++ por el decodificador v2 compartido (AJV) y comprueba que rechaza un campo extra o una ruta con consulta:

```sh
npm run build -w contracts
node firmware/test/cebo/check_contracts.mjs
```

## Comprobación física pendiente (tareas 2.1 y 2.2)

Requiere ESP32, LAN propia/autorizada y una copia con el flag activado. Sustituye `IP` por la dirección de la placa. Registra PASS/FAIL/PENDIENTE y el commit grabado.

1. Provisionar Wi-Fi por BLE y cambiar a modo Cebo (`set_mode` `cebo`). Esperado: `module.http.status` pasa a `running`.
2. `curl.exe -i http://IP/` devuelve 200 con el formulario.
3. Marcador ficticio: `curl.exe -i -d "user=admin&pass=MARCADOR-DEMO-123" "http://IP/login?token=MARCADOR-DEMO-123"` devuelve 200 sin eco. Después, en monitor serie, eventos y respuestas, el marcador no aparece.
4. Se reciben eventos `cebo.interaction` con `access` y `auth_attempt`, IP de origen y puerto 80.
5. Límite: abrir seis conexiones TCP sin enviar nada y pedir `/` en cada una antes de 5 s. Esperado: como máximo cuatro respuestas.
6. Inactividad: abrir una conexión y esperar más de 5 s. Esperado: el servidor la cierra.
7. Salir del modo (`set_mode` `idle`) o enviar `cebo.stop`: `Test-NetConnection IP -Port 80` debe informar `TcpTestSucceeded : False`.
8. No abrir puertos del router ni exponer la placa a Internet.

## Estado de verificación

| Comprobación | Resultado |
| --- | --- |
| Pruebas de host de Cebo | PASS (Windows, MinGW; sin radio) |
| Eventos y estado del código C++ frente al decodificador v2 compartido | PASS (paquete de contratos instalado en una carpeta temporal; host) |
| Compilación del módulo con core oficial ESP32 3.3.12, ESP32S3 (sketch aislado con `esp_http_server`) | PASS |
| Verificar del sketch completo con el flag activado | PENDIENTE |
| 2.1 HTTP real y eventos desde teléfono/PC en LAN | PENDIENTE |
| 2.2 Puerto 80 cerrado al detener Cebo o cambiar de modo | PENDIENTE |
