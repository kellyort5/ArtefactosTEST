# Spec Delta

## MODIFIED Requirements

### Requirement: Separación entre base y módulos funcionales
La base SHALL identificar como no implementadas las capacidades de módulos todavía ausentes, conservando sus puntos de extensión y anunciando únicamente las funcionalidades realmente disponibles.

#### Scenario: Consulta inicial de capacidades
- **WHEN** el firmware y la app presentan el estado de la base
- **THEN** distinguen conectividad y perfiles de recursos de las capacidades de detección y honeypot realmente disponibles
- **AND** no inician servicios todavía ausentes ni publican amenazas ficticias como reales
