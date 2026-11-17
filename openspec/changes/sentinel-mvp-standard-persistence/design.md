# Design

## Context

Ver propuesta para motivación. El core oficial ESP32 3.3.12 escribe boot_app0.bin en 0xe000 durante carga y exportación normales. La tabla anterior coloca allí el final de NVS y phy_init; nueve claves locales evitan esa escritura y la imagen merged. La configuración de conectividad permanece solo en RAM. El journal, la identidad y los bonds ya usan NVS.

## Goals / Non-Goals

Conservar datos confirmados en reinicios/cortes normales sin exigir continuidad entre cargas USB. Mantener todos los modos, contratos y seguridad. No implementar módulos pendientes, OTA, cifrado de flash, migraciones entre versiones ni un framework de almacenamiento. No grabar una placa como efecto de preparar o compilar.

## Decisions

### Distribución compatible con direcciones de Arduino

Tabla del sketch para flash 16MiB:

| Partición | Tipo/subtipo | Inicio | Tamaño |
| --- | --- | --- | --- |
| otadata | data/ota | 0xe000 | 0x2000 |
| factory | app/factory | 0x10000 | 0x400000 |
| nvs | data/nvs | 0x410000 | 0x6000 |
| event_log | data/nvs | 0x416000 | 0x40000 |

Reservar la región boot_app0 elimina la colisión sin reducir app4MiB, NVS24KiB o journal256KiB/64 eventos. otadata no añade OTA. El bootloader oficial selecciona factory cuando no hay apps OTA. El SDK oficial incorpora PHY init en la app; su calibración persiste en NVS. NVS queda antes del otro data/nvs para compatibilidad con búsquedas del core. Validar tabla con generador oficial y artefactos de build. Se descarta la alternativa de NVS20KiB en0x9000 por reducir capacidad y la alternativa de mantener recetas especiales por complejidad innecesaria bajo la política aprobada.

### Cuatro claves locales en lugar de nueve

Conservar etiqueta del menú, build.partitions=sentinel, upload.maximum_size=4194304 y enlace --wrap=nvs_flash_init. El selector permite verificar configuración y el límite protege el tamaño real de app. Retirar upload.pattern_args y cuatro overrides postobjcopy/postsavehex para volver a recetas estándar. El instalador elimina solo esas cinco claves propias, conserva claves/comentarios ajenos, crea respaldo antes de modificar y es idempotente. No editar core, SDK ni compilador.

### Protección mínima de arranque NVS

Conservar el wrapper existente: initArduino llama a nvs_flash_init antes de setup y borra automáticamente NVS ante NO_FREE_PAGES/NEW_VERSION_FOUND. setup/initVariant son demasiado tardíos; NVS propia no protege bonds internos. La protección es contra formato silencioso al reiniciar, no compatibilidad entre versiones. Errores se muestran y no autorizan borrar durante arranque normal.

### Configuración persistente con NVS oficial

Guardar una entrada completa y acotada de conectividad validada; confirmar solo después de commit. Fallo de guardado mantiene la configuración anterior. Restaurar tras validación al iniciar, permaneciendo en Idle y sin asociación. No considerar vigente la hora UTC anterior; seguir requiriendo una fuente de tiempo válida para TLS. Ausencia es estado no configurado, corrupción/error es visible sin formato. Reutilizar APIs oficiales y un helper pequeño testeable, sin nuevas dependencias. Credenciales autorizadas y CA pueden residir en NVS estándar: no se promete cifrado físico; nunca logs, historial, estado público o Git. No almacenar credenciales recibidas por Cebo.

### Selección de objetivo Escudo

La selección SSID/BSSID/canal también es configuración aceptada y actualmente se pierde al salir del modo. Guardarla mediante un helper pequeño del núcleo con NVS oficial y registro acotado, validado y commit, sin framework ni cambio wire. Confirmar selección solo tras guardar; fallo conserva objetivo anterior y deja captura segura. Recuperar al volver a Escudo con captura detenida; no guardar catálogo, uptime ni evidencia temporal. Los datos de referencia de seguridad de la selección, si se conservan, son contexto anterior y nunca observaciones nuevas. Mantener propiedad central del almacenamiento y pruebas del camino real de selección/recuperación/fallos, conservando regresiones.

## Risks / Trade-offs

- Cambio incompatible de direcciones → primera programación limpia, pérdida USB aceptada y nuevo emparejamiento/configuración; sin migración automática.
- Escritura interrumpida → NVS commit, entradas limitadas, validación al recuperar y pruebas focalizadas de fallo; no prometer una escritura que no terminó.
- Configuración inválida o falta de espacio → error explícito, conservar configuración anterior y no borrar NVS.
- Flash sin cifrar → no prometer confidencialidad ante extracción física; preservar seguridad de canales y privacidad de mensajes.
- Hardware no comprobado → separar compilación/regresión verificadas de pruebas breves reales pendientes; no afirmar durabilidad física por un mock.

## Migration Plan

1. Instalar las cuatro claves con respaldo y retirar las cinco antiguas propias.
2. Compilar con core oficial3.3.12 y tabla nueva; comprobar direcciones/artefactos/ausencia de SDK propio.
3. En la primera carga de esta distribución, seleccionar borrado completo desde Arduino y subir por USB; documentar pérdida permitida, no automatizar sobre dispositivo no identificado.
4. Emparejar de nuevo, configurar y producir eventos. Reiniciar/cortar alimentación sin subir y comprobar recuperación. Pruebas físicas pendientes explícitas si no hay equipo disponible.
5. Actualizaciones/reinstalaciones USB posteriores también pueden inicializar datos. Imagen merged sirve para inicialización, no garantiza preservar datos. Volver a un firmware con otra tabla requiere otra carga limpia.
