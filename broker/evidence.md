# Evidencia del grupo 5 — sentinel-core-foundation

> Registro histórico de su entrega. Para instalación y alcance actual consultar [acuerdos vigentes](../docs/mvp-policy.md). TLS, cuentas/ACL o dependencias anteriores aquí descritas no se reinstalan para el MVP actual; se conservan las evidencias y pendientes originales.

**Evidencia histórica:** las pruebas TLS/WSS de este documento corresponden a la configuración anterior. El MVP actual usa MQTT/TCP1883 y WS9001 sin certificados; ver [verificación vigente](../openspec/changes/sentinel-mqtt-mvp-lan/verification.md).


Fecha local: 2026-11-08 (America/El_Salvador). Ámbito exclusivo: `broker/` y casillas 5.x de tasks.md. Rama existente `feat/sentinel-core-foundation`; sin commits, push, merge, cambios de rama o borrado de fuentes/volúmenes. No se modificaron proposal/design/specs ni package/lockfile del trabajo compartido.

## Entorno real

WSL Ubuntu, Python 3.12.3, OpenSSL 3.0.13, Node v24.13.1. Docker inicialmente no estaba integrado en la distro y la CLI Windows no encontraba `dockerDesktopLinuxEngine`; `docker.exe desktop start` inició Docker Desktop y habilitó el socket real. Docker Desktop 4.76.0, Engine/CLI 29.5.2, Compose v5.1.4; contenedor Linux amd64 ejecutando Mosquitto **2.0.22** (comprobado dentro del contenedor). El bloqueo inicial de Docker quedó resuelto, no se usó broker simulado.

Imagen oficial: `eclipse-mosquitto:2.0.22@sha256:81a6193ae693942a3231cd3ac52875b02b7e3a19da671b2fb4b55cc874c5b279`. Se consultó el índice del registro Docker Hub con token anónimo de lectura, se verificó SHA256 del cuerpo contra `Docker-Content-Digest` y el digest fijado; `docker pull` confirmó ese digest. `verify-image.py` conserva la comprobación reproducible sin imprimir tokens.

## Comandos y resultados

Todos desde la raíz del repositorio; PATH Node usado: `/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH`.

| Comando/comprobación | Resultado |
| --- | --- |
| `python3 broker/scripts/verify-image.py` | PASS: tag exacto y digest del manifiesto oficial coinciden. |
| `docker compose -f broker/compose.yaml config` y `config --services` | PASS: únicamente mosquitto; puertos 8883/9001 enlazados a 127.0.0.1; volumen nombrado y rutas de archivos, sin contenido secreto. |
| `python3 broker/scripts/prepare.py --node-id lab-node` | PASS: CA EC P256/SAN localhost+127.0.0.1, certificado servidor, seis cuentas aleatorias para dos nodos, hashes `mosquitto_passwd -U`, ACL explícitas y marcador final. |
| Restricción DACL Windows y revisión recursiva de sus ACE | PASS: `.secrets` y todos sus archivos permiten acceso al SID del usuario actual; sin otros principals. Se corrigió la aplicación DACL para no exigir SeSecurityPrivilege/SACL. |
| `git check-ignore` y `git ls-files --error-unmatch` sobre cada archivo privado | PASS: todos ignorados y ninguno versionado. No se imprimieron contenidos. |
| Ejecución del entrypoint real en imagen Docker sin material privado | PASS: exit 1 y mensaje accionable `prepare.py --node-id ID`; no inicia broker ni acceso anónimo. |
| `python3 broker/scripts/manage.py up`, `ps`, `logs` | PASS: contenedor real activo, ambos puertos protegidos en loopback. |
| `node broker/scripts/smoke.mjs` | PASS: las cinco líneas de resultados de abajo; la versión final incluyó comandos de operador hacia otro nodo y denegación de lecturas. |
| Conexiones Python ssl con CA y hostname validados, versiones fijadas TLS1.2 y TLS1.3, a ambos puertos | PASS: ambos listeners negocian cada versión segura. No se desactivó verificación. |
| `node broker/scripts/check-provisioning.mjs` | PASS: CA pública 648 bytes; comando configure_connectivity validado por `@sentinel/contracts`, 1198 bytes con la cuenta privada cargada solo en RAM. No acredita envío BLE. |
| `python3 broker/scripts/manage.py stop` y `up` | PASS: parada y reanudación del mismo contenedor/volumen. |
| Render de Compose con `SENTINEL_BIND_IP=192.0.2.10` | PASS: el parámetro cambia host_ip de ambos puertos; solo render, no se intentó enlazar esa IP documental ni se abrió LAN. |
| Revisión final fuentes/logs/inspect y permisos dentro del contenedor | PASS: 13 fuentes del broker sin passwords ni claves privadas, tampoco passwords en logs/inspect; proceso principal UID1883, tmpfs700 y clave600, sin montar CA privada ni clients.json. |
| `python3 -m py_compile broker/scripts/*.py` y `node --check broker/scripts/smoke.mjs` | PASS. |

