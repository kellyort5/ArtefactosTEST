# Tasks

**Cierre administrativo autorizado el 2026-11-10:** se archiva con 26/45 tareas verificadas y 19 comprobaciones pendientes aceptadas por el usuario bajo la suposición de que el hardware funciona. Las casillas abiertas se conservan; no son pruebas realizadas ni PASS. Ver [closure.md](closure.md) y el informe integral. Esta aceptación sustituye la condición de archivo del seguimiento original, sin modificar los requisitos funcionales.

Estas tareas implementan únicamente la base compartida descrita en [proposal.md](proposal.md), [design.md](design.md) y las siete specs. Ninguna está completada por crear estos artefactos. Mantener cambios pequeños y no hacer commits, push, merges ni eliminar ramas sin autorización explícita.

Dependencias: grupo 1 → 2 → 3 → 4; grupo 5 depende de 1/2 y puede prepararse independientemente; grupo 6 usa 2–5; grupo 7 usa 1/2/5/6; grupo 8 comprueba el conjunto. El simulador no bloquea el trabajo real. No esperar benchmarks ni las funcionalidades futuras de los módulos.

## 1. Monorepo y configuración reproducible

- [x] 1.1 Crear estructura separada `firmware/`, `broker/`, `app/`, `contracts/` y `simulator/`, preservando archivos existentes; verificar el árbol y que no se añadan servicios o frameworks de monorepo ajenos a D-01.
- [x] 1.2 Crear workspace npm privado para app/contratos y scripts básicos; fijar Node y dependencias compatibles de D-01 con lockfile único; verificar instalación limpia con `npm ci`.
- [x] 1.3 Inicializar React/Vite/TypeScript/Tailwind/Zustand con una vista de base sin datos ficticios; verificar `npm run build -w app` y ejecución del servidor de desarrollo.
- [x] 1.4 Documentar prerrequisitos y scripts en README y `docs/development.md`, incluyendo el papel futuro del simulador; verificar que nombres/rutas de scripts coincidan y no se modifiquen acuerdos de arquitectura fuera del cambio.

## 2. Contratos v1 y validación compartida

- [x] 2.1 Definir esquemas JSON v1 para state/metrics/event/command/ack, sobre común, capacidades y comandos de base de D-04; verificar tipos, tamaño máximo y campos mediante fixtures válidos e inválidos.
- [x] 2.2 Publicar tipos/codec/validador del workspace de contratos usando AJV y un script `npm run check:contracts`; verificar rechazo de versión incompatible, mensaje malformado, tamaño excesivo y campos privados en respuestas públicas.
- [x] 2.3 Definir UUIDs, metadatos BLE, framing y ejemplos de fragmentación sin secretos; verificar ensamblado con MTU por defecto, límite, timeout y desconexión mediante casos pequeños ejecutables.
- [x] 2.4 Documentar comandos, errores, vigencia monotónica, idempotencia, tópicos/QoS y ejemplos ficticios en `contracts/README.md`; verificar correspondencia con D-04/D-05/D-06 y conservar como pendientes las operaciones específicas de módulos.

## 3. Firmware N16R8, supervisor y periféricos

- [x] 3.1 Crear `platformio.ini`, CMake, entrada C/C++ y configuración ESP-IDF/FreeRTOS con versiones fijadas; definir entorno `sentinel_s3`; verificar `pio run -d firmware -e sentinel_s3`.
- [ ] 3.2 Configurar overrides de flash 16 MB, PSRAM octal 8 MB, tabla de particiones y `event_log` de D-02; verificar compilación y, al grabar, tamaños detectados sin asumir las características N8 del perfil genérico.
- [ ] 3.3 Implementar identidad persistente, boot ID nuevo y estado seguro inicial sin conexión automática; verificar en consola una inicialización real sin secretos y revisión/capacidades coherentes.
- [x] 3.4 Implementar validación cJSON equivalente al contrato y gestor de comandos con vigencia, revisión, caché acotada y errores sanitizados; verificar con los mismos casos breves de versión, comando vencido, otro arranque, duplicado e ID conflictivo.
- [x] 3.5 Implementar supervisor serializado y gestores de recursos con perfiles idle/Escudo/Cebo y puntos de extensión; verificar exclusión, cola llena, transición fallida y rechazo de Sanitizador/captura/honeypot mediante pruebas focalizadas sin servicios ficticiamente completados.
- [ ] 3.6 Integrar SSD1306/I2C y botón configurable con debounce y patrones de D-02; verificar compilación, pantalla real, acción corta y ventana de pairing sin doble acción; informar OLED ausente sin habilitar pairing inseguro.
- [ ] 3.7 Documentar cableado propuesto, configuración efectiva, puerto USB, grabación/monitor y comprobaciones del grupo en `docs/firmware.md`; verificar comandos de build/upload/monitor y registrar la lectura real de memoria cuando haya placa disponible.

