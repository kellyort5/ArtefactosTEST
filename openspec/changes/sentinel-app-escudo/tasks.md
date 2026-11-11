# Tasks

Responsable único de todas las casillas: **Persona 4**. Estado inicial: todo pendiente; propuesta lista para revisión, sin implementación.

## 1. Pantalla Escudo

- [ ] 1.1 Implementar catálogo paginado y selección/controles BLE; verificar estados vacío, oculto, pendiente, rechazo y objetivo aplicado.
- [ ] 1.2 Implementar gráficos, tabla y detalle de cuatro detectores; comprobar fixtures benignos/sospechosos y lenguaje de indicio.
- [ ] 1.3 Integrar cobertura/historial y sincronización; probar dedup/gap y documentar flujo en docs/modules/app-escudo.md.
- [ ] 1.4 Ejecutar npm run test:app y npm run build; comprobar vista responsive y acciones con contratos.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar Android y seleccionar/monitorear una red real por BLE con ESP32; verificar gráficas y estado con datos reales.
- [ ] 2.2 Desconectar/reconectar BLE y comprobar recuperación sin doble incidente; adjuntar captura/evidencia sanitizada.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
