# SPRINT 2: Sensor Reading - Quick Start Guide

> ⚙️ **Software Requirement:** ESP32 Arduino Core **3.0.0+**  
> 📌 **Pin Restriction:** Only GPIOs 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 46

## 📋 Hardware Setup

### Components Required

| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32-S3 DevKit | 1 | Main controller |
| YF-S401 Flow Sensor | 1 | Measure water flow (0.3-6 L/min) |
| SE045 Water Level Sensor | 2 | Measure water level in tanks |
| 10kΩ Resistors | 2 | Voltage divider (protection) |
| 22kΩ Resistors | 2 | Voltage divider (protection) |
| 220Ω Resistor | 1 | LED current limiting |
| LED | 1 | Status indicator |
| Breadboard | 1 | Circuit assembly |
| Jumper Wires | 20+ | Connections |

---

## 🔌 DETAILED CONNECTION DIAGRAM

### Complete Wiring Schematic:

```
                    ┌─────────────────────┐
                    │   ESP32-S3 DevKit   │
                    │                     │
                    │  GPIO 4  (INT) ─────┼─────► YF-S401 Signal (Yellow)
                    │                     │
                    │  GPIO 5  (ADC) ─────┼──┐
                    │                     │  │   ┌──────────── SE045 Tank 1
                    │  GPIO 6  (ADC) ─────┼──┼───┼──┐
                    │                     │  │   │  │   ┌──── SE045 Tank 2
                    │  GPIO 7  ────220Ω───┼──┼───┼──┼───┼──► LED ─── GND
                    │                     │  │   │  │   │
                    │  GND ───────────────┼──┼───┼──┼───┼──┐
                    │  3.3V ──────────────┼──┼───┼──┼───┼──┼──► Sensors VCC
                    │                     │  │   │  │   │  │
                    └─────────────────────┘  │   │  │   │  │
                                             │   │  │   │  │
                    ┌───────────────────┐    │   │  │   │  │
                    │  YF-S401 Flow     │    │   │  │   │  │
                    │                   │    │   │  │   │  │
                    │  VCC (Red)    ────┼────┼───┼──┼───┘  │
                    │  Signal (Yellow) ─┼────┘   │  │      │
                    │  GND (Black) ─────┼────────┼──┼──────┘
                    └───────────────────┘        │  │
                                                 │  │
                    ┌───────────────────┐        │  │
                    │  SE045 Tank 1     │        │  │
                    │  (Water Level)    │        │  │
                    │                   │        │  │
   Voltage Divider: │  VCC ─────────────┼────────┘  │
   (CRITICAL!)      │  Signal ──10kΩ────┼───┐       │
                    │         │         │   │       │
                    │         22kΩ      │   │       │
                    │         │         │   │       │
                    │  GND ────┴─────────┼───┼───────┘
                    └───────────────────┘   │
                                   ESP32 GPIO 5
                                            
                    ┌───────────────────┐   
                    │  SE045 Tank 2     │   
                    │  (Water Level)    │   
                    │                   │   
   Voltage Divider: │  VCC ─────────────┼───┘
   (CRITICAL!)      │  Signal ──10kΩ────┼───┐
                    │         │         │   │
                    │         22kΩ      │   │
                    │         │         │   │
                    │  GND ────┴─────────┼───┘
                    └───────────────────┘
                                   ESP32 GPIO 6
```

### ⚠️ CRITICAL: Voltage Divider for SE045 Sensors

**Why it's needed:**
- SE045 outputs 0-4.5V max
- ESP32 ADC max input: **3.3V**
- **Exceeding 3.3V WILL DAMAGE the ESP32 permanently!**

**Voltage Divider Calculation:**
```
Vout = Vin × (R2 / (R1 + R2))
     = 4.5V × (22kΩ / (10kΩ + 22kΩ))
     = 4.5V × 0.6875
     = 3.09V ✅ Safe!
```

**Physical Construction (per sensor):**
```
SE045 Signal ──┬── 10kΩ ──┬── ESP32 GPIO (5 or 6)
               │          │
               └─ 22kΩ ───┴── GND
```

---

## 🔌 Pin-by-Pin Connection Table

### ESP32-S3 Connections:

