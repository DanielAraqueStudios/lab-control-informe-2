# Sprint 6: Medición Ultrasónica (HC-SR04)

## Cambio de Arquitectura

El sistema ha evolucionado abandonando los sensores resistivos analógicos (SE045) los cuales requerían un divisor de voltaje y presentaban inestabilidades, en favor de la medición digital directa con sensores ultrasónicos **HC-SR04**.

Este Sprint contiene el código autónomo para aislar, probar, calibrar y hacer *debugging* de la señal de distancia en milímetros antes de inyectar este nuevo mecanismo al código principal `05_complete_system.ino` y conectarlo a la UI de Python.

## Pines de Referencia Sprint 9

Sprint 9 es la referencia vigente de pinout para el sistema integrado. La prueba aislada de este sprint usa los mismos pines de ultrasonido:

| Tanque | Función | GPIO ESP32-S3 | Cable recomendado |
|--------|---------|---------------|-------------------|
| **# 1**| **TRIG**| **GPIO 5**    | Amarillo |
| **# 1**| **ECHO**| **GPIO 6**    | Verde |
| **# 2**| **TRIG**| **GPIO 8**    | Amarillo |
| **# 2**| **ECHO**| **GPIO 9**    | Verde |

## Conexión Eléctrica ⚠️ IMPORTANTE

1. **Voltaje de Operación (VCC)**: El HC-SR04 estándar requiere **5V** para funcionar correctamente. (Conectar al pin de 5V o VUSB del ESP32).
2. **Señal de Retorno (ECHO)**: El pin ECHO emitirá un pulso de 5V. Aunque el ESP32-S3 puede llegar a tolerarlo si los tiempos son cortos, la mejor práctica de ingeniería **SÍ** requiere usar de nuevo un divisor de voltaje simple para reducir la señal de ECHO a 3.3V (por ej. 10kΩ/22kΩ como se usaba en el Sprint 2).

```text
HC-SR04 (ECHO) ──── 10kΩ ──── ESP32 (Pin 6/9)
                       │
                      22kΩ
                       │
                      GND
```

## Depuración / Carga

Abre el **Serial Plotter** (Herramientas -> Serial Plotter) configurado a `115200 baudios`. Verás inmediatamente dos líneas (azul y roja) que trazan en vivo la distancia de los dos tanques en Milímetros. ¡Usa tu mano levantándola sobre cada sensor para validar que ambos respondan antes de integrarlos al agua!
