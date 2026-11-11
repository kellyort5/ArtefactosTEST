# Broker local protegido

Mosquitto **2.0.22**, imagen oficial `eclipse-mosquitto`, fijada al índice multiarquitectura `sha256:81a6193ae693942a3231cd3ac52875b02b7e3a19da671b2fb4b55cc874c5b279`. Un único servicio Compose, sin proxy, backend o cloud. MQTT 5/TLS en **8883** y WSS en **9001**, mínimo TLS 1.2, contraseña obligatoria en ambos. No existe listener 1883 ni alternativa anónima. Configuración fuente en `config/`; la ACL enumera identidades y topics exactos, sin confiar en un client ID elegido por el cliente.

Ejecutar desde la raíz del repositorio. Requiere Docker Engine/Docker Desktop iniciado y Compose, Python 3.10+, OpenSSL, Git y Node 24 con las dependencias del workspace instaladas (`npm ci` en raíz). Los scripts usan Docker Linux; en WSL pueden usar automáticamente la CLI Windows de Docker Desktop si el daemon Linux no responde, aunque exista un socket obsoleto. `DOCKER_BIN` permite elegir otra CLI, sin contener credenciales. En este entorno:

```sh
export PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH
python3 broker/scripts/verify-image.py
docker version
docker compose -f broker/compose.yaml config
```

La selección automática comprueba respuesta de `docker info` (máximo 5 s por CLI) y la conserva durante esa ejecución; si Linux falla, prueba la CLI Windows disponible. `DOCKER_BIN`, `DOCKER_HOST` y `DOCKER_CONTEXT` explícitos se respetan: no se cambia de engine para ocultar un fallo. Si ambos daemons fallan se conserva una CLI para `compose config` offline; las operaciones de ejecución muestran el error real de Docker.

Si WSL no tiene Docker integrado, iniciar Docker Desktop y habilitar la integración de esta distro. Como alternativa local disponible, `'/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' desktop start`; `python3 broker/scripts/manage.py config` resuelve rutas Windows/WSL. `config` solo imprime rutas y parámetros públicos: las contraseñas nunca son variables Compose.

## Si WSL muestra timeout o un socket obsoleto

La existencia de `/var/run/docker.sock` no demuestra que el engine esté activo. Comprobar estado y puertos antes de cambiar endpoint o confianza TLS:

```sh
python3 broker/scripts/manage.py ps
'/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' desktop status
'/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' desktop start
python3 broker/scripts/manage.py ps
```

Si está disponible únicamente la CLI Windows, se puede seleccionarla explícitamente, conservando conversión de rutas:

```sh
DOCKER_BIN='/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' python3 broker/scripts/manage.py ps
```

Si Docker responde pero el contenedor está parado, `python3 broker/scripts/manage.py up` lo inicia conservando volumen y secretos. Después repetir `node broker/scripts/smoke.mjs` con Node24/dependencias del workspace; WSL debe alcanzar localhost/127.0.0.1 en ambos puertos y validar la CA. Un timeout TCP/CONNACK no se corrige desactivando TLS ni cambiando SAN. El nombre de host continúa siendo `localhost` mientras se use el laboratorio generado por defecto. Pruebas focalizadas de selección de CLI: `python3 broker/scripts/test_common.py`; son independientes de la prueba funcional real del broker.

## Preparar un laboratorio descartable

Antes de conectar un nodo real, leer su `device_id` y usarlo en `--node-id`; para las pruebas aisladas usar `lab-node`. Los IDs admiten 1–64 caracteres alfanuméricos, `_` y `-`; usar máximo 55 para que también `operator-<id>` cumpla el límite de usuario del contrato BLE. `--peer-id` prepara un segundo nodo local (por defecto `acl-check-peer`) para comprobar aislamiento entre nodos; debe ser distinto. Se generan cuentas separadas node/operator/viewer para cada nodo, sin una cuenta administrativa.

```sh
python3 broker/scripts/prepare.py --node-id lab-node
python3 broker/scripts/manage.py config
python3 broker/scripts/manage.py up
python3 broker/scripts/manage.py ps
node broker/scripts/check-provisioning.mjs
```

