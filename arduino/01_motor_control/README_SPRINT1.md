# SPRINT 1: Motor Control - Quick Start Guide

## 📋 Hardware Setup

### Connections Required

**⚠️ PIN RESTRICTION:** Only use GPIOs 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 46

```
ESP32-S3 → H-Bridge L298N
─────────────────────────
GPIO 17  → ENA (Enable/PWM)
GPIO 15  → IN1 (Direction 1)
GPIO 16  → IN2 (Direction 2)
GPIO 7   → LED Status (with 220Ω resistor to GND)
GND      → GND (common ground)

H-Bridge → Motor
────────────────
OUT1, OUT2 → Motor terminals (12V DC)

Power Supply
────────────
12V/2A → H-Bridge power input (+12V, GND)
USB    → ESP32-S3 programming/power
```

### ⚠️ IMPORTANT SAFETY:
- **DO NOT** power motor before uploading code
- Verify all connections before plugging 12V
- Use separate power supply for H-Bridge (not USB)
- Common GND between ESP32 and H-Bridge is REQUIRED

---

## 🔧 Upload Instructions

1. **Open Arduino IDE**
2. **Configure Board:**
   - Tools → Board → ESP32 Arduino → **ESP32S3 Dev Module**
   - Tools → USB CDC On Boot → **Enabled**
   - Tools → Port → Select your COM port

3. **Upload:**
   - Click Upload button
   - Wait for "Done uploading" message

4. **Open Serial Monitor:**
   - Tools → Serial Monitor
   - Set baud rate to **115200**
   - Set line ending to **Newline** or **Both NL & CR**

---

## 📡 UART Commands

### Basic Commands

| Command | Description | Example |
|---------|-------------|---------|
| `PWM,<value>` | Set PWM directly (0-255) | `PWM,128` |
| `SPEED,<pct>` | Set speed as percentage (0-100) | `SPEED,50` |
| `DIR,<dir>` | Set direction (0=forward, 1=reverse) | `DIR,0` |
| `RAMP,<value>` | Smooth ramp to target PWM | `RAMP,200` |
| `STOP` | Emergency stop | `STOP` |
| `STATUS` | Show current state | `STATUS` |
| `TEST` | Run automatic test sequence | `TEST` |
| `HELP` | Show command list | `HELP` |

### Command Format
- Commands are **case-insensitive** (PWM = pwm = Pwm)
- Press **Enter** after typing command
- Wait for confirmation message

---

## 🧪 Testing Procedure

### Step 1: System Check (No Motor Power)
```
1. Upload code to ESP32-S3
2. Open Serial Monitor
3. You should see:
   - Welcome banner
   - "[OK] Pines configurados"
   - "[READY] Sistema listo"
   - Command list
```

### Step 2: Power Up Motor
```
1. Verify all connections one more time
2. Connect 12V power to H-Bridge
3. Type: STATUS
4. Expected: Motor=DETENIDO, PWM=0
```

### Step 3: Basic Motor Test
```
1. Type: SPEED,25
   → Motor should spin slowly (25% power)
   
2. Type: STATUS
   → Should show: PWM=63 (25%), Motor=ACTIVO
   
3. Type: SPEED,50
   → Motor speed should increase
   
4. Type: STOP
   → Motor should stop immediately
```

### Step 4: Direction Test
```
1. Type: DIR,0
   → Direction: ADELANTE
   
2. Type: SPEED,30
   → Motor spins forward
   
3. Type: STOP
   
4. Type: DIR,1
   → Direction: REVERSA
   
5. Type: SPEED,30
   → Motor spins backward
   
6. Type: STOP
```

### Step 5: Ramp Test
```
1. Type: RAMP,200
   → Motor should accelerate smoothly in steps
   → Less mechanical stress than instant start
   
2. Type: RAMP,0
   → Motor should decelerate smoothly
```

### Step 6: Automatic Test
```
1. Type: TEST
   → System runs complete test sequence:
      • Ramp 0→100%
      • Hold 2 seconds
      • Ramp 100%→0%
      • Set 50%
      • Reverse direction
      • Stop
```

---

## 📊 Expected Output Examples

