# Verificacion de sentinel-arduino-installers

**Aviso posterior de vigencia:** este registro conserva resultados, comandos, hashes y pendientes históricos. [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) sustituye las exigencias anteriores de RAM-only, continuidad USB y tabla inmutable: NVS explícita para configuración autorizada, tabla nueva/cuatro claves y carga/exportación oficial. Primera carga limpia desde IDE; USB permite reinicializar sin migración. Los resultados de este informe no acreditan la nueva tabla ni el build integrado nuevo, pendiente de evidencia del coordinador; los pendientes físicos siguen abiertos.

## Resultado

Dos instaladores PowerShell5.1 listos: install-wolfmqtt.ps1 e install-board-config.ps1, helper compartido y prueba focalizada. El primero obtiene wolfMQTT2.2.0 original y verifica el ZIP/inventario; el segundo combina las nueve claves existentes. No cambian versiones, MQTT5, transportes TLS, particiones, contratos ni codigo funcional del firmware. Sin commits/push/merge o archivo de cambios.

## Comprobaciones Windows

Se ejecutaron scripts reales mediante Windows PowerShell5.1 de Windows/System32, sin Pest ni herramientas nuevas. El runner firmware/test/installers_test.ps1 usa exclusivamente directorios temporales Windows con espacios en sus rutas.

- Instalacion desde ZIP local y descarga HTTPS real original: PASS. Descarga nativa registrada en /tmp/sentinel-installers-native-test.log; la instalacion aplica los tres archivos fuente de Sentinel.
- Repeticion sin ZIP disponible: PASS sin cambios de contenidos, fechas ni respaldos; retorna antes de descargar.
- Instalacion alterada y archivo antiguo sobrante: PASS, reemplazo completo verificado y respaldo original fuera de libraries; sin mezcla.
- Hash incorrecto y biblioteca bloqueada: PASS, conserva instalacion/respaldos previos.
- Configuracion con opciones/comentarios ajenos y duplicados: PASS, nueve claves correctas, respaldo exacto y repeticion sin escrituras.
- Claves con mayusculas incorrectas: no se confunden con las claves exactas que requiere Arduino; se agregan las definiciones correctas sin borrar opciones ajenas.
- Core ausente, WhatIf y archivo de configuracion bloqueado: PASS sin instalacion parcial o sobrescritura.

Suite final y recuento de comprobaciones en /tmp/sentinel-installers-native-final-test.log, exit0. La descarga real ya se verifico; las siguientes ejecuciones reutilizaron el ZIP para comprobar cambios de pruebas/validacion sin repetir red innecesariamente. ZIP SHA256: 5e2cbd25502e4935d3c86f8d4693cd063c6bcafe65ef6f5c4d6d3f938e8323a5. Veintiun archivos originales se contrastan con hashes versionados; la configuracion personalizada se obtiene de los tres archivos del repositorio.

## Instalacion real y limites

WhatIf sobre rutas detectadas por el CLI del IDE: PASS. Detecta Sketchbook y Arduino15 personalizados del equipo; la configuracion real ya correcta es no-op y la biblioteca indica la reparacion prevista. Hashes y fechas de los archivos reales permanecieron intactos: /tmp/sentinel-installers-real-preview.json y .log. No se reemplazo una biblioteca de trabajo para probar el instalador ni se conecto/grabo una placa.

Las fuentes compiladas del sketch siguen identicas al build oficial final anterior. El manifest solo agrega metadatos ZIP y hashes de archivos originales; sus campos anteriores se conservan exactamente. Evidencia /tmp/sentinel-installers-boundaries.json. No se repite build de firmware por cambios exclusivamente de preparacion, pruebas y documentacion.

Se actualizaron firmware/SentinelNode/README.md, docs/firmware.md, guia-pruebas-persona-1.md, technical-decisions.md y firmware-arduino-migration.md. firmware/arduino/README.md concentra uso, rutas, ZIP local, WhatIf, respaldos y prueba reproducible. Los scripts no se ejecutan durante Verify/Upload y no instalan wolfSSL ni otro SDK. La politica de ejecucion opcional se documenta solo con alcance Process, sin cambios globales ni elevacion.

Validacion estricta OpenSpec37/37 PASS y revision de enlaces locales sin errores; informes locales /tmp/sentinel-installers-validation.json y /tmp/sentinel-installers-links.json. git diff --check PASS. Los paquetes descargados, proveedores, temporales y respaldos permanecen fuera de Git. El reemplazo conserva recuperacion ante errores normales; no se promete transaccion frente a corte electrico o ejecuciones simultaneas sobre el mismo destino. Cerrar Arduino IDE antes de instalar evita archivos en uso.
