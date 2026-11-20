# Verificación de sentinel-app-escudo

Fecha: 2026-11-20. Rama: `feat/sentinel-app-escudo`. Implementación por P1 mediante autorización específica del usuario para esta spec originalmente de P4. No se cambia el reparto de los demás encargos.

## Entrega de software

- Pantalla normal Escudo sin flag diagnóstico: activación del modo, escaneo, páginas de cuatro redes, SSID oculto/hexadecimal, selección y captura con ACK/estado confirmado.
- Controles BLE exclusivamente; mismo controller, contratos y transportes. El envío no se presenta como éxito. Resultado pendiente/rechazado visible, metadata fresca y consulta de estado después de mutaciones aplicadas.
- Catálogo invalidado al cambiar nodo, arranque, enlace o escaneo. ACK sin correlación, de otro escaneo o llegado después de desconectar no incorpora redes antiguas.
- Gráficas SVG, volumen de cuatro detectores, tabla/filtro/evidencia con tiempo/calidad, red, canal, RSSI, origen y arranque. Un SSID duplicado benigno no genera incidente; Karma se presenta como indicio pasivo inconcluso.
- Cobertura/pausas/descartes, recuperación por cursor y rango retenido/huecos; reutiliza deduplicación y almacenamiento existentes.
- Reinstalación USB explícita: archivo separado de eventos y cursor anteriores, reset atómico solo con canales desconectados, actualización IndexedDB conservando datos existentes. No se ejecuta ante reinicios normales.
- Guía: [docs/modules/app-escudo.md](../../../docs/modules/app-escudo.md). Reparto y guías P1/P4 actualizados para no duplicar esta implementación.

## Evidencia automatizada

- `npm run test:app`: **36/36**. Incluye paginación, ACK tardío/no correlacionado/rechazado, catálogo vacío, cuatro detectores y catálogo benigno, métricas BLE acotadas por boot, dedup/gap, archivo USB y actualización de una base local v1 conservando cursor/eventos.
- `npm run build`: TypeScript y Vite correctos. Advertencia de tamaño de chunk (>500 kB); no impide compilar y no se hicieron optimizaciones prematuras.
- `node app/checks/escudo-browser.mjs`: Chromium, 390 y 1280 px; pantalla de producto sin flag, escaneo/paginación, SSID oculto/hex, selección rechazada/aplicada, inicio/parada, gráficos, cuatro incidentes, duplicado benigno e historial tras desconexión. Se repitió con la antigua variable `VITE_SENTINEL_P1_TOOLS=1` en el servidor Vite: el panel no reaparece y la pantalla funcional pasa. El runner sustituye exclusivamente el límite BLE; no acredita radio ni teléfono físico.
- `openspec validate sentinel-app-escudo --strict`: válido.
- `git diff --check`: sin problemas de formato.

## Android y comprobaciones físicas

`npm run android:sync` y `./gradlew clean assembleDebug` completados con JDK 21 y SDK Android Linux. Los ensamblados incrementales después de sincronizar fallaron en `compressDebugAssets` por artefactos generados ausentes. La limpieza Gradle resolvió el primer fallo. El APK final compiló correctamente con `./gradlew :app:clean assembleDebug`; la guía incluye este remedio para WSL, sin cambios a SDK/core/firmware. Se verificó que todos sus assets web coinciden byte a byte con `app/dist`.

**2.1 y 2.2 siguen pendientes:** selección/monitoreo real y recuperación BLE con ESP32/teléfono, incluidos gráficos, estado y ausencia de doble incidente. El BLE que el usuario probó antes de esta implementación no acredita esta pantalla nueva. No se simularon ataques en redes reales ni se acredita OLED o pruebas de otros incrementos.

## Alcance local

Sin nuevas dependencias ni cambios en firmware, broker o contratos de mensajes. Los cambios previos del usuario en `firmware/SentinelNode/SentinelConfig.h` y `firmware/SentinelNode/SentinelNode.ino` se conservaron. El usuario autorizó publicar e integrar esta entrega mediante PR hacia develop. La spec permanece activa con sus pruebas físicas pendientes.

## Retirada solicitada del panel temporal

Se eliminaron P1Diagnostics, el flag, el runner exclusivo del panel y las APIs de envío/replay diagnóstico. Dos regresiones del controller se trasladaron a `public-control.test.ts`; las pruebas que solo ejercitaban presets retirados ya no aplican. `npm run check:contracts`: **156/156**, conservando negativos de boot/revisión/TTL y correlación. La salida serie de firmware sin OLED permanece intacta. La guía P1 usa controles normales y distingue replay host de evidencia física.

Se sincronizaron las especificaciones principales `app-escudo` y `mobile-control`; se retiró el delta anterior del panel para impedir que su archivado posterior lo vuelva a exigir. La evidencia antigua se conserva identificada como histórica.

APK final sin diagnóstico: `app/android/app/build/outputs/apk/debug/app-debug.apk` (no versionado). SHA-256: `be208d32d3e45425e69c453ccb858ee9ca1f8fa2f878dbd2422183a3f7a0c310`. Assets contrastados con `app/dist`; sin panel ni flag de diagnóstico en el bundle. `./gradlew :app:clean assembleDebug`: **BUILD SUCCESSFUL**.
