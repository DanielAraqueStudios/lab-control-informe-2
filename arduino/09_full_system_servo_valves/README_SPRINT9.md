# Sprint 9: Full System with Servo Valve Height Control

## Goal

This sprint integrates the complete hydraulic system with two servo-controlled tank valves.

The ESP32-S3 reads the actual tank heights using the ultrasonic sensors and automatically adjusts the valve angle for each tank.

## Main Behavior

Valve calibration:

```text
170 degrees = fully open
180 degrees = fully closed
```

Control logic:

```text
target height - actual height = error

If the tank is below target:
  valve moves toward 170 degrees

If the tank is at or above target:
  valve closes toward 180 degrees
```

At startup, the firmware prints the actual ultrasonic height of Tank 1 and Tank 2 over UART.

## Hardware Pins

| Component | ESP32-S3 GPIO |
|----------|---------------|
| Pump PWM / L298N ENA | GPIO17 |
| Pump IN1 | GPIO15 |
| Pump IN2 | GPIO16 |
| Flow sensor YF-S401 | GPIO4 |
| Tank 1 ultrasonic TRIG | GPIO5 |
| Tank 1 ultrasonic ECHO | GPIO6 |
| Tank 2 ultrasonic TRIG | GPIO8 |
| Tank 2 ultrasonic ECHO | GPIO9 |
| Status LED | GPIO7 |
| Tank 1 servo valve | GPIO18 |
| Tank 2 servo valve | GPIO19 |

## Dependencies

Install the Arduino library:

```text
ESP32Servo
```

## UART Commands

Serial Monitor:

```text
115200 baud
Newline enabled
```

Height control commands:

| Command | Meaning |
|---------|---------|
| `SETLEVEL1,<mm>` | Set Tank 1 target height |
| `SETLEVEL2,<mm>` | Set Tank 2 target height |
| `LEVELCTRL,ON` | Enable automatic servo valve height control |
| `LEVELCTRL,OFF` | Disable valve height control and close valves |
| `VALVESTATUS` | Print actual heights, targets, and valve angles |

Manual valve commands:

| Command | Meaning |
|---------|---------|
| `S1,<0-180>` | Set Tank 1 valve angle manually |
| `S2,<0-180>` | Set Tank 2 valve angle manually |
| `V1,OPEN` | Open Tank 1 valve to 170 degrees |
| `V1,CLOSE` | Close Tank 1 valve to 180 degrees |
| `V2,OPEN` | Open Tank 2 valve to 170 degrees |
| `V2,CLOSE` | Close Tank 2 valve to 180 degrees |
| `VALVES,OPEN` | Open both valves |
| `VALVES,CLOSE` | Close both valves |

Existing full-system commands are still available:

```text
SETMODE,<mode>
SETPWM,<0-255>
SETREF,<type>,<params>
SETPID1,<Kp>,<Ki>,<Kd>
SETPID2,<Kp>,<Ki>,<Kd>
STARTCTRL
STOPCTRL
STATUS
HELP
```

## Example Use

Control both tank heights with servo valves:

```text
SETMODE,MANUAL
SETPWM,120
SETLEVEL1,100
SETLEVEL2,80
LEVELCTRL,ON
VALVESTATUS
```

The valve controller changes valve position. The pump must still be running with `SETPWM` or one of the existing automatic pump control modes for the water level to rise.

Manual valve test:

```text
S1,90
S1,170
S1,180
S2,90
S2,170
S2,180
```

## Safety Behavior

If the measured level reaches the maximum safety limit:

```text
Pump OFF
Valve control OFF
Both valves CLOSED
```

If `STOPCTRL` is sent:

```text
PID/control OFF
Pump OFF
Both valves CLOSED
```

## Notes

The ultrasonic sensors provide the feedback. The servos do not report their actual physical position back to the ESP32-S3, so valve angle status means the last commanded angle.

The control range is intentionally narrow because the physical valve calibration is `170=open` and `180=closed`.