### After SPEED,50 command:
```
[CMD] Recibido: SPEED,50
[MOTOR] PWM establecido: 127 (49.8%)
```

### After STATUS command:
```
--- ESTADO DEL SISTEMA ---
Motor: ACTIVO
PWM: 127 / 255 (49.8%)
Dirección: ADELANTE
Voltaje estimado: 5.98 V
Tiempo activo: 45 s
-------------------------
```

### After RAMP,200 command:
```
[CMD] Recibido: RAMP,200
[RAMP] Iniciando rampa desde 0 hasta 200
[MOTOR] PWM establecido: 5 (2.0%)
[MOTOR] PWM establecido: 10 (3.9%)
...
[MOTOR] PWM establecido: 200 (78.4%)
[RAMP] Rampa completada
```

---

## ❌ Troubleshooting

### Problem: Motor doesn't spin

**Check:**
- [ ] 12V power connected to H-Bridge?
- [ ] Motor properly connected to OUT1/OUT2?
- [ ] Is PWM value high enough? (try PWM,150)
- [ ] Verify with multimeter: voltage on motor terminals?
- [ ] Try manual test: connect motor directly to 12V

**Solution:**
```
# Test PWM signal with oscilloscope on GPIO25
# Should see square wave at 10kHz when PWM>0
```

### Problem: Motor spins but very weak

**Possible causes:**
1. **PWM too low** → Try: `SPEED,80`
2. **12V power insufficient** → Check power supply with multimeter
3. **Motor stalled** → Remove mechanical load, test free-running

### Problem: Serial Monitor shows garbage

**Fix:**
- Change baud rate to **115200**
- Check: Tools → Port → Correct COM port selected
- Press **RESET** button on ESP32

### Problem: "avrdude: stk500_recv(): programmer is not responding"

**Fix:**
- ESP32 is not AVR! Don't worry about this error
- Make sure **ESP32S3 Dev Module** is selected (not Arduino board)
- Hold **BOOT** button while clicking Upload

### Problem: Motor runs continuously, ignores commands

**Emergency:**
1. **Disconnect 12V immediately**
2. Check code uploaded correctly
3. Verify GPIO pins not shorted
4. Re-upload code

---

## 📈 Performance Metrics

### Expected Behavior:
- **PWM Frequency:** 10 kHz (audibly silent)
- **Command Response:** < 100ms
- **Ramp Step:** 5 PWM units per 50ms
- **Full Ramp Time (0→255):** ~2.5 seconds

### PWM to Voltage Conversion:
```
Voltage = (PWM / 255) × 12V

PWM 0    → 0V
PWM 64   → 3.0V
PWM 128  → 6.0V
PWM 192  → 9.0V
PWM 255  → 12.0V
```

---

## 🎯 Success Criteria

✅ **You successfully completed Sprint 1 if:**

1. Code uploads without errors
2. Serial Monitor shows welcome message
3. Motor responds to SPEED commands
4. Direction changes work (DIR,0 vs DIR,1)
5. STOP command immediately halts motor
6. RAMP produces smooth acceleration
7. TEST sequence runs completely
8. STATUS shows accurate information

---

## 📝 Notes for Lab Report

### Data to Record:
1. **PWM vs Motor Speed:** Measure actual flow at different PWM levels
2. **Startup Behavior:** Note minimum PWM for motor to start
3. **Response Time:** Measure delay between command and action
4. **Voltage Readings:** Verify PWM-to-voltage conversion accuracy

### Screenshots to Capture:
- Serial Monitor during TEST sequence
- Oscilloscope showing PWM waveform (GPIO25)
- System STATUS output

---

## 🔜 Next Steps

After mastering Sprint 1, proceed to:

**Sprint 2: Sensor Reading**
- Add YF-S401 flow sensor
- Implement interrupt-based pulse counting
- Read SE045 water level sensors (ADC)
- Real-time data visualization

---

## 📞 Support

**Issues or Questions?**
- Check README.md troubleshooting section
- Review H-Bridge datasheet (L298N/TB6612)
- Verify ESP32-S3 pinout diagram
- Test components individually

**Remember:** Always STOP motor before disconnecting power!

---

**Universidad Militar Nueva Granada**  
*Ingeniería Mecatrónica - 2026*
