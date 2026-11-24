# Design

> Actualización 2026-11-20: el panel temporal P1 fue retirado por petición del usuario en `sentinel-app-escudo`. Sus tareas y resultados siguientes son evidencia histórica, no instrucciones para reinstalarlo. Se retiró su delta mobile-control para que un archivado posterior no lo restaure. La salida serie, las correcciones BLE y los pendientes físicos de firmware se conservan.

## Context

El usuario autoriza la visualización serie temporal y un panel P1 BLE. Se trabaja en checkout aislado desde b1f8729 para preservar MQTT LAN concurrente. Ver propuesta y deltas; no cambiar protocolo, modelo de modos ni políticas de NVS/MQTT.

## Goals / Non-Goals

Habilitar la guía P1 sin OLED y sin esperar pantallas específicas. No acreditar OLED física, integración futura P2/P3/P4, radio ni ataques sin pruebas reales. No introducir otro servidor, transporte o dependencia; no reemplazar la autorización BLE por USB.

## Decisions

- `SENTINEL_SERIAL_TEST_DISPLAY=0` por defecto. Backend local pequeño con callbacks, sin fingir has_oled. En modo1 muestra estado por serie acotado/deduplicado y un código por intento real durante la ventana física; fallo de salida cierra ventana. Preparar Serial115200 desde Arduino. El flujo normal no imprime códigos.
- Integrar backend con el supervisor existente: pulsación abre ventana con una salida local habilitada; la entrega válida del código se exige antes de confirmar ble_passkey, limpieza al desconectar/caducar. No imprimir contraseñas, configuraciones privadas ni material de bonds. Se justifica una excepción de visualización local del código, no un sistema de logs.
- Contador temporal en el camino real inmediato a Detectors::ingest, expuesto por serie como evidencia T08. Sin tramas crudas, alteración de detectores, persistencia ni contratos adicionales; coste constante y salida periódica acotada.
- Panel de pruebas con `VITE_SENTINEL_P1_TOOLS=1`, desactivado normalmente. Reutiliza controller/BleTransport, codecs, metadatos protegidos y store/journal reales. Lista cerrada de operaciones públicas P1 y la misma cola serializada de órdenes del controller. Los negativos omiten únicamente el filtro de capacidades de la UI; el gate y autorización del firmware permanecen vigentes. Un observador de mensajes validados permite mostrar ACK/resultados/métricas sin mocks ni un segundo cliente BLE.
- Opciones negativas deliberadas (boot/revisión/caducidad) y reenvío exacto explícito para comprobar gate/dedup. Ninguna afecta al envío normal de producto ni permite configurar credenciales. Resultado en memoria limitada del panel, borrado al cambiar nodo/arranque/canal. No confundir reenvío exacto con el reintento automático prohibido.
- Preparar APK de diagnóstico y firmware de prueba con las opciones documentadas. Mantener código fuente por defecto seguro y distinguir perfiles de compilación. Compilar Windows con core oficial, y build/test app/Android; copiar entradas a temporal Windows cuando el checkout reside en WSL, sin alterar receta o SDK.

## Risks / Trade-offs

- Código visible al operador USB → habilitación explícita de prueba, ventana física y no guardar consola/capturas con código.
- Fallo de entrega al driver UART → no confirmar pairing. UART no puede verificar lectura humana o monitor abierto: abrirlo antes de emparejar y no guardar su código. Ningún fallo habilita Just Works.
- Casos negativos podrían modificar el modo → usuario los solicita explícitamente en red autorizada; ACK real, no ejecución automática ni overrides en producto.
- Hardware/ataques o módulos ausentes → matriz de pruebas con evidencia requerida; OLED y servicios futuros pendientes, sin reducir su alcance.

## Migration Plan

Cambios aditivos de firmware/app sin migración de datos ni nueva tabla. Activar opciones solo para las pruebas P1 y volver a desactivarlas al conectar OLED. USB puede reinicializar datos conforme a la política vigente; primera carga de tabla nueva limpia, no borrar para cada RESET.

## Corrección de recursos observada durante la prueba física

La caché de32 comandos se ubica explícitamente en PSRAM mediante la API oficial, sin reducir capacidad ni modificar SDK/core. Se comprueban resultados de inicialización BLE y tareas. El código canónico y los resultados grandes de callbacks se reservan fuera de la pila host de5120bytes; falta de memoria rechaza la operación sin fugas y el JSON temporal se limpia al liberar. No se cambia autorización, protocolo, gate ni NVS. Evidencia, auditoría de pila, imagen final y límites en [la verificación de correcciones](ble-stack-verification.md).