| ESP32 Pin | Connects To | Wire Color (Suggested) | Function |
|-----------|-------------|------------------------|----------|
| **GPIO 4** | YF-S401 Signal | Yellow | Flow sensor pulses (interrupt) |
| **GPIO 5** | Voltage Divider Output (Tank 1) | Orange | ADC input (0-3.3V safe) |
| **GPIO 6** | Voltage Divider Output (Tank 2) | Blue | ADC input (0-3.3V safe) |
| **GPIO 7** | LED Anode (+) via 220Ω | Green | Status indicator |
| **3.3V** | All sensor VCC pins | Red | Power supply |
| **GND** | All sensor GND + LED Cathode | Black | Common ground |

### YF-S401 Flow Sensor:

| Sensor Pin | Color | Connects To | Notes |
|------------|-------|-------------|-------|
| **VCC** | Red | ESP32 3.3V or 5V | Can use 3.3V or 5V |
| **Signal** | Yellow | ESP32 GPIO 4 | Pulse output (Hall effect) |
| **GND** | Black | ESP32 GND | Ground |

**Installation:** Mount in-line between pump output and Tank 1 input. Arrow on sensor body shows flow direction.

### SE045 Water Level Sensors (×2):

| Sensor Pin | Connects Via | Final Destination | Notes |
|------------|--------------|-------------------|-------|
| **VCC** | Direct | ESP32 3.3V | Power input |
| **Signal** | **10kΩ/22kΩ divider** | ESP32 GPIO 5 (Tank 1)<br>ESP32 GPIO 6 (Tank 2) | **Must use voltage divider!** |
| **GND** | Direct | ESP32 GND | Ground |

**Installation:** Mount vertically inside each tank. Sensor should be fully immersed for full-scale reading (40mm).

---

## 🔧 Upload Instructions

### Prerequisites
⚠️ **IMPORTANT:** Requires **ESP32 Arduino Core 3.0.0 or higher**

1. **Open Arduino IDE**
2. **Configure Board:**
   - Tools → Board → ESP32 Arduino → **ESP32S3 Dev Module**
   - Tools → USB CDC On Boot → **Enabled**
   - Tools → Port → Select your COM port

3. **Upload:**
   - Open `02_sensor_reading.ino`
   - Click Upload button
   - Wait for "Done uploading"

4. **Open Serial Monitor:**
   - Tools → Serial Monitor
   - Set baud rate to **115200**
   - Set line ending to **Newline**

---

## 📡 UART Commands

### Sensor Reading Commands

| Command | Description | Example Output |
|---------|-------------|----------------|
| `FLOWRAW` | Raw pulses/second from YF-S401 | `Pulsos/segundo: 156` |
| `FLOWLPM` | Flow rate in liters/minute | `Flujo: 1.245 L/min` |
| `LEVEL1` | Water level Tank 1 in mm | `Nivel Tanque 1: 15.32 mm` |
| `LEVEL2` | Water level Tank 2 in mm | `Nivel Tanque 2: 8.47 mm` |
| `ALLSENSORS` | Read all sensors at once | Full sensor report |

### Streaming Commands

| Command | Description | Example |
|---------|-------------|---------|
| `STREAM,<ms>` | Start data streaming | `STREAM,500` (every 500ms) |
| `STOPSTREAM` | Stop streaming | `STOPSTREAM` |

### Calibration Commands

| Command | Description | Example |
|---------|-------------|---------|
| `CALIBFLOW,<K>` | Set flow sensor K factor | `CALIBFLOW,95.5` |
| `RESETVOL` | Reset accumulated volume | `RESETVOL` |

### System Commands

| Command | Description |
|---------|-------------|
| `STATUS` | Show complete system status |
| `HELP` | Display command list |

---

## 🧪 Testing Procedure

### Step 1: Dry Test (No Water)

```
1. Upload code to ESP32-S3
2. Open Serial Monitor (115200 baud)
3. You should see:
   - Welcome banner
   - "[OK] Pines configurados"
   - "[OK] ADC inicializado (12 bits)"
   - "[OK] Interrupción de flujo habilitada"
   - "[READY] Sistema listo"
```

