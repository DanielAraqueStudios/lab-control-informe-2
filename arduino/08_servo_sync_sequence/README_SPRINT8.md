# Sprint 8: Control y Depuracion UART de Servomotores

## Objetivo

Este sprint prueba dos servomotores en un ESP32-S3 usando la libreria `ESP32Servo`.

El firmware hace dos cosas:

- Mueve ambos servos automaticamente de `0` a `180` grados y vuelve de `180` a `0`.
- Permite enviar comandos por UART para detener el barrido, mover cada servo a un angulo deseado y consultar el estado de depuracion.

## Pines

| Servo | GPIO ESP32-S3 | Funcion |
|-------|---------------|---------|
| Servo 1 | GPIO18 | Signal PWM |
| Servo 2 | GPIO19 | Signal PWM |

## Hardware

- ESP32-S3 DevKit
- 2x servomotores
- Fuente externa 5V para los servos
- GND comun entre ESP32-S3 y fuente de servos

No alimentar los servos desde el pin 3.3V del ESP32-S3.

## Conexion

```text
ESP32-S3 GPIO18  -> Servo 1 Signal
ESP32-S3 GPIO19  -> Servo 2 Signal
Fuente 5V        -> VCC servos
Fuente GND       -> GND servos
ESP32 GND        -> GND fuente servos
```

## Firmware

Archivo principal:

```text
arduino/08_servo_sync_sequence/08_servo_sync_sequence.ino
```

Dependencia Arduino:

```text
ESP32Servo
```

Parametros principales:

| Parametro | Valor |
|-----------|-------|
| Servo 1 | GPIO18 |
| Servo 2 | GPIO19 |
| Rango por comando | 0-180 grados |
| Delay entre pasos del barrido | 15 ms |
| Pausa al final del recorrido | 1000 ms |
| UART | 115200 baud |

## Comportamiento

Al iniciar, el sketch:

1. Inicializa `Serial` a `115200 baud`.
2. Adjunta `servo1` a GPIO18 y `servo2` a GPIO19.
3. Mueve ambos servos a `0 grados`.
4. Inicia un barrido automatico de `0 -> 180 -> 0`.
5. Atiende comandos UART sin bloquear el barrido.

Cuando se envia un comando manual de angulo, el barrido automatico se detiene para que el servo quede en la posicion solicitada.

## Comandos UART

Configurar Serial Monitor a `115200 baud`.

| Comando | Funcion |
|---------|---------|
| `START` | Reanuda el barrido automatico |
| `STOP` | Detiene el barrido automatico |
| `HOME` | Detiene el barrido y mueve ambos servos a 0 grados |
| `S1,<angle>` | Mueve Servo 1 a un angulo 0-180 |
| `S2,<angle>` | Mueve Servo 2 a un angulo 0-180 |
| `SET,<servo>,<angle>` | Mueve el servo indicado a un angulo 0-180 |
| `BOTH,<a1>,<a2>` | Mueve ambos servos a angulos especificos |
| `ANGLES` | Imprime los angulos comandados actuales |
| `GET,<servo>` | Imprime el angulo comandado de un servo |
| `DEBUG` | Imprime estado completo de depuracion |
| `HELP` | Imprime la lista de comandos |

Ejemplos:

```text
STOP
S1,45
S2,135
SET,1,90
BOTH,0,180
ANGLES
GET,2
DEBUG
START
```

## Nota importante sobre los angulos

Los servomotores estandar no entregan feedback de posicion al ESP32-S3. Los comandos `ANGLES`, `GET` y `DEBUG` muestran el ultimo angulo comandado por el firmware, no una medicion fisica real del eje.

Si el servo es de rotacion continua o 360 grados, `Servo.write(0-180)` normalmente controla direccion/velocidad, no una posicion angular real de 0-360. Para control posicional real de 360 grados se requiere un servo compatible con posicion 360 o calibracion con `writeMicroseconds()`.
