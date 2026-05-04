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

Commands are case-insensitive because the firmware converts incoming UART text to uppercase before parsing.

Height control commands:

| Command | Meaning |
|---------|---------|
| `SETLEVEL1,<mm>` | Set Tank 1 target height (`h1`) |
| `SETLEVEL2,<mm>` | Set Tank 2 target height (`h2`) |
| `LEVELCTRL,ON` | Enable automatic servo valve height control |
| `LEVELCTRL,OFF` | Disable valve height control and close both valves |
| `VALVESTATUS` | Print actual heights, targets, and valve angles |

The height targets are independent for each tank. The valve controller compares each ultrasonic height against its own target and commands the corresponding servo valve.

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

Manual valve commands disable automatic valve height control before moving the valve.

Mode, pump, and PID commands:

| Command | Meaning |
|---------|---------|
| `SETMODE,MANUAL` | Manual pump mode |
| `SETMODE,AUTO_FLOW` | Automatic flow PID mode |
| `SETMODE,AUTO_LEVEL1` | Automatic Tank 1 level PID mode |
| `SETMODE,AUTO_LEVEL2` | Automatic Tank 2 level PID mode |
| `SETMODE,CASCADE` | Cascade mode: Tank 1 level to flow to pump |
| `SETPWM,<0-255>` | Set pump PWM in `MANUAL` mode only |
| `SETPID1,<Kp>,<Ki>,<Kd>` | Set Tank 1 level PID gains |
| `SETPID2,<Kp>,<Ki>,<Kd>` | Set Tank 2 level PID gains |
| `STARTCTRL` | Start the active pump/PID control mode and print current heights |
| `STOPCTRL` | Stop PID/control, stop pump, disable valve control, and close both valves |

Reference commands:

| Command | Meaning |
|---------|---------|
| `SETREF,STEP,<initial>,<final>` | Step reference |
| `SETREF,RAMP,<initial>,<final>,<duration_s>` | Ramp reference |
| `SETREF,PARA,<initial>,<final>,<duration_s>` | Parabolic/smooth reference |

Logging, metrics, calibration, and experiments:

| Command | Meaning |
|---------|---------|
| `DATALOG` | Toggle CSV telemetry logging |
| `METRICS` | Start/stop and print performance metrics |
| `CALMODE` | Start calibration stream with default 60% pump |
| `CALMODE,<0-100>` | Start calibration stream with pump percentage |
| `CALSTOP` | Stop calibration mode and stop pump |
| `SETCAL,<tank>,<empty>,<full>` | Apply legacy ADC calibration values for tank 1 or 2 |
| `GETCAL` | Print current legacy ADC calibration values |
| `EXPERIMENT,STEP_FLOW` | Run predefined step flow experiment |
| `EXPERIMENT,RAMP_LEVEL` | Run predefined Tank 1 ramp level experiment |
| `EXPERIMENT,DISTURBANCE` | Run predefined disturbance test |

Information commands:

| Command | Meaning |
|---------|---------|
| `STATUS` | Print full system status |
| `HELP` | Print available Sprint 9 commands |

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

`SETCAL` and `GETCAL` are preserved from older ADC level-sensor workflows. In Sprint 9, the active tank height feedback comes from the HC-SR04 ultrasonic sensors.
