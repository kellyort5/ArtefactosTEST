# Spec Delta

## Purpose

La base v1 rechaza campos funcionales nuevos; los tres autores de firmware y la app necesitan un vocabulario común antes de integrar.

## ADDED Requirements

### Requirement: Mensajes funcionales versionados

El sistema SHALL definir mensajes v2 cerrados de estado, métricas, eventos, órdenes y confirmaciones para los tres módulos, con límites de tamaño y unidades explícitas.

#### Scenario: Mensajes funcionales versionados verificable

- **WHEN** se valida un evento real de cada familia y un campo desconocido
- **THEN** los eventos válidos se aceptan y el mensaje con campos desconocidos se rechaza

### Requirement: Procedencia explícita de eventos funcionales

Los eventos v2 SHALL incluir origin con valor real o simulated y conservarlo al recuperar o retransmitir el evento. Los eventos históricos v1 SHALL mantener su formato e identidad originales sin inferir procedencia no acreditada.

#### Scenario: Procedencia conservada durante recuperación

- **WHEN** se recibe y retransmite un evento v2 simulado junto a un evento histórico v1
- **THEN** el evento v2 conserva origin simulated y el evento v1 permanece intacto
- **AND** un evento v2 sin origin o con un valor no admitido se rechaza

### Requirement: Compatibilidad explícita

La app nueva SHALL reconocer la base v1 y los eventos históricos v1; el firmware v2 SHALL rechazar órdenes de versiones no soportadas sin aplicar efectos.

#### Scenario: Compatibilidad explícita verificable

- **WHEN** se conecta una app nueva a la base y luego a firmware v2 con un evento antiguo
- **THEN** se mantiene el control básico v1, se negocia v2 y el evento antiguo conserva identidad

### Requirement: Órdenes seguras y paginadas

El sistema SHALL correlacionar órdenes y resultados, rechazar replays inválidos y paginar catálogos e historial sin superar 2048 bytes por mensaje.

#### Scenario: Órdenes seguras y paginadas verificable

- **WHEN** una orden se repite y se solicita una página que excedería el tamaño
- **THEN** no se repite el efecto y la página se reduce o devuelve error explícito

### Requirement: Privacidad de mensajes

Los contratos SHALL excluir credenciales capturadas y cuerpos completos de eventos y definir configuración secreta solo por un canal autenticado autorizado.

#### Scenario: Privacidad de mensajes verificable

- **WHEN** se intenta validar un evento con contraseña o configurar credenciales por MQTT
- **THEN** se rechazan ambas entradas sin reflejar el secreto

### Requirement: Contrato de entrega móvil

El sistema SHALL definir registro y baja autenticados por nodo y una notificación correlacionada al evento, sin devolver ni registrar el token de entrega.

#### Scenario: Contrato de entrega móvil verificable

- **WHEN** una instalación registra su token y solicita otro nodo
- **THEN** se registra para su identidad y se rechaza el acceso cruzado