Salida funcional final, sin secretos:

```text
PASS mqtts: node/operator/viewer authorized round trips
PASS mqtts: anonymous/password/CA/hostname/ACL denied and unauthorized deliveries absent
PASS wss: node/operator/viewer authorized round trips
PASS wss: anonymous/password/CA/hostname/ACL denied and unauthorized deliveries absent
PASS restart: retained state survives in named volume and is received with retain=true
```

En ambos listeners se exigieron códigos de rechazo MQTT 5 para credenciales y PUBACK 135 para publicaciones prohibidas, errores de confianza para CA desconocida y ERR_TLS_CERT_ALTNAME_INVALID para hostname fuera de SAN. Se observaron suscriptores autorizados durante publicaciones prohibidas y no recibieron esas sondas. Se verificó además filtrado de lecturas de otro nodo y de commands por viewer, mientras el destinatario autorizado recibía el mismo mensaje. La sonda retenida sobrevivió al reinicio real y llegó por WSS con retain=true; se limpió únicamente esa sonda descartable al finalizar. El broker no produce estados ni incidentes de producto ficticios.

Los logs de Mosquitto muestran errores TLS de las conexiones negativas/cierres de los clientes y una recomendación informativa de reemplazar message_size_limit; se conserva este límite 2048 además del límite de paquete 8192 porque el primero limita el payload del contrato. No aparecen contraseñas, claves o cuerpos privados en los logs.

## Seguimiento por ID

| Tarea | Estado | Evidencia / pendiente |
| --- | --- | --- |
| 5.1 | Completa | Compose válido, un servicio, imagen oficial exacta+digest verificados. |
| 5.2 | Completa | Preparación real, SAN, claves y passwords fuera de args/logs/Git, DACL/ignorados comprobados y fallo accionable sin archivos privados. |
| 5.3 | Completa | Arranque Docker y flujos autenticados reales node/operator/viewer sobre TLS y WSS. |
| 5.4 | Completa | Negativos de autenticación, confianza, hostname, escritura y lectura ACL con ausencia de entrega no autorizada, por ambos listeners. |
| 5.5 | **Parcial, casilla pendiente** | Volumen persistente, loopback por defecto y parámetro LAN implementados; reinicio con retención comprobado. Falta IP/LAN autorizada concreta y cliente desde esa red para comprobar alcance/firewall tras configuración; el render de una IP documental no lo demuestra. |
| 5.6 | Completa | README de preparación/CA/roles/puertos/LAN/firewall/ejecución/parada, contrastado con comandos locales reales; sin router ni cuentas admin frontend. La comprobación LAN pendiente se identifica expresamente en 5.5. |

No se han usado ESP32/OLED/Android ni se reclama verificación de hardware, BLE o confianza Android. El usuario difirió el uso de Android; estas pruebas del broker usaron clientes locales reales. La ausencia de un entorno LAN autorizado concreto impide completar 5.5, sin impedir la implementación y prueba independiente del resto.

## Entrega e integración privada

El contenedor queda levantado en loopback para la integración del coordinador. Endpoints públicos locales: `mqtts://localhost:8883` y `wss://localhost:9001/mqtt`. CA pública solo en `broker/.secrets/ca.crt`; cuentas separadas por rol en `broker/.secrets/clients.json` privado; no se adjuntan ni divulgan valores. Para producto usar device_id real/SAN de su red, cuenta node en firmware y operator/viewer en app, siempre mediante aprovisionamiento privado y RAM conforme al diseño.

Fuentes añadidas: `broker/compose.yaml`, `broker/config/mosquitto.conf`, `broker/config/acl.template`, `broker/.gitignore`, `broker/scripts/common.py`, `broker/scripts/prepare.py`, `broker/scripts/manage.py`, `broker/scripts/entrypoint.sh`, `broker/scripts/verify-image.py`, `broker/scripts/smoke.mjs`, `broker/scripts/check-provisioning.mjs`, `broker/README.md` y este informe. En tasks.md se actualizan solo 5.1/5.2/5.3/5.4/5.6 bajo lock exclusivo `/tmp/sentinel-core-tasks.lock`, releyendo y contrastando el texto original; se comprueba progreso OpenSpec tras cada cambio.

