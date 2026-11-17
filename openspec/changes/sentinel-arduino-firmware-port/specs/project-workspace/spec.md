# Spec Delta

## MODIFIED Requirements

### Requirement: Instrucciones ejecutables de desarrollo
El desarrollo, compilación, subida y monitor del firmware SHALL usar Arduino IDE Windows con el core oficial y las dependencias/configuración fijadas; aplicación, broker y simulador conservan sus herramientas. La documentación SHALL incluir prerrequisitos y comandos para preparar dependencias, compilar/grabar/monitorizar firmware, ejecutar broker protegido, compilar app, sincronizar Android y efectuar la comprobación funcional básica.

#### Scenario: Comprobación de la base
- **WHEN** se siguen las instrucciones con ESP32-S3 N16R8 y teléfono Android reales
- **THEN** se puede verificar estado, comando, respuesta y periféricos por los canales pertinentes
- **AND** se registran resultados reales o limitaciones explícitas sin declarar probadas funciones simuladas

#### Scenario: Flujo Arduino para todos los módulos

- **WHEN** un integrante prepara Escudo, Sanitizador o Cebo
- **THEN** usa la misma plataforma Arduino, API y configuración reproducible sin instalar otro supervisor ni un core por módulo
