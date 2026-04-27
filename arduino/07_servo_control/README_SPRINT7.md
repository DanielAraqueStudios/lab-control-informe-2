# Sprint 7: Control Dual de Servomotores

## Objetivo

Este sprint agrega una prueba aislada para mover dos servomotores desde comandos UART/USB, usando los pines reservados:

| Servo | GPIO ESP32-S3 | Funcion |
|-------|---------------|---------|
| Servo 1 | GPIO10 | PWM signal |
| Servo 2 | GPIO11 | PWM signal |

El sketch esta pensado como base para una futura pestana en la interfaz Python, sin modificar todavia el sistema principal `05_complete_system.ino`.

## Hardware

- ESP32-S3 DevKit
- 2x servomotores 5V
- Fuente externa 5V para servos
- GND comun entre fuente de servos y ESP32

## Conexion

```text
ESP32-S3 GPIO10  -> Servo 1 Signal
ESP32-S3 GPIO11  -> Servo 2 Signal
Fuente 5V        -> VCC servos
Fuente GND       -> GND servos
ESP32 GND        -> GND fuente servos
```

No alimentar los servos desde el pin 3.3V del ESP32. Si los servos consumen corriente alta, usar una fuente dedicada.

## Firmware

Archivo principal:

```text
arduino/07_servo_control/07_servo_control.ino
```

El firmware usa LEDC directamente:

- Frecuencia: `50 Hz`
- Resolucion: `16 bits`
- Pulso minimo: `500 us`
- Pulso neutro: `1500 us`
- Pulso maximo: `2500 us`

## Comandos UART

Configurar Serial Monitor a `115200 baud`.

### Control por angulo

```text
S1,90
S2,120
BOTH,45,135
```

### Control por direccion posicional

```text
DIR,1,LEFT
DIR,1,CENTER
DIR,1,RIGHT
DIR,2,LEFT
DIR,2,CENTER
DIR,2,RIGHT
```

Valores internos:

| Direccion | Angulo |
|-----------|--------|
| LEFT | 45 |
| CENTER | 90 |
| RIGHT | 135 |

### Movimiento incremental

```text
STEP,1,LEFT
STEP,1,RIGHT
STEP,2,LEFT
STEP,2,RIGHT
```

Cada paso cambia `10 grados`.

### Servos de rotacion continua

Si los servos son de rotacion continua, usar:

```text
CR,1,CW,60
CR,1,CCW,60
CR,2,CW,80
CR,2,CCW,80
STOP,1
STOP,2
```

Donde `speed` va de `0` a `100`.

> Nota: En servos posicionales normales, los comandos `CR` no representan velocidad real; para ellos usar `S1`, `S2`, `DIR` o `STEP`.

### Estado y ayuda

```text
STATUS
HELP
DISABLE,1
DISABLE,2
```

`DISABLE` apaga la salida PWM del servo indicado. El servo puede dejar de sostener posicion.

## Integracion futura con Python

La pestana de la interfaz puede enviar estos comandos por el mismo `SerialWorker` usado por el resto del proyecto:

- Botones izquierda/centro/derecha: `DIR,<servo>,<direction>`
- Sliders de angulo: `S1,<angle>` y `S2,<angle>`
- Botones horario/antihorario: `CR,<servo>,CW,<speed>` y `CR,<servo>,CCW,<speed>`
- Boton stop: `STOP,<servo>`