## Revalidación de disponibilidad WSL — tarea task_65da43ed3c8d

Tras el aviso de la app, se reprodujo el fallo: `/var/run/docker.sock` existía pero `/usr/bin/docker` respondía `Cannot connect to the Docker daemon at unix:///var/run/docker.sock`; la CLI Windows también fallaba porque el pipe `dockerDesktopLinuxEngine` no existía, y `docker.exe desktop status` no podía obtener el estado. Las cuatro conexiones TCP desde WSL a localhost/127.0.0.1 en 8883/9001 agotaron timeout. Esto acredita indisponibilidad real del engine desde ambas CLIs en ese momento; no identifica por qué dejó de estar disponible. Esta sesión tiene acceso sin sandbox y política de aprobación never, por lo que no admite `require_escalated`; no fue necesario escalar ni declarar un bloqueo final.

Se ejecutó `'/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' desktop start`. Después Engine29.5.2 respondió por ambas CLIs y el mismo contenedor, con política de reinicio, volvió a `running`, sin OOM ni error de proceso y manteniendo publicación exclusivamente en `127.0.0.1`. No se borraron ni rotaron `.secrets`, certificados, contraseñas, contenedor o volumen; no se modificaron Compose, ACL ni protecciones TLS.

Se corrigió `broker/scripts/common.py`: la selección automática ahora prueba `docker info` con timeout5 s por candidato y usa Windows si la CLI nativa no alcanza el daemon, incluso si el socket WSL sigue existiendo. Mantiene la selección durante una ejecución para que rutas y comandos usen la misma CLI; respeta DOCKER_BIN y endpoints/contextos explícitos. Si ningún daemon responde conserva una CLI para renderizar Compose offline y deja visibles los errores reales al ejecutar operaciones. No arranca otro engine automáticamente. README documenta diagnóstico, recuperación y selección Windows explícita.

Verificaciones posteriores:

- `python3 broker/scripts/manage.py ps`: mismo servicio Mosquitto activo con 8883/9001 solo en 127.0.0.1; selección automática `/usr/bin/docker` ahora responde.
- `DOCKER_BIN='/mnt/c/Program Files/Docker/Docker/resources/bin/docker.exe' python3 broker/scripts/manage.py ps`: PASS, mismo contenedor accesible, conversión de rutas Windows/WSL correcta.
- Conexiones TCP `socket.create_connection` desde WSL: PASS localhost8883, localhost9001, 127.0.0.1:8883 y 127.0.0.1:9001.
- `PATH=/home/jefry/.nvm/versions/node/v24.13.1/bin:$PATH node broker/scripts/smoke.mjs`: PASS nuevamente las cinco líneas del informe inicial, incluyendo TLS y WSS reales, credenciales válidas, anónimo/password/CA/hostname inválidos, ACL de escritura/lectura sin entrega prohibida y retained tras reinicio real.
- `python3 broker/scripts/test_common.py`: seis regresiones de selección de CLI pasan (nativa disponible y caché, fallback Windows y rutas, timeout de sondeo, DOCKER_BIN explícito, endpoint explícito sin cambio de engine, ambas no disponibles para config offline). Estos tests simulan únicamente resultados de sondeo de CLI; la prueba funcional anterior usa Docker/MQTT reales.
- `python3 -m py_compile broker/scripts/common.py broker/scripts/test_common.py` y `python3 broker/scripts/manage.py config`: PASS.

Rutas y endpoints para raíz/app continúan siendo `mqtts://localhost:8883`, `wss://localhost:9001/mqtt`, CA pública `broker/.secrets/ca.crt` y cuentas por rol en `broker/.secrets/clients.json` privado, cargadas solo en RAM por los clientes. Se comunicó al coordinador la recuperación y estas rutas sin divulgar credenciales. La app puede repetir su recorrido local con Node24; no hace falta modificar hostname/SAN o aceptar certificados inválidos para resolver este incidente.

Archivos modificados en esta revalidación: `broker/scripts/common.py`, `broker/scripts/test_common.py` (nuevo), `broker/README.md` y este informe. El inventario de fuentes del broker pasa de13 a14 archivos; material privado permanece ignorado. No se tocaron tareas OpenSpec: **5.5 sigue pendiente** por la comprobación de LAN/firewall con red autorizada real, y recuperar loopback no completa esa comprobación. No quedan bloqueos para la prueba local de la app en el estado comprobado; Docker Desktop debe permanecer iniciado.