## 4. Registro persistente compartido

- [ ] 4.1 Implementar ring NVS limitado de D-07 con identificadores/cursor estables y registros sanitizados; verificar almacenamiento de un evento de sistema real y lectura después de un reinicio breve.
- [x] 4.2 Implementar recuperación de slots, sobrescritura y reporte de gap/rango retenido sin borrado automático ante error; verificar un caso pequeño de límite con capacidad reducida para la comprobación, sin pruebas prolongadas de desgaste.
- [x] 4.3 Implementar consulta `read_events` paginada y reentrega con los mismos IDs, sin eliminar por ack MQTT; verificar límites de página/mensaje y repetición de una consulta interrumpida.
- [x] 4.4 Documentar capacidad, garantías, tratamiento de errores y recuperación en `docs/firmware.md`; verificar que lo descrito coincide con los límites efectivos y no promete historial ilimitado o cero pérdidas ante todo corte.

## 5. Broker protegido en Docker Compose

- [x] 5.1 Crear Compose con imagen oficial Mosquitto 2.x exacta/digest y configuración fuente; verificar `docker compose -f broker/compose.yaml config` sin secretos impresos y ausencia de servicios adicionales.
- [x] 5.2 Implementar preparación local de CA/certificados con SAN y usuarios de laboratorio sin claves/contraseñas en argumentos o logs; verificar que los archivos privados estén ignorados y que su ausencia produzca un fallo accionable, no acceso anónimo.
- [x] 5.3 Configurar listeners TLS/WSS, autenticación obligatoria y ACL de nodo/operador/observador; verificar arranque y conexiones válidas por ambos listeners.
- [x] 5.4 Comprobar de forma breve rechazo anónimo, contraseña incorrecta, CA/hostname no confiables y publicación fuera del nodo o desde observador; verificar que no se entregue el mensaje denegado.
- [ ] 5.5 Configurar volumen persistente, enlace loopback por defecto y parámetro explícito para LAN autorizada; verificar reinicio conservando estado y acceso LAN solo tras configurarlo.
- [x] 5.6 Documentar preparación, confianza CA, puertos/LAN/firewall, ejecución y parada en `broker/README.md`; verificar los pasos en entorno local sin publicar puertos del router ni incrustar cuentas admin en el frontend.

## 6. Radio, BLE autenticado y MQTT del dispositivo

- [ ] 6.1 Inicializar Wi-Fi/eventos y NimBLE bajo gestión del supervisor, sin escaneo activo/asociación en Escudo; verificar arranque, habilitación/parada de recursos y BLE anunciado en placa real sin iniciar captura.
- [ ] 6.2 Implementar ventana física, LE Secure Connections/MITM, passkey OLED, bonding y protección de GATT; verificar con Android un pairing válido, nivel de seguridad negociado y rechazo de un cliente nuevo fuera de ventana.
- [ ] 6.3 Implementar metadatos/entrada/salida GATT y framing de D-05 conectado al gestor real de comandos; verificar lectura inicial, `get_state`, respuesta completa y descarte de mensaje parcial al desconectar.
- [ ] 6.4 Implementar `configure_connectivity` y `set_time` solo por BLE protegido, parámetros privados en RAM y respuestas sanitizadas; verificar aplicación de datos de laboratorio sin persistir o publicar contraseñas.
- [ ] 6.5 Implementar STA y esp-mqtt en Cebo con CA/hostname/fecha validados, autenticación, MQTT 5 y reintentos no bloqueantes; verificar conexión real al broker del grupo 5 y fallo seguro ante certificado incorrecto.
- [ ] 6.6 Implementar topics/QoS, LWT, estado periódico, comandos no retenidos y reentrega de eventos del ring; verificar una orden MQTT aplicada, un duplicado sin doble efecto y rechazo de comando retenido.
- [ ] 6.7 Integrar cambio de perfil con cierre STA/MQTT, control BLE y reconciliación de confirmación final; verificar ida/vuelta Cebo–Escudo sin asociación en Escudo ni recursos simultáneos.
- [ ] 6.8 Documentar pairing, confianza TLS, reloj, reprovisionamiento tras reboot y errores/reconexión en `docs/connectivity.md`; verificar los pasos de configuración en placa real sin omitir verificaciones de seguridad.