**Test commands:**
```
> ALLSENSORS
--- LECTURA DE SENSORES ---
Flujo: 0.000 L/min
Volumen total: 0.00 L
Nivel Tanque 1: 37.00 mm    ← Typical: 37-45mm when DRY (sensor baseline)
Nivel Tanque 2: 41.00 mm    ← Typical: 37-45mm when DRY (sensor baseline)
---------------------------
```

**⚠️ IMPORTANT - SE045 Sensor Behavior:**
- **DRY sensor (no water contact):** Reads ~37-45mm (not 0mm!)
- **WET sensor (water contact):** Reading increases from baseline
- This is NORMAL - SE045 has internal resistance causing baseline offset
- To measure actual water depth: subtract dry baseline from wet reading

### Step 2: Flow Sensor Test (Water Flow)

```
1. Connect water source to pump
2. Route pump output through YF-S401 sensor
3. Start pump at low speed
4. Type: FLOWLPM
   Expected: Non-zero flow rate (e.g., 0.5-2.0 L/min)

5. Type: FLOWRAW
   Expected: Pulse count > 0 (typical: 50-200 pulses/s)

6. Increase pump speed
   Expected: Flow rate increases proportionally
```

### Step 3: Water Level Sensor Test

```
1. Place SE045 sensors in tanks (vertical position)
2. Add water gradually to Tank 1
3. Type: LEVEL1
   Expected: Reading increases with water level

4. Test Tank 2 sensor similarly
5. Remove sensors from water
   Expected: Reading returns to ~0 mm
```

### Step 4: Streaming Test (Serial Plotter)

```
1. Type: STREAM,500
   Response: "[STREAM] Iniciado cada 500 ms"
   
2. Tools → Serial Plotter
   - You should see 3 graphs:
     * Flow: Flow rate over time
     * Level1: Tank 1 level
     * Level2: Tank 2 level

3. Create flow/level changes
   - Graphs should update in real-time

4. Stop streaming: STOPSTREAM
```

---

## 📊 Expected Output Examples

### ALLSENSORS Command:
```
[CMD] Recibido: ALLSENSORS

--- LECTURA DE SENSORES ---
Flujo: 1.234 L/min
Volumen total: 15.67 L
Nivel Tanque 1: 52.45 mm    ← (Baseline ~40mm + 12mm water = 52mm)
Nivel Tanque 2: 47.78 mm    ← (Baseline ~40mm + 8mm water = 48mm)
---------------------------

NOTE: To get actual water depth, subtract dry baseline (~37-45mm)
```

### STATUS Command:
```
========== ESTADO DEL SISTEMA ==========

[SENSOR DE FLUJO YF-S401]
  Flujo actual: 1.234 L/min
  Volumen total: 15.67 L
  Factor K: 98.00 pulsos/(L/min)
  Pulsos/seg: 123

[SENSORES DE NIVEL SE045]
  Tanque 1: 18.45 mm
  Tanque 2: 12.78 mm

[CONFIGURACIÓN]
  Streaming: ACTIVO
  Intervalo: 500 ms
  ADC resolución: 12 bits
  Filtro promedio: 5 muestras

  Tiempo activo: 145 s
========================================
```

### Serial Plotter Output (STREAM,500):
```
Flow:1.234	Level1:18.45	Level2:12.78
Flow:1.240	Level1:18.52	Level2:12.81
Flow:1.229	Level1:18.48	Level2:12.75
...
```

---

## 🔬 Calibration Procedures

### Calibrating YF-S401 Flow Sensor

**Nominal K Factor:** 98 pulses per L/min (5880 pulses/L)

**Experimental Calibration:**

1. **Setup:**
   - Connect sensor inline with pump
   - Prepare graduated container (1-2 liters)
   - Have stopwatch ready

2. **Data Collection:**
   ```
   > RESETVOL          ← Start fresh
   > STREAM,1000       ← Stream every 1 second
   
   - Start pump at constant speed
   - Collect water in container for 60 seconds
   - Note volume collected (e.g., 1.5 L)
   - Read total pulses from Serial Monitor
   ```

3. **Calculate K Factor:**
   ```
   K = Total Pulses / (Volume / Time)
   
   Example:
   - Total pulses: 8800
   - Volume: 1.5 L
   - Time: 60 s = 1 min
   
   K = 8800 / (1.5 L/min) = 5867 pulses/L
     = 97.8 pulses per L/min
   ```

