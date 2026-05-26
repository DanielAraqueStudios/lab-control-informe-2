# Sprint 9: Manual Hardware Control

## Goal

Sprint 9 is now the manual bring-up firmware for the complete hydraulic hardware.

The purpose is to verify, from UART, that the ESP32-S3 can control:

- Pump PWM and direction.
- Tank 1 servo valve.
- Tank 2 servo valve.
- YF-S401 flow reading.
- HC-SR04 height readings for both tanks.

No PID, references, automatic height control, metrics, or experiments are active in this version. Those features should be added only after manual hardware control is reliable.

## Startup Behavior

At boot the firmware always starts in a safe manual state:

```text
PWM = 0
Pump = OFF
Tank 1 valve = CLOSED
Tank 2 valve = CLOSED
Stream = OFF
Safety = OFF
```

The firmware prints:

```text
[OK] Startup state: PWM=0, pump OFF, valves CLOSED
```

## Canonical Sprint 9 Pinout

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

No external servo library is required. Pump and servos are driven with the ESP32 LEDC API directly, which avoids channel conflicts between the pump PWM and Servo 1.

Use ESP32 Arduino Core 3.0.0 or newer because the firmware uses the current LEDC API:

```cpp
ledcAttach(pin, frequency, resolution);
ledcWrite(pin, value);
```

## Serial Settings

```text
115200 baud
Newline enabled
```

Commands are case-insensitive because the firmware converts incoming UART text to uppercase before parsing.

Every received command should echo as:

```text
[CMD] <COMMAND>
```

If you send `S1,170` and do not see `[CMD] S1,170`, the ESP32 did not receive the command.

## Pump Commands

| Command | Meaning |
|---------|---------|
| `PWM,<0-255>` | Set pump PWM and apply it immediately |
| `SETPWM,<0-255>` | Alias for `PWM` |
| `PWM,0` | Stop pump output |
| `PUMP,ON` | Enable pump using the current PWM |
| `PUMP,OFF` | Disable pump output |
| `DIR,FWD` | Set pump direction forward |
| `DIR,REV` | Set pump direction reverse |

Examples:

```text
PWM,0
PWM,120
PUMP,OFF
DIR,FWD
```

## Servo Valve Commands

Valve calibration:

```text
170 degrees = open
180 degrees = closed
```

Manual raw angle commands:

| Command | Meaning |
|---------|---------|
| `S1,<0-180>` | Set Tank 1 servo valve raw angle |
| `S2,<0-180>` | Set Tank 2 servo valve raw angle |
| `BOTH,<a1>,<a2>` | Set both servo valve raw angles |

Calibrated valve commands:

| Command | Meaning |
|---------|---------|
| `V1,OPEN` | Move Tank 1 valve to 170 degrees |
| `V1,CLOSE` | Move Tank 1 valve to 180 degrees |
| `V2,OPEN` | Move Tank 2 valve to 170 degrees |
| `V2,CLOSE` | Move Tank 2 valve to 180 degrees |
| `VALVES,OPEN` | Open both valves |
| `VALVES,CLOSE` | Close both valves |
| `VALVESTATUS` | Print servo valve pins, angles, and attach state |

Debug / compatibility commands:

| Command | Meaning |
|---------|---------|
| `DIR,<servo>,LEFT` | Move valve toward calibrated open |
| `DIR,<servo>,CENTER` | Move valve halfway between open and closed |
| `DIR,<servo>,RIGHT` | Move valve toward calibrated closed |
| `STEP,<servo>,LEFT` | Decrease valve angle by 1 degree |
| `STEP,<servo>,RIGHT` | Increase valve angle by 1 degree |
| `SERVOTEST,<servo>` | Sweep closed-open-closed |
| `DISABLE,<servo>` | Detach servo PWM |
| `CR,<servo>,CW,<0-100>` | Continuous-rotation pulse test |
| `CR,<servo>,CCW,<0-100>` | Continuous-rotation pulse test |
| `STOP,<servo>` | Send neutral pulse to continuous-rotation servo |

Recommended first servo test:

```text
STREAM,OFF
S1,170
S1,180
S2,170
S2,180
VALVESTATUS
```

If the command is received but the servo does not move, check:

```text
Servo signal -> GPIO18 or GPIO19
Servo VCC    -> external 5V
Servo GND    -> common GND with ESP32
```

Do not power servos from the ESP32 3.3V pin.

Servo command acknowledgements include the pulse width now, for example:

```text
[OK] Servo1 angle=90 pulse_us=1500
```

## Sensor / Telemetry Commands

| Command | Meaning |
|---------|---------|
| `READ` | Read sensors once and print one `[DATA]` line |
| `STREAM,ON` | Start periodic `[DATA]` output |
| `STREAM,OFF` | Stop periodic `[DATA]` output |
| `DATALOG` | Alias for `STREAM,OFF` in this manual firmware |
| `STATUS` | Print complete manual hardware status |
| `PINOUT` | Print canonical Sprint 9 pinout |
| `HELP` | Print command list |

Data line format:

```text
[DATA] flow_lpm=0.000,t1_mm=61.7,t2_mm=126.4,pwm=120,pump=ON,dir=FWD,s1=170,s2=180,volume_l=0.000
```

## Optional Safety

Safety is OFF by default to avoid blocking manual hardware tests when a sensor is not calibrated or mounted yet.

| Command | Meaning |
|---------|---------|
| `SAFETY,ON` | Stop pump if a measured tank level is at or above the safety limit |
| `SAFETY,OFF` | Disable automatic safety stop |
| `STOP` | Emergency stop: pump OFF and both valves CLOSED |

Safety limit in firmware:

```text
MAX_LEVEL_ALLOW = 110 mm
```

## Recommended Bring-Up Sequence

1. Upload Sprint 9 firmware.
2. Open Serial Monitor at `115200 baud`, newline enabled.
3. Confirm startup says `PWM=0, pump OFF, valves CLOSED`.
4. Test command receive:

```text
STATUS
PINOUT
```

5. Test Servo 1:

```text
S1,170
S1,180
SERVOTEST,1
```

6. Test Servo 2:

```text
S2,170
S2,180
SERVOTEST,2
```

7. Test pump manually:

```text
PWM,80
PWM,120
PWM,0
```

8. Test sensors:

```text
READ
STREAM,ON
STREAM,OFF
```

## Removed from This Manual Version

These previous Sprint 5/9 commands are intentionally not active in this manual refactor:

```text
SETMODE
SETREF
STARTCTRL
LEVELCTRL
SETPID1
SETPID2
METRICS
EXPERIMENT
CALMODE
SETCAL
GETCAL
```

They can be reintroduced later after pump, valves, and sensors are confirmed working.
