## Purpose

Mantener una base reproducible y separada para compilar, configurar y ejecutar los componentes de Sentinel Node sin introducir dependencias entre sus implementaciones.

## ADDED Requirements

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
La base SHALL identificar como no implementados los detectores Escudo, servicios Cebo y Sanitizador, conservando sus puntos de extensión sin anunciarlos como funcionalidades disponibles.

#### Scenario: Consulta inicial de capacidades
- **WHEN** el firmware y la app presentan el estado de la base
- **THEN** distinguen conectividad y perfiles de recursos de detección y honeypot aún ausentes
- **AND** no inician servicios específicos ni publican amenazas ficticias como reales
