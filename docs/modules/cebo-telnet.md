# Cebo: Telnet simulado

Responsable: Persona 3. Cambio: [sentinel-cebo-telnet](../../openspec/changes/sentinel-cebo-telnet/proposal.md) ([ficha](../encargos/sentinel-cebo-telnet.md), [diseño](../../openspec/changes/sentinel-cebo-telnet/design.md), [tareas](../../openspec/changes/sentinel-cebo-telnet/tasks.md)). Rama: `feat/sentinel-cebo-telnet`, apilada sobre `feat/sentinel-cebo-http` ([panel HTTP](cebo-http.md)), de la que depende. Se integra con la [API de módulos](../development.md#api-de-módulos-de-firmware-persona-1-2-y-3) y el [contrato v2](../../contracts/module-protocol.md).

Este incremento añade `telnet_session.cpp` (protocolo y privacidad, lógica pura) y `telnet_service.cpp` (sockets lwIP) en `firmware/SentinelNode/src/cebo/`, y hace que el módulo gestione HTTP y Telnet con límites y parada comunes. La clasificación por ventanas, `cebo.stats`, `cebo.configure` y la persistencia del historial pertenecen al cambio de [eventos e historial](../encargos/sentinel-cebo-eventos-historial.md). `SENTINEL_MODULE_CEBO` sigue en 0; ver [cómo activarlo](cebo-http.md#activar-y-compilar).

## Qué hace y qué no hace

En modo Cebo y con IPv4 en la LAN autorizada, el ESP32 acepta sesiones Telnet en el puerto 23 y simula una consola de router con inicio de sesión: pide usuario y contraseña, responde siempre «Login incorrect» y cierra tras tres intentos. **No hay shell**: no se llama a `system`, `exec` ni a ningún intérprete, y ninguna entrada se ejecuta. No debe exponerse a Internet ni mediante puertos del router. Solo IPv4 y solo TCP.

## Interacción esperada

```text
Router management console

login: <usuario>
Password:            (sin eco: el servidor ofrece IAC WILL ECHO)
Login incorrect

login:
```

Al tercer intento fallido responde `Too many failed attempts` y cierra. Todos los textos son fijos; nada de lo escrito por el cliente se refleja. Un cliente manual debe escribir dentro de 5 s entre teclas (límite de inactividad); un script no tiene problema.

| Hecho observado | Interacción registrada |
| --- | --- |
| Conexión aceptada | `connect` |
| Línea de usuario completa | `input` |
| Línea de contraseña completa | `auth_attempt` |
| Línea de más de 512 bytes | `input` o `auth_attempt` según la etapa, y cierre por límite |
| Cierre | `disconnect` con evidencia `peer_closed`, `idle_timeout` o `capacity_limit` |

Una interacción de Telnet lleva siempre `protocol: "telnet"`, `destination_port: 23` y `path: null`. Se publican `input` y `auth_attempt` como eventos `cebo.interaction`; conexión y cierre circulan por el bus. Ejemplo:

```json
{"source_ipv4":"192.0.2.2","source_port":51000,"destination_port":23,"protocol":"telnet","interaction":"auth_attempt","path":null,"count":1,"window_ms":1000,"suspicion":"informational","evidence":["auth_attempt"]}
```

Como en HTTP, cada interacción es **aislada** (`count: 1`, `suspicion: "informational"`, severidad `info`) hasta que el cambio de eventos e historial añada la clasificación. El cierre tras tres intentos o por exceso de línea se marca como `capacity_limit`: es el cierre por un límite propio.

## Protocolo: fin de línea e IAC

- Fin de línea: `CR LF`, `CR NUL`, `LF` o `CR` solos. El segundo byte de la pareja no cuenta como línea nueva aunque llegue en otra entrega. Las entradas pueden llegar en cualquier fragmentación, incluso byte a byte.
- Borrado (`BS`, `DEL`): baja la longitud de la línea sin pasar de cero.
- IAC acotado: `WILL x` se rechaza con `DONT x` y `DO x` con `WONT x`; `WONT` y `DONT` no se responden, para no abrir bucles. `DO/DONT ECHO` son respuestas a nuestra oferta de suprimir el eco y no se contestan. Como máximo 16 respuestas por sesión. `IAC IAC` es el dato 255. La subnegociación (`IAC SB … IAC SE`) se descarta, con un tope de 64 bytes para no tragar datos indefinidamente. El servidor no acepta ni ofrece ninguna otra función.

## Datos que se registran y datos que se descartan

De cada línea solo se **cuenta** su longitud: no se almacena, no se refleja y no se interpreta. La sesión y el bus no tienen ningún campo capaz de contener usuario, contraseña ni texto de línea. Lo recibido por `recv` existe solo en un buffer transitorio de 128 bytes que se borra con escritura volátil en cuanto se procesa. No hay logs de contenido. Quedan registrados únicamente IPv4 y puerto de origen, tipo de interacción y motivo de cierre. La prueba de host entrega un marcador secreto ficticio fragmentado (usuario, contraseña, subnegociación y línea excesiva) y comprueba que no aparece en la salida, en eventos ni en la memoria de la sesión y del bus. El borrado del buffer transitorio es una garantía de diseño y revisión; la prueba de host no puede inspeccionar la pila una vez liberada.

## Límites

Valores iniciales propuestos en el diseño; son cotas de implementación, no mediciones de capacidad. Se ajustan ante fallos, sin retirar funciones.

| Límite | Valor | Efecto |
| --- | --- | --- |
| Sesiones Telnet | 2 (`TelnetMaxSessions`) | La conexión excedente se acepta y se cierra de inmediato |
| Conexiones totales HTTP + Telnet | 6 = 4 + 2 (`MaxConnections`) | Coincide con `max_connections` del contrato v2 |
| Línea | 512 bytes de datos (`TelnetMaxLine`) | Mayor: «Input too long» y cierre por límite |
| Bytes recibidos por sesión | 4096 (`TelnetMaxSessionBytes`) | «Session limit reached» y cierre; acota el uso de CPU de un cliente que no termina líneas |
| Intentos de autenticación | 3 (`TelnetMaxAttempts`) | Cierre tras el tercero |
| Inactividad | 5 s (`IdleTimeoutMs`, compartido con HTTP) | La tarea cierra la sesión |
| Salida pendiente | 256 B | Cubre el peor caso (saludo, 16 respuestas y tres intentos); un cliente que no lee se cierra en lugar de acumular salida |
| Reintento de arranque fallido | 5 s | El estado Telnet pasa a `error` entre intentos; HTTP no se ve afectado |

## Ciclo de vida y parada común

- Un único `Cebo` gestiona los dos servicios con un bus compartido. Cada servicio espera la LAN, arranca, falla y se recupera por separado: un fallo de uno no detiene al otro. Si la LAN se pierde, ambos cierran y reabren al volver.
- El listener Telnet usa un socket de escucha lwIP y una tarea propia (`cebo_telnet`, 4 KiB de pila, prioridad inactiva + 3) con `select` de 100 ms. No se abre otro gestor Wi-Fi ni otro MQTT.
- `request_stop` no bloquea: la tarea ve la marca en su siguiente vuelta, cierra las sesiones y el listener, y solo entonces `stopped()` pasa a verdadero. El módulo no queda detenido hasta que **los dos** servicios cerraron; un cierre incompleto se reintenta en cada `tick` dentro del plazo del supervisor. La tarea termina con `vTaskDelete`; el sistema libera su memoria poco después, sin que quede sesión, socket ni código de la tarea en ejecución.
- `cebo.stop` y `cebo.start` actúan sobre los dos servicios. Estado: `{"kind":"cebo","http":{"available":true,"status":"running"},"telnet":{"available":true,"status":"running"},"active_connections":N,"max_connections":6,"idle_timeout_ms":5000}`, con `honeypot_http` y `honeypot_telnet` verdaderos; con ello el gate del núcleo ya admite `cebo.start`.
- Los servicios siguen arrancando al entrar en el modo Cebo (`AutoStartServices`) hasta que la app tenga el control de Cebo; ver los [puntos para coordinar con P1](cebo-http.md#puntos-para-coordinar-con-p1).

## Activar y compilar

Mismo procedimiento que el [panel HTTP](cebo-http.md#activar-y-compilar): activar `SENTINEL_MODULE_CEBO` solo en la copia local y no incluirlo en el PR.

```sh
node firmware/test/cebo/run_host.mjs
```

Comprueba, además de lo de HTTP: la sesión Telnet con entradas fragmentadas byte a byte, las cuatro formas de fin de línea, IAC (rechazo de opciones, fragmentación, subnegociación, tope de respuestas), línea de exactamente 512 y de 513 bytes, límite de bytes por sesión, inactividad, tres intentos en una sola entrega, privacidad con marcador secreto, límites comunes y la parada conjunta de los dos servicios con dobles. Con las dependencias del workspace instaladas, `node firmware/test/cebo/check_contracts.mjs` pasa los eventos y estados que produce el código por el decodificador v2 compartido (AJV). Ninguna de las dos sustituye la compilación con Arduino IDE ni las pruebas en LAN.

## Comprobación física pendiente (tareas 1.4, 2.1 y 2.2)

Requiere ESP32, LAN propia/autorizada y una copia con el flag activado y provisionada por BLE. Sustituye `IP` por la dirección de la placa. Registra PASS/FAIL/PENDIENTE y el commit grabado.

1. Cambiar a modo Cebo. Esperado: `module.http.status` y `module.telnet.status` pasan a `running`.
2. Con un cliente Telnet (PuTTY en modo Telnet, o `telnet IP` si el cliente está habilitado): aparece el saludo y `login:`. Escribir un usuario y una contraseña ficticios dentro de 5 s entre teclas: la contraseña no se muestra y responde «Login incorrect» y `login:` otra vez.
3. En paralelo, `curl.exe -i http://IP/` devuelve 200 mientras la sesión Telnet sigue abierta.
4. Se reciben eventos `cebo.interaction` con `input` y `auth_attempt` de `telnet` (puerto 23, ruta nula) y de HTTP, con la IP de origen.
5. Marcador ficticio fragmentado, sin cliente interactivo:

   ```powershell
   $c = New-Object Net.Sockets.TcpClient('IP', 23); $s = $c.GetStream()
   foreach ($ch in 'user-MARCADOR-DEMO-123'.ToCharArray()) { $s.WriteByte([byte]$ch); Start-Sleep -Milliseconds 40 }
   $s.Write([byte[]](13, 10), 0, 2)
   foreach ($ch in 'pass-MARCADOR-DEMO-123'.ToCharArray()) { $s.WriteByte([byte]$ch); Start-Sleep -Milliseconds 40 }
   $s.Write([byte[]](13, 10), 0, 2); Start-Sleep -Milliseconds 300
   $buffer = New-Object byte[] 512; [Text.Encoding]::ASCII.GetString($buffer, 0, $s.Read($buffer, 0, 512)); $c.Close()
   ```

   Esperado: el texto devuelto son solo los avisos fijos y, en monitor serie y eventos, el marcador no aparece.
6. Línea excesiva: enviar más de 512 bytes sin fin de línea. Esperado: «Input too long» y cierre.
7. Tres intentos fallidos: «Too many failed attempts» y cierre.
8. Inactividad: conectar y no escribir; a los 5 s el servidor cierra.
9. Límite: abrir tres conexiones a Telnet a la vez. Esperado: la tercera se cierra de inmediato.
10. Salir del modo (`set_mode` `idle`) o enviar `cebo.stop`: `Test-NetConnection IP -Port 80` y `Test-NetConnection IP -Port 23` informan `TcpTestSucceeded : False`. Volver a entrar en modo Cebo vuelve a abrir ambos puertos, lo que comprueba que no queda ningún listener ni tarea de la sesión anterior.
11. No abrir puertos del router ni exponer la placa a Internet.

## Estado de verificación

| Comprobación | Resultado |
| --- | --- |
| Pruebas de host de Cebo (HTTP y Telnet) | PASS (Windows, MinGW; sin radio) |
| Errores deliberados en una copia (límite de línea, eco IAC, tipo de interacción), comprobación puntual fuera del repositorio | Cada uno rompe la prueba correspondiente |
| Eventos y estado del código C++ frente al decodificador v2 compartido | PASS (20 eventos y 6 estados; paquete de contratos instalado en una carpeta temporal; host) |
| Compilación del módulo con core oficial ESP32 3.3.12, ESP32S3 (sketch aislado, `esp_http_server`, lwIP y FreeRTOS) | PASS |
| Verificar del sketch completo con el flag activado | PENDIENTE |
| 1.4 instrucciones con cliente Telnet en LAN | PENDIENTE |
| 2.1 Telnet y HTTP reales simultáneos con eventos | PENDIENTE |
| 2.2 Puertos 80 y 23 cerrados al salir de Cebo, sin tarea ni sesión residual | PENDIENTE |