Para LAN, incluir **antes de generar** el DNS y la IP realmente utilizados mediante `--san`, repetible. Ejemplo de nombres ficticios que deben sustituirse por los de la red propia:

```sh
python3 broker/scripts/prepare.py --node-id ID_REAL --san broker.example --san 192.0.2.10
```

El script no sobrescribe un laboratorio existente ni rota credenciales silenciosamente. Para otro laboratorio conservar el existente y preparar otro checkout. Si una operación falla antes de generar archivos, se puede repetir; un fallo posterior requiere revisar el material parcial en privado y preparar otro checkout, no iniciar el broker con archivos incompletos. No se emite el marcador `ready` hasta terminar. Ausencia de archivos produce instrucciones de preparación y fallo de arranque, nunca acceso anónimo.

`prepare.py` crea solo en `broker/.secrets/` ignorado:

- `ca.key`, `ca.crt`: CA local EC P256; la clave permanece privada y **no se monta en el contenedor**. CA de 365 días; certificado servidor de 30 días. Volver a preparar otro laboratorio al caducar, sin omitir validación de fecha.
- `runtime/server.crt`, `runtime/server.key`, `runtime/mosquitto.passwd`, `runtime/acl`, `runtime/ready`: únicamente lo necesario para el broker.
- `clients.json`: credenciales aleatorias por identidad y nodo, para aprovisionamiento privado y cliente de prueba; no usar cuentas de otros servicios.

No se pasa ninguna contraseña por argumentos, variables de Docker, logs o shell history. `mosquitto_passwd -U` convierte un archivo privado a PBKDF2; si falla, se vacía el archivo que contenía el texto claro y no se habilita el arranque. Abrir `clients.json` **solo en un editor local privado** al aprovisionar o introducir la cuenta correspondiente en la UI; no imprimirlo con `cat`, pegarlo en chats ni adjuntarlo a informes. La app recibe operator/viewer en ejecución y los mantiene en RAM; firmware recibe solo su cuenta node por BLE protegido y la mantiene en RAM. No incorporar `.secrets`, contraseñas o claves al frontend, a variables `VITE_*`, fixtures ni repositorio.

En Linux se aplica umask 077; en unidades Windows montadas en WSL, el script restringe además la ACL de `.secrets` al SID del usuario actual usando PowerShell. Si falla esa restricción, aborta antes de crear claves. Docker monta solo `runtime/` de lectura; al arrancar copia ese material a tmpfs Linux 0700/0600 y propietario 1883, incluso cuando los permisos del bind mount Windows no reflejan chmod. Mosquitto baja privilegios a su usuario. La CA privada y `clients.json` no son accesibles dentro del contenedor.

## Confianza y endpoints

Por defecto, `mqtts://localhost:8883` para clientes nativos y `wss://localhost:9001/mqtt` para mqtt.js. `localhost` e IP `127.0.0.1` están en SAN; para otro endpoint se exige SAN coincidente. CA pública en `.secrets/ca.crt`: suministrarla al cliente nativo y al firmware vía BLE protegido, con una referencia horaria válida antes de TLS. La CA EC compacta permite construir `configure_connectivity` respetando CA≤1536 y mensaje completo≤2048 bytes; validar siempre el comando final con el codec compartido. La CA no es una contraseña, pero el directorio que la contiene también guarda claves: compartir **solo ca.crt** por un canal local adecuado.

En navegador, confiar en la CA en el almacén que use el navegador. Para Android seguir la configuración CA/debug documentada por la app; no activar tráfico claro global. mqtt.js web depende del almacén de confianza del navegador/WebView. `ca` de Node en la prueba no demuestra confianza Android. No usar `rejectUnauthorized:false`, certificados aceptados manualmente por excepción, hostname alternativo sin SAN ni salto de fecha. Los fallos de CA/hostname se corrigen aprovisionando confianza y SAN correctos.

## ACL y contratos

Para cada ID autorizado, prefijo `sentinel/v1/nodes/<id>/`:

| Cuenta | Leer | Publicar |
| --- | --- | --- |
| `node-<id>` | commands | state, availability, metrics, events, acks |
| `operator-<id>` | state, availability, metrics, events, acks | commands |
| `viewer-<id>` | state, availability, metrics, events, acks | nada |