## 7. Aplicación Android y servicios de comunicación

- [x] 7.1 Añadir Capacitor Android y plugin BLE compatible, versionando código/configuración nativa y wrapper; verificar build web, sincronización y `assembleDebug` sin registrar artefactos generados.
- [ ] 7.2 Implementar adaptador BLE real con permisos, bonding, metadatos, framing y operaciones de base; verificar teléfono real, error de permisos y orden/respuesta desde el ESP32.
- [x] 7.3 Implementar adaptador mqtt.js por WSS con identidad de operador/observador en memoria, CA confiable y contratos v1; verificar conexión real, rechazo de credenciales incorrectas y ausencia de secretos en bundle/estado persistente.
- [x] 7.4 Implementar store de nodo/revisión/transportes y reconciliación de comandos con cambio de canal; verificar que recepción/publicación no se confunda con aplicación y que el estado antiguo se marque.
- [ ] 7.5 Implementar UI de conexión/configuración, perfil/fase/capacidades y eventos de sistema; verificar que muestre datos reales y funciones Escudo/Cebo/notificaciones pendientes sin indicadores de seguridad fabricados.
- [x] 7.6 Implementar historial acotado en IndexedDB, cursor, replay y deduplicación; verificar evento repetido, reinicio de app y gap reportado sin duplicar historial.
- [ ] 7.7 Documentar instalación Android, configuración CA de laboratorio y flujo BLE/MQTT en `app/README.md`; verificar build/sync/instalación y recorrido breve en teléfono real, sin exigir soporte BLE de todos los navegadores.

## 8. Comprobación integral y revisión de alcance

- [x] 8.1 Ejecutar compilaciones finales de firmware, contratos y app/Android usando los comandos documentados; verificar resultados y que una instalación desde lockfiles no dependa de archivos generados versionados.
- [ ] 8.2 Ejecutar recorrido real con ESP32/OLED/botón/Android: pairing, estado, configuración, Cebo conectado, orden MQTT y retorno a Escudo por canal disponible; verificar exclusión de recursos y capacidades pendientes de módulos.
- [ ] 8.3 Desconectar BLE/broker, generar un evento real de transición, reiniciar brevemente y recuperar; verificar persistencia, reconexión, deduplicación y controles físicos sin benchmarks.
- [x] 8.4 Revisar diff y archivos nuevos, secretos, configuraciones/lockfiles, enlaces y coherencia de specs; verificar que no se implementaron detectores/honeypot/NAT/notificaciones fuera del cambio ni se alteró su obligatoriedad en R-11.
- [x] 8.5 Registrar comandos, dispositivo/entorno y resultados de la comprobación integral en `docs/core-foundation-check.md`; verificar que toda prueba no ejecutada quede explícitamente pendiente y no se marque completada con simulación.

## Workflow follow-up

- Presentar el cambio para revisión de otro integrante según CONTRIBUTING.md; no crear commits, push, merges ni eliminar ramas sin autorización.
- Continuar con propuestas independientes para Escudo, Cebo y notificaciones del MVP completo, conservando sus funcionalidades.
- Archivar esta base solo después de implementar y revisar sus tareas; crear los artefactos no completa la implementación.
