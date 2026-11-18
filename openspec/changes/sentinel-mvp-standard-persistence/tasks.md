# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

## 1. Arduino estándar y distribución de flash

- [x] 1.1 Aplicar tabla compatible con boot_app0 conservando capacidades; mantener solo cuatro claves justificadas y retirar las cinco obsoletas en el instalador. Verificar tabla, guardas y regresiones del instalador con preservación de claves ajenas e idempotencia.
- [x] 1.2 Documentar carga normal USB frente a imagen de inicialización, primera carga limpia y protecciones restantes. Verificar instrucciones y enlaces de firmware y soporte Arduino.

## 2. Configuración persistente

- [x] 2.1 Persistir y restaurar configuración validada mediante NVS oficial, confirmar después de commit y conservar configuración anterior ante fallo. Verificar con pruebas focalizadas guardado, recuperación, ausencia, corrupción, fallo y privacidad; arranque Idle sin reloj UTC obsoleto.
- [x] 2.2 Actualizar documentación de conectividad y contratos de consumo sin modificar wire schemas. Verificar que distingue configuración autorizada persistida de credenciales de terceros prohibidas y no promete cifrado físico.

- [x] 2.3 Persistir y recuperar el objetivo aceptado del Escudo durante cambios de modo y reinicios sin captura automática. Verificar con pruebas focalizadas selección, fallo de guardado y recuperación sin catálogo/uptime/evidencias obsoletas; actualizar guía del módulo.

## 3. Coherencia del proyecto y reparto

- [x] 3.1 Extender política y regla de soluciones estándar primero a arquitectura, decisiones, roadmap, AGENTS/CONTRIBUTING, guías y encargos P1-P4 y planes OpenSpec activos. Verificar que preserva módulos, responsables, pruebas físicas pendientes e historial de evidencias anteriores.
- [x] 3.2 Reconciliar specs principales aplicables con los deltas revisados y validar OpenSpec completo en modo estricto; revisar enlaces y diferencias frente al estado inicial sin perder trabajo ajeno.

## 4. Integración de software

- [x] 4.1 Compilar firmware completo en Windows con core oficial3.3.12 tras integrar los cambios; comprobar particiones, direcciones de carga/exportación estándar, límite4MiB, configuración efectiva y ausencia del SDK propio. Registrar comandos, resultados y límites sin subir a una placa.
- [x] 4.2 Ejecutar regresiones focalizadas de firmware/Escudo, revisar integración real de persistencia y evidencias, y dejar una comprobación física breve de reinicio/corte/USB pendiente explícita. Verificar que no se declara hardware probado ni se hacen commits/push/merge.

## Workflow follow-up

- En hardware real: primera carga limpia, nuevo emparejamiento/configuración, eventos confirmados, reinicio y corte de alimentación sin USB; comprobar conservación. Reinstalación USB puede reinicializar datos.
- Solicitar revisión del equipo antes de integrar. Publicar, integrar o archivar solo con autorización específica.