Todo lo demás se deniega, incluyendo otro nodo y `$SYS`. Replicar bloques explícitos de la plantilla al añadir nodos y generar sus credenciales de forma privada; reiniciar para aplicar. Ambos listeners comparten la misma política. Las ACL controlan topics; no validan JSON ni impiden a un operador publicar un comando retenido: app y firmware deben imponer frescura, versión, autorización de operación, retención y confirmación según [contratos](../contracts/README.md).

state/availability QoS1 retenidos; metrics QoS0 sin retener; events/commands/acks QoS1 sin retener. availability es `online`/`offline` y LWT; un retained antiguo no acredita presencia actual. `max_packet_size` 8192 acota paquetes, `message_size_limit` 2048 acota payload; el broker no reemplaza la validación v1 en los extremos.

## Prueba funcional real, parada y persistencia

Usar **solo el laboratorio descartable**, antes de conectar producto: la prueba publica sondas de transporte, no estados de producto ni incidentes. Usa las cuentas del archivo privado en RAM y `mqtt` ya instalado en el workspace, sin otro package/lockfile:

```sh
PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH node broker/scripts/smoke.mjs
python3 broker/scripts/manage.py logs
python3 broker/scripts/manage.py stop
python3 broker/scripts/manage.py up
```

Comprueba por TLS y WSS flujos node→viewer, operator→node y node→operator; rechazo anónimo, password incorrecta, CA desconocida, hostname fuera de SAN; PUBACK denegado y **ausencia de entrega** para nodo/operador fuera de alcance y viewer que publica; filtrado de lecturas no autorizadas. Después publica una sonda retenida, reinicia el contenedor real y exige recuperación por WSS con flag retained; al terminar limpia solo esa sonda. Un fallo termina con exit 1; no se sustituye el broker por un proceso simulado.

`SENTINEL_TEST_HOST` selecciona un endpoint realmente incluido en SAN para las pruebas. `manage.py restart` reinicia; `stop` conserva contenedor y volumen; `up` vuelve a iniciar. El volumen nombrado `sentinel-core_mqtt-data` contiene la persistencia Mosquitto, con guardado periódico de 30 s y al detenerse ordenadamente. No se borra como parte de scripts de recuperación. Esto conserva estado MQTT configurado entre reinicios, **no** ofrece un archivo histórico ni recupera eventos que el broker nunca recibió; el ring del firmware sigue siendo necesario. Un corte abrupto puede perder datos aún no guardados.

## LAN autorizada y firewall

Compose enlaza únicamente `127.0.0.1` por defecto. Para placa/teléfono elegir la IP local concreta de una red propia/autorizada, incluirla en SAN y cambiar explícitamente el enlace:

```sh
SENTINEL_BIND_IP=192.0.2.10 python3 broker/scripts/manage.py config
SENTINEL_BIND_IP=192.0.2.10 python3 broker/scripts/manage.py up
```

`192.0.2.10` es un ejemplo documental, no una IP utilizable aquí. Mantener el mismo parámetro al ejecutar operaciones Compose; `up` recreará el contenedor si cambió el enlace, conservando el volumen. Permitir en el firewall del host únicamente TCP8883/9001 desde los clientes de esa LAN autorizada, sin desactivar el firewall completo. En Docker Desktop el bind/firewall pertenecen al host Windows; comprobar la IP allí. Usar el endpoint LAN en los clientes, nunca `localhost` del teléfono. No abrir puertos del router, UPnP ni acceso Internet. Para volver a loopback, ejecutar `up` sin SENTINEL_BIND_IP y revisar `config`/`ps`.

La prueba local de loopback no acredita alcance LAN, firewall ni hardware: verificarlos aparte con un cliente autorizado desde esa red y registrar resultado. Evidencia de esta implementación en [evidence.md](evidence.md).

Fuentes oficiales: [imagen Docker](https://hub.docker.com/_/eclipse-mosquitto), [configuración Mosquitto](https://mosquitto.org/man/mosquitto-conf-5.html). El digest se vuelve a comprobar con `verify-image.py`; no se actualiza automáticamente.
