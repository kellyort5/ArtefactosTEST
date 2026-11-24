# Tasks

Responsable de este incremento: **Persona 1**, por autorización expresa del usuario para implementar esta spec originalmente asignada a P4. Los demás encargos de P4 no cambian. Software implementado; aceptación física 2.1/2.2 pendiente. Ver [verificación](verification.md).

## 1. Pantalla Escudo

- [x] 1.1 Implementar catálogo paginado y selección/controles BLE; verificar estados vacío, oculto, pendiente, rechazo y objetivo aplicado.
- [x] 1.2 Implementar gráficos, tabla y detalle de cuatro detectores; comprobar fixtures benignos/sospechosos y lenguaje de indicio.
- [x] 1.3 Integrar cobertura/historial y sincronización; probar dedup/gap y documentar flujo en docs/modules/app-escudo.md.
- [x] 1.4 Ejecutar npm run test:app y npm run build; comprobar vista responsive y acciones con contratos.

- [x] 1.5 Retirar panel, flag y APIs de diagnóstico P1; adaptar guía, conservar pruebas negativas host y comprobar que la variable antigua no reactiva el panel.

## 2. Comprobación del incremento

- [ ] 2.1 Compilar Android y seleccionar/monitorear una red real por BLE con ESP32; verificar gráficas y estado con datos reales.
- [ ] 2.2 Desconectar/reconectar BLE y comprobar recuperación sin doble incidente; adjuntar captura/evidencia sanitizada.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
