# project-workspace Specification

## Purpose

Mantener una base reproducible y separada para compilar, configurar y ejecutar los componentes de Sentinel Node sin introducir dependencias entre sus implementaciones.

## Requirements

### Requirement: Componentes independientes y configuración reproducible
El repositorio SHALL separar firmware, broker, aplicación, contratos y simulador, conservar los documentos existentes y registrar configuración fuente, versiones y lockfiles necesarios para reproducir sus compilaciones.

#### Scenario: Preparación desde el repositorio
- **WHEN** un integrante sigue las instrucciones desde una copia del repositorio
- **THEN** puede localizar y preparar cada componente sin reemplazar documentación o código de otros componentes
- **AND** las dependencias se resuelven mediante las versiones y lockfiles versionados

### Requirement: Configuración local sin secretos versionados
La base SHALL permitir configurar equipo, red autorizada, endpoints y autenticación sin incluir secretos reales en código, ejemplos, artefactos versionados o logs.

#### Scenario: Entorno nuevo sin credenciales
- **WHEN** no existe configuración privada local
- **THEN** las compilaciones no contienen credenciales predeterminadas ni accesos anónimos alternativos
- **AND** la ejecución que necesita secretos informa cómo suministrarlos sin imprimir su contenido

### Requirement: Instrucciones ejecutables de desarrollo
La documentación SHALL incluir prerrequisitos y comandos para preparar dependencias, compilar/grabar/monitorizar firmware, ejecutar broker protegido, compilar app, sincronizar Android y efectuar la comprobación funcional básica.

#### Scenario: Comprobación de la base
- **WHEN** se siguen las instrucciones con ESP32-S3 N16R8 y teléfono Android reales
- **THEN** se puede verificar estado, comando, respuesta y periféricos por los canales pertinentes
- **AND** se registran resultados reales o limitaciones explícitas sin declarar probadas funciones simuladas

### Requirement: Separación entre base y módulos funcionales
La base SHALL identificar como no implementadas las capacidades de módulos todavía ausentes, conservando sus puntos de extensión y anunciando únicamente las funcionalidades realmente disponibles.

#### Scenario: Consulta inicial de capacidades
- **WHEN** el firmware y la app presentan el estado de la base
- **THEN** distinguen conectividad y perfiles de recursos de las capacidades de detección y honeypot realmente disponibles
- **AND** no inician servicios todavía ausentes ni publican amenazas ficticias como reales

### Requirement: Compilación autónoma del firmware

El firmware SHALL poder compilarse y prepararse para carga desde Arduino IDE con dependencias distribuidas por sus proveedores, sin exigir reconstruir un SDK propio ni ejecutar scripts del proyecto manualmente para cada compilación.

#### Scenario: Equipo de desarrollo nuevo

- **WHEN** un integrante instala las dependencias documentadas y abre el sketch del repositorio
- **THEN** puede verificar y preparar la carga del firmware completo desde el IDE conservando las funciones y contratos vigentes
- **AND** la documentación distingue las herramientas internas del IDE de los comandos de mantenimiento opcionales

### Requirement: Persistencia operativa y programación USB estándar
El firmware SHALL conservar identidad, vínculos BLE, configuración e historial confirmado durante operación normal, reinicios y cortes de alimentación. La programación USB SHALL permitir reinicializarlos, sin exigir migración entre versiones. La preparación SHALL priorizar compilación, carga y exportación estándar de Arduino, documentando cada personalización restante.

#### Scenario: Carga con distribución de flash nueva
- **WHEN** se prepara la primera carga USB con la distribución nueva
- **THEN** las instrucciones indican inicialización limpia y posterior configuración y emparejamiento
- **AND** no prometen conservar los datos de la distribución anterior

#### Scenario: Instalación sobre configuración local anterior
- **WHEN** se ejecuta de nuevo la preparación documentada de Arduino
- **THEN** retira las recetas propias obsoletas de carga y exportación conservando claves ajenas y respaldo
- **AND** repetir la instalación correcta no mezcla configuraciones ni cambia innecesariamente el archivo

#### Scenario: Reinicio ordinario con error de almacenamiento
- **WHEN** el dispositivo encuentra un error de inicialización NVS durante un arranque sin programación USB
- **THEN** informa el fallo sin formatear automáticamente datos persistentes
- **AND** la excepción de reinstalación USB no habilita un borrado durante ese arranque
