# Instrucciones para Codex

- Leer [docs/brief.md](docs/brief.md) antes de proponer cambios.

- Utilizar OpenSpec para nuevas funcionalidades.

- No implementar funcionalidades sin revisar las specs.

- Mantener separados firmware, broker, app y simulador.

- Escribir pruebas para lógica crítica.

- Solicitar revisión antes de decisiones de arquitectura importantes.

- No realizar pruebas activas sobre redes ajenas.

- No guardar contraseñas ni credenciales de terceros.

## Arquitectura y alcance

- Respetar las decisiones aprobadas de [arquitectura](docs/architecture.md), [decisiones técnicas](docs/technical-decisions.md), [roadmap](docs/roadmap.md) y las especificaciones OpenSpec aplicables. Distinguir acuerdos confirmados de recomendaciones e hipótesis pendientes.
- Mantener los cambios pequeños, coherentes y relacionados con una sola funcionalidad o corrección. Dividir una entrega grande en incrementos integrables sin reducir su alcance acordado.
- No modificar código ni archivos ajenos al alcance de la tarea sin una justificación concreta. Preservar el trabajo existente de otros integrantes.

## Git y colaboración

- Seguir [CONTRIBUTING.md](CONTRIBUTING.md). Trabajar en la rama correspondiente a la tarea: `feat/*` para funcionalidades y `fix/*` para correcciones; no desarrollar directamente sobre `main`.
- Revisar la rama y el estado de Git antes de editar. Si el trabajo pendiente pertenece a otra tarea, preservarlo; no descartarlo, moverlo ni incluirlo automáticamente en la entrega actual.
- No ejecutar commits, push, merges ni eliminar ramas locales o remotas sin autorización explícita del usuario para la operación correspondiente. Solicitar preparar un mensaje de commit no autoriza ejecutar el commit ni publicarlo.
- Cuando se solicite preparar un commit, revisar su contenido y utilizar Conventional Commits: `<tipo>(<alcance opcional>): <resumen>`. Ejemplos: `feat(firmware): agrega descubrimiento pasivo` y `fix(app): evita incidentes duplicados`.
- Integrar cambios mediante Pull Requests con descripción y comprobación funcional básica; al menos otro integrante debe revisar el PR.
- Versionar las especificaciones y artefactos OpenSpec del cambio junto con su código correspondiente. No ignorar `openspec/` ni sus archivos de planificación necesarios.
- No incluir secretos, credenciales, archivos `.env` reales, dependencias instaladas, compilaciones ni artefactos generados. Los ejemplos de configuración solo deben contener valores ficticios. Conservar configuración fuente, lockfiles y código, incluido el proyecto Android nativo.
- Revisar los cambios antes de considerarlos terminados: alcance, diferencias, archivos nuevos, secretos y comprobaciones apropiadas. Para cambios funcionales, compilar y hacer una comprobación breve; mantener pruebas focalizadas para lógica crítica. Para documentación, verificar coherencia y enlaces.
- No resolver conflictos automáticamente mediante operaciones destructivas de Git: no usar `reset --hard`, `clean -f`, descarte masivo con `restore`/`checkout`, reescritura de historial ni push forzado para eliminar el conflicto. Inspeccionar los archivos, conservar el trabajo ajeno y resolver de forma explícita; pedir orientación si la resolución cambia el comportamiento o el alcance acordado.

## Soluciones oficiales y persistencia

Evaluar primero APIs, bibliotecas, opciones y recetas oficiales de Arduino antes de personalizar core, SDK, compilador, carga o exportación. Justificar por escrito la necesidad, alternativas y alcance mínimo de cualquier excepción; no fabricar un SDK si la solución oficial satisface el requisito.

Aplicar la [política de persistencia](docs/persistence-policy.md): identidad, bonds, configuración e historial confirmado sobreviven a uso normal/reinicio/corte; reinstalación o actualización USB puede reinicializarlos sin migración. La primera carga de la tabla nueva es limpia desde el IDE; los scripts de preparación no graban placas. No borrar NVS automáticamente en arranque normal ni almacenar credenciales de terceros; secretos fuera de Git/logs/eventos/estado público.
