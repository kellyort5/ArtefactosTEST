# Tasks

Responsable único de todas las casillas: **Persona 1**, con agentes de apoyo bajo integración central. Implementación, comprobaciones técnicas y revisión de consumidor registradas en [contracts/evidence-v2.md](../../../../contracts/evidence-v2.md). El usuario aprobó expresamente el contrato y el cierre de 2.2 el 2026-11-11: «Apruebo el contrato; cerrar 2.2». También eligió sincronizar los seis requisitos y archivar. Las pruebas no sustituyen esa aprobación ni acreditan hardware.

## 1. Esquemas y compatibilidad

- [x] 1.1 Crear esquemas v2 de sobre, estado y métricas; verificar ejemplos de los tres perfiles y rechazo de campos desconocidos con npm run check:contracts.
- [x] 1.2 Definir uniones de eventos y resultados paginados con límites/unidades del diseño; comprobar fixtures válidos y sobredimensionados.
- [x] 1.3 Añadir ramas de comandos y ACK para cada operación de la tabla; comprobar correlación y casos inválidos/secretos.
- [x] 1.4 Extender el codec/exportaciones para v1 y v2 sin modificar semántica v1; verificar fixtures existentes y eventos históricos.
- [x] 1.5 Documentar negociación, topics, paginación y API de registro/push en contracts/README.md; comprobar que cada operación tiene ejemplo y consumidor.

## 2. Contrato listo para consumidores

- [x] 2.1 Añadir fixtures compartidos benignos/sospechosos y ejemplos HTTP de registro sin secretos reales; verificar tamaño UTF-8 y nombres de campos.
- [x] 2.2 Ejecutar npm run check:contracts y npm run build -w contracts; registrar resultados y obtener revisión del usuario y un consumidor del contrato.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
