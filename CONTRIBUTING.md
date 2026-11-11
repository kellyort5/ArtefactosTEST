# Colaborar en Sentinel Node

## Contexto y alcance

Antes de comenzar, leer [el brief](docs/brief.md), las [instrucciones del repositorio](AGENTS.md), la [arquitectura](docs/architecture.md), las [decisiones técnicas](docs/technical-decisions.md) y el [roadmap](docs/roadmap.md). Respetar las decisiones confirmadas y revisar las especificaciones OpenSpec relacionadas con la tarea.

Mantener separados firmware, broker, app y simulador. Cada cambio debe resolver una sola funcionalidad, corrección o propósito concreto; evitar refactorizaciones, cambios de formato y modificaciones ajenas a ese alcance sin justificación.

## Ramas

- `main` representa la versión estable e integrada del proyecto.
- Las funcionalidades se desarrollan en ramas `feat/<descripcion-breve>`; por ejemplo, `feat/escudo-descubrimiento`.
- Las correcciones se desarrollan en ramas `fix/<descripcion-breve>`; por ejemplo, `fix/duplicados-historial`.
- Crear la rama desde la base estable adecuada, normalmente `main` actualizado. Revisar antes el estado de Git y conservar cualquier trabajo pendiente.
- No desarrollar directamente en `main`; integrar mediante Pull Requests.

Una funcionalidad amplia puede dividirse en varias ramas y PR pequeños con dependencias explícitas. Esa división organiza la implementación y no elimina requisitos del alcance final.

## OpenSpec y código

Para nuevas funcionalidades, preparar el cambio con OpenSpec y revisar sus specs antes de implementar. Las propuestas, specs, diseños y tareas pertinentes deben versionarse junto con el código correspondiente. Si los artefactos se revisan en un PR previo, el PR de implementación debe enlazarlo y mantenerlos coherentes con el resultado final.

Conservar tanto las specs principales como los cambios y archivos archivados que formen parte del historial del proyecto. No excluir `openspec/` con `.gitignore`. Actualizar las tareas según comportamiento realmente implementado, sin dar por terminadas funciones representadas solo por mocks, simulación o interfaz.

## Mensajes de commit

Usar Conventional Commits:

```text
<tipo>(<alcance opcional>): <resumen>
```

Tipos habituales: `feat`, `fix`, `docs`, `refactor`, `test`, `build`, `ci` y `chore`. El alcance puede identificar el componente afectado: `firmware`, `broker`, `app`, `simulator`, `contracts` o `docs`.

Ejemplos:

```text
feat(firmware): agrega selección de red por BLE
fix(app): evita duplicar eventos sincronizados
docs: documenta el flujo de colaboración
chore(broker): ajusta configuración local de desarrollo
```

Mantener commits pequeños y relacionados. Codex solo puede ejecutar commits, push, merges o eliminar ramas con autorización explícita; pedirle preparar un mensaje o revisar un diff no autoriza esas operaciones.

## Pull Requests y revisión

Cada PR debe incluir:

- Qué cambió y por qué, con alcance y componentes afectados.
- Referencia al cambio OpenSpec y sus specs cuando corresponda.
- Comprobación funcional básica: qué se ejecutó, entorno o dispositivo utilizado y resultado. Si algo no se pudo comprobar, indicarlo con su limitación.
- Riesgos o pendientes que afecten al comportamiento entregado y justificación de modificaciones fuera del alcance inicial, si existen.

Al menos **otro integrante** debe revisar el PR antes de integrarlo en `main`. El autor atiende los comentarios y revisa de nuevo el resultado. La revisión debe considerar alcance, comportamiento, specs, datos sensibles y comprobación funcional.

Para firmware, comprobar compilación y comportamiento breve en el ESP32 cuando aplique. Para BLE, MQTT o notificaciones, comprobar el canal real pertinente. Mantener pruebas automatizadas pequeñas para lógica crítica. Para documentación o configuración, hacer la comprobación apropiada sin exigir pruebas de hardware que no correspondan. No convertir benchmarks o campañas extensas de estabilidad en un requisito general para integrar cada incremento.

Estas son convenciones del equipo; su documentación no configura protecciones de ramas ni reglas de aprobación en el servicio remoto.

## Archivos que se conservan y archivos locales

Versionar código fuente y configuración necesaria para reproducir el proyecto: `platformio.ini`, `CMakeLists.txt`, `sdkconfig.defaults*`, `sdkconfig` si forma parte de la configuración mantenida, `idf_component.yml`, `dependencies.lock`, `package.json`, lockfiles, configuración Vite/TypeScript/Tailwind y `capacitor.config.*`.

Conservar el proyecto Android nativo, su código Java/Kotlin, manifest, recursos, archivos Gradle y Gradle Wrapper. No ignorar la carpeta `android/` completa ni todos los archivos `.jar`: el wrapper necesario para compilar debe poder versionarse. Separar los archivos nativos mantenidos de los recursos web y metadatos regenerados por la sincronización de Capacitor.

Excluir dependencias instaladas, directorios de compilación, APK/AAB, cachés, logs y archivos temporales. No excluir todos los archivos `.json`, `.yaml`, `.pem` o carpetas de datos: pueden contener contratos, configuración, certificados públicos o fixtures necesarios.

Los `.env.example`, `.env.sample` y `.env.template`, incluidas variantes por entorno, pueden versionarse **solo con valores ficticios**. Los `.env` reales, claves privadas, archivos de cuentas de servicio, almacenes de firma y contraseñas del broker permanecen fuera de Git. Los certificados públicos y las CA pueden ser parte de la configuración; revisar que no contengan claves privadas.

`.gitignore` no evita todos los secretos ni deja de versionar archivos ya registrados. Revisar siempre el contenido y la lista de archivos preparados antes de un commit. No usar inclusión forzada para saltarse una exclusión sin revisar por qué existe. Si se descubre un secreto ya publicado, coordinar su revocación y tratamiento; no reescribir el historial automáticamente.

## Conflictos y operaciones de Git

Inspeccionar cada conflicto y resolverlo conservando las aportaciones necesarias. No descartar cambios ajenos ni usar operaciones destructivas o reescritura de historial para hacer desaparecer conflictos automáticamente. Consultar al responsable de la otra parte si la intención no está clara o la resolución modifica decisiones funcionales o arquitectónicas.

Antes de dar una tarea por terminada, revisar diferencias y archivos nuevos, mantener la documentación/specs coherentes y registrar la comprobación realizada. No incluir automáticamente cambios preexistentes de otra tarea en el commit o PR actual.
