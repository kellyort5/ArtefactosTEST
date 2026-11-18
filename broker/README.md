# Broker MQTT del MVP

Mosquitto **2.0.22**, imagen oficial fijada al digest de `compose.yaml`, en un único servicio Docker. MQTT5/TCP en **1883** y MQTT5/WebSocket en **9001**, sin TLS ni certificados por decisión del usuario. Autenticación obligatoria y ACL por nodo/rol para **v1 y v2**. Persistencia en volumen; no sustituye el historial del ESP32.

Usar una red de confianza propia o autorizada. Las cuentas y los mensajes MQTT viajan sin cifrar; usar cuentas exclusivas del laboratorio. El broker escucha en LAN por defecto. No publicar estos puertos en Internet ni configurar redirecciones en el router. BLE y HTTPS/DoH conservan sus protecciones.

## Preparar y arrancar

Desde la raíz, con Docker Desktop/Engine activo, Compose y Python3:

```sh
python3 broker/scripts/prepare.py --node-id ID-REAL-DEL-NODO
python3 broker/scripts/manage.py up
python3 broker/scripts/manage.py ps
```

Obtener el ID real por BLE/metadata o consola; el ID ficticio `lab-node` sirve solo para pruebas locales. IDs de 1–55 caracteres alfanuméricos, `_` o `-`. La preparación crea cuentas `node-<id>`, `operator-<id>` y `viewer-<id>` y un segundo nodo descartable para comprobar aislamiento. No hay cuenta administrativa en la app. No requiere OpenSSL, CA, SAN, DNS especial ni sincronizar la hora para conectar.

El material privado queda en `broker/.secrets/`, ignorado por Git: `clients.json` contiene las cuentas y `runtime/` hashes de contraseñas, ACL y marcador de preparación. Abrir `clients.json` solo en un editor privado para introducir la cuenta; no imprimirlo en terminal/chat, capturas o informes. Permisos privados en Linux y ACL del usuario en Windows/WSL. Las contraseñas no se pasan por argumentos de procesos, Docker env o logs.

Si ya existe `.secrets`, **no volver a generarlo ni borrarlo**: `manage.py up` reutiliza cuentas/volumen y aplica las ACL fuente v1/v2 a las identidades existentes. Los certificados de una preparación anterior quedan sin uso; no se copian al tmpfs ni se exigen. No hace falta rotar cuentas para cambiar de red o IP. `prepare.py` no sobrescribe un laboratorio existente.

## Teléfono y ESP32 reales

1. Conectar PC, teléfono y ESP32 a una red de confianza que permita comunicación entre clientes. Puede ser una red autorizada de otra persona; no se necesita administrar el router. El ESP32 requiere Wi-Fi compatible de 2.4 GHz.
2. Obtener la **IPv4 del PC** con `ipconfig` en Windows, en el adaptador Wi-Fi/Ethernet de esa red. No usar IP de WSL, VPN o Docker. Ejemplo ficticio: `192.168.1.10`.
3. En la app Android instalada, usar `ws://192.168.1.10:9001/mqtt`, ID del nodo y cuenta operator/viewer. No instalar certificados. `localhost` en un teléfono es el teléfono, no el PC.
4. Configurar el ESP32 por BLE protegido con `mqtt://192.168.1.10:1883`, cuenta node y datos de la Wi-Fi autorizada. Sin CA. Configurar no inicia MQTT; requiere entrar en Cebo cuando su módulo esté disponible. La base actual no implementa servicios Cebo ni los habilita ficticiamente.
5. Si Windows pregunta por Docker en el firewall, permitirlo solo en la red de confianza. Si hace falta, autorizar **en el PC** los puertos TCP1883/9001; no desactivar todo el firewall ni cambiar el router. Si no tienes permiso para ello, usar un equipo/red disponibles que sí permitan comunicación.

`SENTINEL_BIND_IP` permite acotar el enlace a una IP del PC o a `127.0.0.1` para pruebas solo locales. Por defecto Compose publica `0.0.0.0:1883` y `0.0.0.0:9001`; reiniciar con `manage.py up` tras cambiar el enlace. En Docker Desktop/WSL los puertos se publican mediante Docker Desktop; no se exige portproxy ni IP fija. El acceso real depende de la configuración local del host.

## Diagnóstico sin tocar el router

Desde el navegador del teléfono abrir `http://IP-DEL-PC:9001/`. Una respuesta HTTP incluso de error del listener demuestra que ese puerto es alcanzable; la prueba definitiva es conectar MQTT en la app. Si no responde, comprobar IP, contenedor y firewall del PC. Cambiar de Wi-Fi puede cambiar la IP: actualizar el endpoint de app y nodo, sin regenerar cuentas.

Una Wi-Fi de invitados puede aislar clientes aunque todos tengan Internet. TLS no resuelve ese aislamiento. Si la red no permite comunicación local, usar otra red o un punto de acceso disponible que sí la permita, con PC/teléfono/ESP32 comunicables; no se requieren cambios al router ajeno. Completar cualquier portal cautivo antes; el firmware de configuración básica no automatiza portales ni redes empresariales.

Si conecta al puerto pero rechaza MQTT, comprobar cuenta, contraseña y el ID exacto. El client ID no concede permisos. El viewer solo observa; operator publica commands de su nodo; node publica state/availability/metrics/events/acks y lee sus commands. Todo lo demás, otros nodos y `$SYS` se deniega. Ambos listeners comparten política. Las ACL no validan el JSON ni la frescura: app y firmware conservan esas comprobaciones.

## Comprobar y detener

```sh
node broker/scripts/check-provisioning.mjs
node broker/scripts/smoke.mjs
npm run check:mqtt -w app
python3 broker/scripts/test_common.py
python3 broker/scripts/manage.py stop
```

Usar Node24 y dependencias instaladas del workspace para las pruebas JS. El smoke usa clientes reales TCP y WS, comprueba v1/v2, eventos/comandos/ACK, rechazo anónimo/contraseña/permisos/lecturas cruzadas y retained tras reiniciar el contenedor; limpia solo su sonda descartable. Solo ejecutar contra este laboratorio autorizado, sin tráfico de pruebas en redes ajenas. `SENTINEL_TEST_HOST` selecciona otro host autorizado; por defecto localhost. No hay CA que suministrar.

`stop` conserva contenedor, cuentas y volumen; `up` vuelve a arrancar; `restart` aplica ACL y reinicia; `logs` muestra diagnóstico acotado. Un socket Docker existente no demuestra que el engine esté activo: consultar `ps`, iniciar Docker Desktop y su integración WSL si corresponde. No cambiar identidades para resolver un timeout de red.

La [verificación vigente](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md) distingue software, build y pruebas físicas. [evidence.md](evidence.md) conserva los resultados históricos de TLS, ya sustituidos.
