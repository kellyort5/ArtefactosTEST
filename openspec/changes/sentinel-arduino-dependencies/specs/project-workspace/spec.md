# Spec Delta

## ADDED Requirements

### Requirement: Compilación autónoma del firmware

El firmware SHALL poder compilarse y prepararse para carga desde Arduino IDE con dependencias distribuidas por sus proveedores, sin exigir reconstruir un SDK propio ni ejecutar scripts del proyecto manualmente para cada compilación.

#### Scenario: Equipo de desarrollo nuevo

- **WHEN** un integrante instala las dependencias documentadas y abre el sketch del repositorio
- **THEN** puede verificar y preparar la carga del firmware completo desde el IDE conservando las funciones y contratos vigentes
- **AND** la documentación distingue las herramientas internas del IDE de los comandos de mantenimiento opcionales