4. **Apply Calibration:**
   ```
   > CALIBFLOW,97.8   ← Update K factor
   ```

5. **Verify:**
   - Repeat test with new K
   - Flow reading should match actual flow

### Calibrating SE045 Level Sensors

**Default Range:** 0-40mm water height  
**⚠️ ACTUAL BEHAVIOR:** Sensor reads 37-45mm when DRY (baseline offset due to internal resistance)

**Experimental Calibration:**

1. **Zero Point (Dry Baseline):**
   ```
   - Remove sensor from water (completely dry)
   - Type: LEVEL1
   - Note reading (expected: 37-45mm, NOT 0mm!)
   - This is your BASELINE value
   
   Example: Baseline = 40.0mm
   ```

2. **Known Depth Test (10mm):**
   ```
   - Submerge sensor to exactly 10mm depth (use ruler)
   - Type: LEVEL1
   - Note reading
   
   Example:
   - Baseline (dry): 40.0mm
   - At 10mm depth: 48.5mm
   - Actual water depth: 48.5 - 40.0 = 8.5mm
   - Scaling factor: 10 / 8.5 = 1.18
   ```

3. **Full Scale Test (40mm):**
   ```
   - Submerge sensor to exactly 40mm depth
   - Type: LEVEL1
   - Calculate: Reading - Baseline = Actual change
   
   Example:
   - At 40mm depth: 74.2mm
   - Actual change: 74.2 - 40.0 = 34.2mm
   - Should be 40mm → needs calibration
   ```

4. **Multi-Point Calibration Table:**
   ```
   Actual Depth | Sensor Reading | Corrected Depth | Error
   -------------|----------------|-----------------|-------
   0mm (dry)    | 40.0mm         | 0.0mm          | -
   10mm         | 48.5mm         | 8.5mm          | -1.5mm
   20mm         | 57.8mm         | 17.8mm         | -2.2mm
   30mm         | 66.1mm         | 26.1mm         | -3.9mm
   40mm         | 74.2mm         | 34.2mm         | -5.8mm
   
   To get actual depth: depth_real = (reading - 40.0) × correction_factor
   ```

5. **Creating Calibration Curve:**
   ```
   - Plot: Actual Depth (X) vs. Sensor Reading - Baseline (Y)
   - Fit linear regression: Y = m×X + b
   - Use m and b for software correction
   ```

6. **Important Notes:**
   - SE045 baseline (dry reading) varies between sensors (37-45mm typical)
   - Always measure YOUR sensor's specific baseline
   - Temperature can affect readings slightly (~0.5mm per 10°C)
   - Sensor must be vertical for accurate readings

---

## ❌ Troubleshooting

### Problem: Flow sensor always reads 0.000 L/min

**Check:**
- [ ] Is water actually flowing through sensor?
- [ ] YF-S401 signal connected to GPIO 4?
- [ ] Sensor powered (VCC connected)?
- [ ] Flow direction correct? (Check arrow on sensor body)
- [ ] Flow rate above minimum (0.3 L/min)?

**Debug:**
```
> FLOWRAW
Expected: > 0 pulses/second if water flowing
If 0: No pulses detected - check wiring/power
```

**Test interrupt:**
- Manually spin sensor rotor with finger
- Should see pulses increase

---

### Problem: Level sensor reads 37-45mm even when dry

**This is NORMAL behavior! ✅**

**Explanation:**
- SE045 sensors have internal resistance that creates a baseline voltage
- Dry sensor typically reads 37-45mm (varies by unit)
- This is NOT a malfunction - it's how the sensor works

**How to use it:**
1. Measure baseline when dry: e.g., 40mm
2. Measure reading with water: e.g., 55mm
3. Actual water depth = 55 - 40 = **15mm**

**If reading is OUTSIDE 37-45mm range when dry:**
- Check voltage divider (should be 10kΩ/22kΩ)
- Verify ADC pin connections
- Clean sensor contacts (oxidation affects baseline)

---

### Problem: Level sensor reads maximum (>70mm) when dry

**Cause:** Voltage divider probably backwards or missing

