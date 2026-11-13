# Tasks

Responsable único de todas las casillas: **Persona 1**, por reasignación explícita del usuario el 2026-11-12. Implementación del adaptador común; ver [verificación](verification.md) y conservar pendientes las comprobaciones físicas no ejecutadas.

## 1. Adaptadores comunes

- [x] 1.1 Actualizar transporte/controlador al codec v1/v2 y negociación; probar incompatibilidad, retained y boot/revisión.
- [x] 1.2 Extender store/journal para familias de tres módulos y paginación; probar dedup, eventos antiguos y gap.
- [x] 1.3 Crear navegación/secciones de módulos con capacidades reales y estados pendientes; verificar no disponible/rechazado sin éxito falso.
- [x] 1.4 Limpiar secretos transitorios y documentar conexión/versiones en app/README.md; comprobar que no hay persistencia de claves.

## 2. Comprobación del incremento

- [ ] 2.1 Ejecutar npm run test:app y npm run build; verificar arranque web y control básico de base real.
- [x] 2.2 Ejecutar npm run android:sync y npm run android:assemble; comprobar permisos/conexión BLE en teléfono o dejar evidencia pendiente.

## Workflow follow-up

- El coordinador actualiza el seguimiento OpenSpec con evidencia del autor; sus compañeros no necesitan ejecutar el CLI.
- Integrar por PR revisado por otro integrante. Commit, push, merge y archivado requieren la autorización correspondiente; esta propuesta no la otorga.
