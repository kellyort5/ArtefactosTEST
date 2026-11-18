# Design

## Context

Ver proposal y deltas. Se integra el commit P1 b63772a (incluidas correcciones de memoria/pila), el retiro TLS MQTT y origin/develop. No archivar cambios ni cerrar comprobaciones físicas por integrar fuentes.

## Goals / Non-Goals

Una base ejecutable y documentada sin sistema de cuentas/roles MQTT o login app. Mantener tres módulos, BLE seguro, Wi-Fi con su contraseña, HTTPS/DoH, claves propias de proveedores y permisos Android necesarios. El usuario del proceso Mosquitto y permisos de archivos del SO no son cuentas MQTT y no se eliminan.

## Decisions

- Compose usa imagen/entrypoint oficiales, allow_anonymous true y ningún password_file/acl_file. Retirar scripts de preparación y sus tests de cuentas, no borrar .secrets/volumen existentes; no son consumidos por el despliegue nuevo. Conservar ayudante Compose por compatibilidad Windows/WSL, sin generación de cuentas.
- Firmware utiliza NetworkClient y wolfMQTT5 sin credenciales CONNECT. Configuración NVS versión3 con solo red Wi-Fi y URI; no migrar ni borrar en arranque. BLE puede reaprovisionar; USB puede reinicializar según política vigente. No conservar campos vacíos de usuario/contraseña simulando su retirada.
- Contracts se generan desde v1 y el generador v2; mantener rechazo estricto de extras y regenerar C++. El contexto de admisión conserva autenticación solo para BLE y aplica los gates de mensajes también en MQTT anónimo. Nunca atribuir aislamiento al schema/topic/device_id.
- App elimina rol del store/controller/transporte y campos de cuenta. Un único cliente BLE y el panel P1 se conservan. Acceso MQTT requiere estado vivo de Cebo para control y no admite operaciones BLE exclusivas.
- P1 conserva núcleo, contratos, Escudo, diagnóstico y coordinación. P2 Sanitizador, P3 Cebo/alertas y P4 pantallas finales/broker físico/laboratorio. No asignar a P4 reimplementar la base o cuentas/ACL retiradas; su encargo broker conserva pruebas LAN/notificaciones. Actualizar fichas y planificación activa con el mismo dueño y números de tarea.
- Históricos, archivados e informes de compilación anteriores siguen como evidencia fechada, con aviso/enlace al estado vigente. No falsificar resultados ni editar sus cifras para hacerlos coincidir con el producto nuevo.

## Risks / Trade-offs

Cualquier cliente con alcance al broker puede leer/publicar, suplantar telemetría y ordenar Cebo: aceptado por el usuario para LAN de confianza sin exposición Internet. La validación de mensajes no mitiga suplantación. Wi-Fi aislada/firewall requiere red utilizable y configuración local; no prometer eludir restricciones.

## Migration Plan

Conservar checkpoint MQTT y rama P1; resolver conflictos explícitamente, revisar specs, aplicar retirada coordinada, probar contratos/firmware/app/broker real, compilar Arduino y Android. Configuración anterior se rechaza y se reaprovisiona; no borrar bonds/identidad/historial. Publicar PR hacia develop y actualizarlo conforme autorización del usuario y reglas remotas; no modificar main, eliminar ramas ni forzar historial.
