# Spec Delta

## ADDED Requirements

### Requirement: Instalación verificada de dependencias Arduino

La preparación inicial SHALL instalar la versión fijada de la biblioteca MQTT desde el proveedor verificando su hash y aplicando la configuración versionada del proyecto. El código del proveedor SHALL permanecer fuera del repositorio y del sketch, sin incorporar otro proveedor TLS.

#### Scenario: Biblioteca ausente
- **WHEN** se ejecuta la preparación sin la biblioteca instalada
- **THEN** descarga el archivo fijado, verifica su integridad e instala la estructura Arduino y configuración completas en el Sketchbook elegido

#### Scenario: Archivo incorrecto
- **WHEN** la descarga o archivo suministrado no coincide con el hash fijado
- **THEN** la operación falla antes de modificar la biblioteca existente

### Requirement: Preparación repetible y recuperación

Los instaladores SHALL reconocer un destino ya correcto sin descargar ni modificar archivos. Una biblioteca incompleta o distinta SHALL reemplazarse por una preparación completa validada, conservando respaldo fuera de libraries y recuperando la instalación anterior ante un fallo de sustitución.

#### Scenario: Segunda ejecución
- **WHEN** se repite la preparación con todos los archivos correctos
- **THEN** termina sin descargar, crear respaldos ni cambiar contenidos o fechas

#### Scenario: Instalación mezclada
- **WHEN** existen archivos antiguos, faltantes o modificados en la biblioteca
- **THEN** conserva un respaldo y activa una sola instalación completa sin archivos sobrantes

### Requirement: Configuración Arduino conservadora

La preparación SHALL aplicar las claves aprobadas únicamente a la versión fijada del core oficial, conservando comentarios y claves ajenas. SHALL respaldar cambios, evitar claves Sentinel duplicadas y no modificar binarios, código del core, flash ni datos del dispositivo.

#### Scenario: Configuración previa
- **WHEN** existen opciones ajenas y claves Sentinel desactualizadas o repetidas
- **THEN** conserva las opciones ajenas y registra una sola definición actual por clave Sentinel con respaldo recuperable

#### Scenario: Core ausente
- **WHEN** no está instalada la versión oficial fijada
- **THEN** informa cómo instalarla y no crea una instalación parcial del core

#### Scenario: Retirada de recetas Sentinel obsoletas
- **WHEN** se actualiza una configuración local Sentinel de nueve claves según sentinel-mvp-standard-persistence
- **THEN** conserva solo menú, build.partitions, límite4MiB y wrapper NVS, y elimina cinco overrides propios de carga/exportación
- **AND** preserva claves ajenas y respaldo, es idempotente y no programa una placa
