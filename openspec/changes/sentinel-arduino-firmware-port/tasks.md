# Tasks

> Antecedente fechado: esta versión puede describir TLS/cuentas/ACL/SDK anteriores. Para el comportamiento vigente usar [política del MVP](../../../docs/mvp-policy.md). No convierte pruebas físicas pendientes en realizadas.

Responsable humano: Persona 1 — Jefry. El usuario autoriza ejecución y coordinación Orca; no autoriza commits/push/merge. El coordinador marca cada casilla con evidencia del autor y la línea exacta del CLI.

## 1. Implementación

- [x] 1.1 Trasladar el árbol modular a un sketch de fuente única y configuración propia, manteniendo API/factories de todos los modos.
- [x] 1.2 Adaptar setup/loop, inicialización única, reserva BLE y política de datos sin reducir seguridad.
- [x] 1.3 Implementar generación de validadores y comprobación de SDK en el build Arduino Windows.
- [x] 1.4 Integrar íntegro el Escudo y puntos de extensión de Sanitizador/Cebo sin capacidades o servicios ficticios.

## 2. Verificación y documentación

- [x] 2.1 Ejecutar comparación y pruebas críticas de núcleo/Escudo antes/después sin debilitar aserciones ni cambiar contratos.
- [ ] 2.2 Compilar el firmware completo mediante herramientas Arduino Windows y comprobar apertura/flujo del IDE con el paquete real.
- [ ] 2.3 Ejecutar subida/monitor por COM real y control BLE básico, comprobando memoria, identidad, seguridad e historial sin borrado.
- [x] 2.4 Actualizar arquitectura, R-01/H-02, desarrollo, guías y encargos de todos los módulos con resultados y pendientes explícitos.
