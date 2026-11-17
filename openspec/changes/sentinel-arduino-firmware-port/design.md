# Design

**Política posterior aplicable:** [sentinel-mvp-standard-persistence](../sentinel-mvp-standard-persistence/design.md) reemplaza RAM-only, continuidad USB y tabla inmutable si aparecen en el plan previo. Identidad/bonds/configuración/historial confirmado persisten en uso normal/reinicio/corte; USB permite reinicialización sin migración. Primera carga de tabla nueva limpia desde IDE; cuatro claves mínimas y recetas oficiales. Conservar responsables, alcance completo y pendientes físicos. Las evidencias previas son históricas; el build integrado nuevo requiere evidencia nueva.

> **Ruta vigente:** la simplificación aprobada sustituye la dependencia del paquete Sentinel por el core oficial y sentinel-arduino-dependencies. El sketch distribuye v2_validator.h; no requiere generación por build. Las decisiones/evidencias originales siguientes describen el port previo y se conservan como antecedente; instalación actual en [firmware](../../../docs/firmware.md).

## Context

El usuario ordena ejecutar el plan, para todos los módulos y sin perder Escudo. Mantener API, protocolos y todo el alcance aprobado. Depende del SDK personalizado para enlace final; fuentes/tests pueden migrarse en paralelo con propiedad de archivos explícita.

## Goals / Non-Goals

**Goals:** árbol único modular Arduino, build/subida/monitor Windows, regresión completa Escudo y puntos de extensión de todos los modos.
**Non-Goals:** reescribir heurísticas, reducir módulos, convertir servicios ausentes en mocks o cambiar app/broker/contratos.

## Decisions

Mover componentes a firmware/SentinelNode/src preservando lógica, firmas y factories. Arduino aporta app_main; setup llama una vez sentinel::start y loop cede CPU. Config propia para pines/módulos, flags SDK reales permanecen en paquete. Mantener generación v2_validator.inc por hook y esquema único; incluir header de reserva BLE antes de initArduino. Único gestor Wi-Fi/netifs con driver en RAM y configuración propia validada/persistente en NVS oficial, NAPT exclusivo Sanitizador y MQTT exclusivo Cebo. La tabla nueva está autorizada por la política posterior; identidad/bonds/configuración/historial se conservan en reinicios/cortes normales, sin garantía entre versiones USB. Host tests y fixtures se adaptan solo en rutas/integración, sin aflojar aserciones. Registrar huellas y comparación antes/después de parsers/detectores/scheduler/discovery. Documentación de todos los encargos debe describir Arduino; evidencia histórica no se transforma en prueba actual. SDK/port final se comprueban con CLI nativo del IDE Windows y con el IDE cuando sea accesible. La subida/monitor reales esperan dispositivo identificado.

## Risks / Trade-offs

Movimiento coordinado para no perder trabajo de P2/P3, doble inicialización o autorecuperación destructiva. Los módulos ausentes permanecen no disponibles hasta su entrega. Firmware y tests host son pruebas distintas; las comprobaciones físicas pendientes se documentan sin marcar completadas. Conservar el árbol anterior por referencia Git; no mantener copias divergentes de firmware.