**Fix:**
1. Disconnect SE045 signal from ESP32
2. Measure voltage with multimeter at GPIO pin:
   - Dry sensor: Should be ~0V
   - Wet sensor (40mm): Should be ~3.0V
3. If reversed: Swap 10kΩ and 22kΩ positions

---

### Problem: Serial Plotter shows erratic/noisy data

**Solutions:**

1. **Increase filter size:**
   ```cpp
   const int FILTER_SIZE = 10;  // Increase from 5
   ```

2. **Increase ADC sample count:**
   ```cpp
   #define ADC_SAMPLES 20  // Increase from 10
   ```

3. **Slower streaming:**
   ```
   > STREAM,1000   ← Reduce update rate
   ```

4. **Check power supply:**
   - Unstable 3.3V can cause noise
   - Use quality USB cable
   - Add 100μF capacitor near sensors

---

### Problem: Compilation error

See Sprint 1 README for ESP32 Core 3.0+ installation

---

## 📈  Performance Metrics

### Expected Specifications:

| Parameter | Specification | Notes |
|-----------|--------------|-------|
| **Flow Sensor Resolution** | 0.01 L/min | At 1-second update rate |
| **Flow Sensor Range** | 0.3 - 6 L/min | Outside range: unreliable |
| **Level Sensor Resolution** | 0.1 mm | With filtering |
| **Level Sensor Range** | 0 - 40 mm | Linear response |
| **ADC Resolution** | 12 bits (0-4095) | ~0.8mV per count |
| **Update Rate (Flow)** | 1 Hz | Fixed (1 second window) |
| **Streaming Rate** | 2-10 Hz | Configurable (100-500ms) |
| **Filter Response Time** | ~5 samples | Moving average |

---

## 🎯 Success Criteria

✅ **You successfully completed Sprint 2 if:**

1. Flow sensor detects water flow (FLOWLPM > 0)
2. Flow rate increases when pump speed increases
3. Level sensors respond to water depth changes
4. Voltage dividers protect ESP32 (no damage!)
5. STREAM command produces real-time graphs
6. STATUS shows all sensors with valid data
7. Calibration commands update readings correctly
8. No ADC over-voltage warnings/damage

---

## 📝 Data to Collect for Lab Report

### Characterization Tests:

1. **Flow Sensor Calibration Curve:**
   - Test at 5-10 different pump speeds
   - Record: Pump PWM, Actual volume, Sensor reading
   - Plot: Sensor reading vs. Actual flow

2. **Level Sensor Calibration Curve:**
   - Test at 0, 10, 20, 30, 40mm depths
   - Record: Actual depth, ADC value, Calculated depth
   - Plot: ADC vs. Depth (should be linear)

3. **Flow Sensor Response Time:**
   - Suddenly change pump speed
   - Measure time to reach 90% of new reading
   - Typical: 1-2 seconds

4. **Level Sensor Noise:**
   - Record 100 samples at constant level
   - Calculate standard deviation
   - Good: σ < 0.5mm

### Screenshots/Data to Capture:

- [ ] Serial Monitor with ALLSENSORS output
- [ ] Serial Plotter showing 3 channels simultaneously
- [ ] STATUS command showing full configuration
- [ ] Calibration data table (Excel/CSV)
- [ ] Photos of voltage divider circuit
- [ ] Photos of sensors installed in system

---

## 🔜 Next Steps

After mastering Sprint 2, proceed to:

**Sprint 3: Sensor-Actuator Integration**
- Combine motor control (Sprint  1) + sensor reading (Sprint 2)
- Open-loop characterization: PWM vs actual flow
- Data logging for MATLAB system identification
- Prepare for PID controller design

---

## 📞 Support

**Sensor-Specific Issues:**
- YF-S401 Datasheet: Check pulse-per-liter specification
- SE045 Manual: Verify voltage output range
- Voltage divider calculator: https://ohmslawcalculator.com/voltage-divider-calculator

**Remember:** 
- ⚠️ NEVER exceed 3.3V on ESP32 ADC pins!
- ✅ ALWAYS use voltage dividers for SE045 sensors!
- 🔌 Test voltage divider with multimeter before connecting to ESP32!

---

**Good luck with sensor integration! 🌊📊**
