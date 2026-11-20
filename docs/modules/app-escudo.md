# Pantalla Escudo Pasivo

Implementación de `sentinel-app-escudo`, disponible en la navegación normal del APK, sin opciones de diagnóstico. Utiliza el controller BLE, contratos v2 y journal compartidos; no necesita Mosquitto para este modo. El diagnóstico P1 se retiró por decisión del usuario; las pruebas negativas y reenvíos exactos se verifican en software.

## Preparación y APK desde WSL

Usa el firmware Arduino actual con Escudo habilitado. Para la placa sin OLED, conserva `SENTINEL_SERIAL_TEST_DISPLAY=1` y el botón físico, según la [guía P1](../guia-pruebas-persona-1.md). Esta entrega solo cambia la app; no exige volver a cargar firmware si ya funciona con v2.

Desde la raíz del repositorio, con Node 24, JDK 21 y el SDK Android Linux instalado:

```sh
export ANDROID_HOME=/home/jefry/Android/Sdk
export ANDROID_SDK_ROOT="$ANDROID_HOME"
npm run android:sync
npm run android:assemble
```

Si Gradle falla en `compressDebugAssets` después de sincronizar, regenera los artefactos de la app desde la raíz (no borra código ni datos del teléfono):

```sh
(cd app/android && ./gradlew :app:clean assembleDebug)
```

Adapta la ruta del SDK a tu instalación WSL; no ejecutes `gradlew.bat` ni comandos PowerShell en esta terminal. APK generado: `app/android/app/build/outputs/apk/debug/app-debug.apk`. Instálalo en el teléfono. No hace falta mantener Vite o Docker abiertos.

## Recorrido funcional

1. Abre **General**, conecta BLE protegido a tu nodo y comprueba el estado v2. Puedes sincronizar la hora para disponer de marcas UTC; si no hay hora, se muestra uptime y calidad, sin inventar fechas.
2. Abre **Escudo Pasivo**. Si el nodo está en Reposo, pulsa **Activar Escudo** y espera la confirmación. Entrar a la pestaña no cambia el modo automáticamente.
3. Pulsa **Escanear redes**, espera **Completado** y pulsa **Ver redes detectadas**. Cada página solicita hasta cuatro redes; **Cargar más redes** continúa con el cursor del mismo escaneo.
4. Revisa SSID, BSSID, canal, RSSI, autenticación, cifrado y PMF. Un SSID vacío aparece como **SSID oculto**; uno no textual se identifica como hexadecimal. La selección conserva sus bytes originales.
5. Pulsa **Seleccionar …** en una red autorizada. El objetivo confirmado se actualiza con el estado del ESP32 después del ACK. Un rechazo mantiene el objetivo previo y muestra su motivo.
6. Pulsa **Iniciar monitoreo**. Comprueba captura, canal, métricas y cobertura. Para cambiar de red, detén la captura, selecciona otro objetivo e inicia de nuevo. **Detener monitoreo** detiene la captura; **Volver a reposo** libera el modo.
7. Revisa actividad Beacon/Deauth/Disassoc, volumen de alertas y la tabla de incidentes. Abre **Ver evidencia** y filtra por detector. Los datos incluyen red, canal, RSSI, calidad de tiempo, origen real/simulado y arranque.
8. Desconecta desde la cabecera: los controles quedan deshabilitados, se conserva el historial y se indica que los datos no son actuales. Reconecta desde **General**, vuelve a Escudo y revisa la recuperación automática o pulsa **Sincronizar historial por BLE**.
9. Comprueba que un mismo ID no genera dos incidentes. Si el búfer del ESP32 perdió eventos, se muestra el rango retenido y la discontinuidad; no se presenta un historial incompleto como íntegro.

Una orden enviada/aceptada no equivale a aplicada. Mientras espera ACK, se bloquean otras acciones del módulo durante hasta 15 s. Después se permite reintentar, pero la orden anterior conserva su resultado pendiente. Consulta estado antes de repetir; el controller refresca metadata y no reenvía automáticamente la mutación.

## Lectura de resultados

- Los cuatro detectores son ráfaga de desautenticación, desconexión sospechosa, posible Evil Twin e indicio de Karma. Son heurísticas: SSID repetido por sí solo no es ataque; Karma conserva su limitación pasiva e inconclusa.
- Las gráficas usan contadores acumulados declarados por el firmware, no tasas inventadas por intervalo. Se retienen hasta 60 muestras en memoria por arranque, sin mezclarlas con otro nodo/boot. Un nuevo ciclo de captura puede reiniciar contadores.
- La radio observa un canal a la vez. Se muestran motivos de pausa, tiempo observado/pausado y descartes. No recibir alertas no demuestra ausencia de ataques.
- El journal mantiene hasta 256 eventos por nodo en el teléfono. Los eventos en vivo no adelantan el cursor de recuperación; solo lo hace una página confirmada y completa, o una con huecos explícitos. Las métricas no son un archivo persistente.

## Reinstalación USB frente a reinicio normal

Si una carga USB reinicializó el nodo, aunque conserve su ID por MAC, no reutilices el cursor anterior. En **Cobertura y recuperación → Después de reinstalar el firmware por USB**:

1. Desconecta BLE y MQTT.
2. Marca la confirmación de reinstalación y pulsa **Guardar historial anterior e iniciar uno nuevo**.
3. Los eventos y el cursor anteriores se archivan localmente en una transacción; el historial actual comienza en cero. Las copias anteriores pueden consultarse en esa sección.
4. Retira el vínculo Android obsoleto si corresponde, vuelve a emparejar y conecta. Se recuperará desde cero.

No usar este procedimiento después de un reinicio/corte normal: allí deben sobrevivir identidad, bonds y eventos. La app no detecta ni presupone automáticamente una reinstalación a partir del boot_id. La actualización de IndexedDB conserva los eventos/cursor existentes y añade el archivo separado, sin nuevas dependencias.

## Verificación y límites

```sh
npm run test:app
npm run build
# En una terminal de pruebas:
npm run dev -w app -- --host 127.0.0.1 --port 5184
# En otra terminal:
node app/checks/escudo-browser.mjs
```

La comprobación de navegador sustituye el límite BLE solo dentro del runner y pasa por el controller y validador reales. No agrega simulaciones al producto ni acredita radio física. Las tareas 2.1 y 2.2 de la [spec](../../openspec/changes/sentinel-app-escudo/tasks.md) requieren selección/monitoreo y desconexión/reconexión con ESP32 y teléfono. Registrar APK, firmware, red autorizada, resultado y evidencia sin claves ni datos privados.
