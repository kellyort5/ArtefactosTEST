# Spec Delta

## ADDED Requirements

### Requirement: Diagnóstico P1 optativo sobre BLE real
La app SHALL ofrecer, solo al activar su opción de pruebas, un panel para operaciones públicas P1 mediante el transporte BLE existente y contrato v2. SHALL mostrar resultados reales correlacionados sin simular éxito, conservar secretos o habilitar servicios ausentes. La app de producto SHALL mantener el panel desactivado por defecto.

#### Scenario: Descubrir y controlar Escudo sin pantalla específica
- **WHEN** se conecta un nodo autorizado y se envían scan, catálogo, selección o captura desde el panel
- **THEN** genera sobre vigente con metadatos reales y framing compartido y muestra ACK, resultados paginados, estado y métricas recibidos
- **AND** solo un ACK aplicado o reconciliación válida acredita aplicación

#### Scenario: Rechazos controlados y reentrega explícita
- **WHEN** el usuario solicita una prueba pública de boot/revisión/caducidad incorrectos o reenvío exacto
- **THEN** envía únicamente por BLE protegido y muestra la respuesta real del nodo
- **AND** no repite automáticamente modificaciones ni afirma que un rechazo o un envío pendiente sea éxito

#### Scenario: Sin canal o sin opción de pruebas
- **WHEN** BLE está desconectado o la opción de diagnóstico está deshabilitada
- **THEN** impide ejecutar el panel y no utiliza MQTT como sustituto del Escudo

#### Scenario: Privacidad de diagnóstico
- **WHEN** se intenta usar el panel para aprovisionamiento o se suministra JSON inválido
- **THEN** rechaza con error sanitizado sin divulgar contenido privado
- **AND** los resultados de diagnóstico son acotados y efímeros; la recuperación de eventos usa el historial real existente
