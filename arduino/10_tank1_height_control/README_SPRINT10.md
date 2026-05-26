# Sprint 10: Tank 1 Height Control

## Goal

Sprint 10 controls the height of Tank 1 using:

- The filtered Tank 1 HC-SR04 ultrasonic reading.
- The Tank 1 servo valve on GPIO18.
- UART commands for target height and control enable/disable.

Sprint 10 keeps the Sprint 9 manual pump and valve commands. The pump is still controlled manually from UART; the automatic loop only moves the Tank 1 outlet valve.

## Control Behavior

Valve calibration:

```text
170 degrees = open
180 degrees = closed
```

The controller uses filtered `t1_mm`:

- If Tank 1 is above the target, the valve opens.
- The farther above target, the more the valve opens.
- If Tank 1 is at or below the target deadband, the valve closes.

Constants in firmware:

```text
Default target       = 80 mm
Allowed target range = 0 to 110 mm
Deadband             = +/- 3 mm
Full-open error      = 40 mm above target
Control interval     = 300 ms
```

## Main UART Commands

| Command | Meaning |
|---------|---------|
| `SETHEIGHT1,<mm>` | Set Tank 1 desired height |
| `SETLEVEL1,<mm>` | Alias for `SETHEIGHT1` |
| `TARGET1,<mm>` | Alias for `SETHEIGHT1` |
| `H1,<mm>` | Short alias for `SETHEIGHT1` |
| `LEVEL1,ON` | Enable Tank 1 height control |
| `LEVEL1,OFF` | Disable Tank 1 height control and close Tank 1 valve |
| `LEVEL1,STATUS` | Print target, current height, distance, and valve angle |

Manual Tank 1 valve commands such as `S1,<angle>`, `V1,OPEN`, `V1,CLOSE`,
`BOTH,<a1>,<a2>`, and `DISABLE,1` turn Tank 1 automatic control OFF.

## Recommended Test Sequence

Open Serial Monitor at `115200 baud` with newline enabled.

```text
STREAM,ON
SETHEIGHT1,80
LEVEL1,ON
PWM,90
PUMP,ON
```

Watch the `[DATA]` lines and `[CTRL1]` control messages.

Example telemetry:

```text
[DATA] flow_lpm=0.000,t1_mm=78.0,t2_mm=1.0,d1_mm=72.0,d2_mm=159.0,pwm=90,pump=ON,dir=FWD,s1=180,s2=180,volume_l=0.000,ctrl1=ON,target1_mm=80.0
[CTRL1] target_mm=80.0,level_mm=95.0,error_mm=15.0,valve_angle=177
```

To stop the automatic valve control:

```text
LEVEL1,OFF
PUMP,OFF
```

For emergency stop:

```text
STOP
```

This stops the pump, closes both valves, and disables Tank 1 height control.

## Notes

If Tank 1 is below the target and the pump is OFF, the controller cannot raise the level. It will close the Tank 1 valve and wait for water input.

The ultrasonic timing protection from Sprint 9 is retained:

- Flow interrupt isolation during ultrasonic timing is ON by default.
- `FLOWISO,ON` and `FLOWISO,OFF` are available for comparison.
- Distances below 20 mm are rejected.
- Readings use a 5-sample median filter and large-jump confirmation.
