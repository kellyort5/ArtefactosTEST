# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

## Context

La instalación existente usa core3.3.12, wolfMQTT2.2.0 y tres archivos de formato/configuración Arduino. Ver proposal.md y project-workspace. El usuario autoriza automatizar ambos pasos preservando la configuración y protecciones aprobadas. Existe trabajo local anterior que debe conservarse.

## Goals / Non-Goals

Preparación inicial sencilla en Windows PowerShell5.1, sin administrador, Python, wolfSSL ni nuevos componentes de firmware. No modificar contratos, librerías TLS ni versiones del runtime. La política posterior autoriza tabla nueva y cuatro claves mínimas, retirando cinco overrides Sentinel antiguos con claves ajenas/respaldo/idempotencia. No compilar ni subir automáticamente a una placa.

## Decisions

- install-wolfmqtt.ps1 descarga ZIP original2.2.0 y comprueba SHA256 fijado en dependencies.json. Se conserva el registro tar.gz previo para mantenimiento existente; se añade metadato ZIP e inventario de hashes de fuentes/headers/licencias, no código del proveedor. ArchivePath permite usar el ZIP original ya descargado y validar el mismo hash.
- Construir un staging hermano fuera del repositorio, añadir los tres archivos existentes y verificar inventario exacto antes de sustituir el destino. Mover la instalación previa a Sketchbook/.sentinel-backups, fuera de libraries, con restauración si la activación falla. La ejecución correcta previa se detecta con hashes sin red ni escritura; no se necesita marker propio.
- install-board-config.ps1 combina exactamente las claves existentes usando escritura temporal y reemplazo con respaldo; conserva líneas ajenas/comentarios y consolida duplicados Sentinel. Exige boards.txt y versión3.3.12 presentes, sin instalar/modificar el core.
- Un helper pequeño compartido resuelve rutas Arduino desde su CLI/configuración y realiza verificaciones comunes. Se admiten rutas explícitas para Sketchbook y datos, incluidas pruebas aisladas; los proveedores nunca se instalan dentro del repositorio. Usar .NET/PowerShell estándar; no distribuir ZIP modificado ni duplicar un SDK.
- Soportar WhatIf y fallar con diagnóstico ante hash incorrecto/archivo bloqueado. No cerrar el IDE ni procesos del usuario; documentar reinicio para que reconozca cambios. Los scripts solo se usan al preparar o actualizar el entorno, no por build.

## Risks / Trade-offs

Descarga fallida o bloqueo de archivos → conservar destino intacto hasta validar staging y restaurar ante error. Rutas personalizadas → consultar configuración real del IDE y permitir override explícito. La preparación necesita PowerShell permitido en el proceso → documentar Bypass de alcance de proceso sin cambiar políticas globales. El reemplazo de una carpeta no es una transacción de sistema de archivos frente a corte eléctrico: el respaldo y la siguiente ejecución permiten recuperación; no prometer atomicidad ante apagado.

## Migration Plan

Probar descarga y reparación en directorios temporales Windows; comprobar idempotencia y preservación de claves/configuración. Comparar antes/después las instalaciones reales mediante WhatIf o no-op, sin reemplazarlas para una prueba. Actualizar guías a dos comandos y conservar procedimiento de recuperación. La lógica del firmware no cambia y no requiere otro build por esta automatización.
