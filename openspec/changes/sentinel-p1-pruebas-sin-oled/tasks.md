# Tasks

## 1. Visualización local de pruebas

- [x] 1.1 Implementar la opción serie desactivada por defecto y su integración con estado, ventana física y passkey BLE; verificar con pruebas focalizadas entrega fallida, ventana cerrada, código temporal y OLED ausente sin presencia ficticia.
- [x] 1.2 Añadir el contador acotado de entrada real a detectores y documentar activación y retirada del modo serie; verificar compilación host y que no almacena tramas ni códigos.

## 2. Diagnóstico BLE P1

- [x] 2.1 Implementar panel optativo y órdenes públicas reales de descubrimiento, selección, captura, estado, historial, modos y negativos controlados; verificar BLE desconectado, metadatos frescos, validación, reenvío explícito y resultados ACK con pruebas focalizadas.
- [x] 2.2 Integrar panel en Android sin alterar el cliente BLE ni las pantallas de producto y documentar su activación; verificar pruebas y build de app con opción desactivada y activada.

## 3. Recorrido integrado

- [x] 3.1 Actualizar guía P1 y matriz T01–T15 para monitor serie y diagnóstico, conservando pendientes físicos, laboratorio e integración de compañeros; verificar coherencia, enlaces e instrucciones ejecutables.
- [x] 3.2 Compilar firmware con configuración estándar Arduino 3.3.12 en ambos modos y generar APK diagnóstico; registrar resultados reproducibles y conservar pruebas físicas como pendientes.

## Workflow follow-up

- Conectar ESP32 y teléfono, ejecutar la guía y registrar evidencias físicas sin passkeys ni secretos; OLED real y escenarios RF controlados permanecen pendientes hasta probarlos.
- Solicitar revisión antes de archivar; no realizar commit, push ni merge sin autorización.

## 4. Correcciones de memoria y pila durante prueba física

- [x] 4.1 Reubicar la caché con la API oficial sin reducir sus32 entradas, comprobar inicialización/tareas y mostrar la ventana por serie. Build y regresiones PASS; versión intermedia cargada y arranque medido, botón/código observados por el usuario.
- [x] 4.2 Decodificar el reinicio con el ELF exacto, corregir los buffers grandes de la pila BLE y probar asignación fallida, limpieza, invariancia del comando y digest. Build, ASan/UBSan y auditoría de marcos nativos PASS. Preparar paquete final sin cargar hardware ausente. Ver [evidencia](ble-stack-verification.md).
- [ ] 4.3 Cargar la imagen final al recuperar la placa y comprobar pairing protegido, estado/ACK aplicado y ausencia de reinicios desde Android. No cerrar por el build o por recibir el código en la versión intermedia.
