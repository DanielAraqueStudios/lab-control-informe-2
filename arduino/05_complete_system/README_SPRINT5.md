# Sprint 5: Sistema Completo

## Descripción General

Sistema de control integral para planta hidráulica de dos tanques con múltiples modos de operación, generación de referencias avanzadas, análisis de métricas de desempeño y logging para MATLAB.

Este es el código **FINAL Y COMPLETO** que integra todos los sprints anteriores en un sistema de control profesional.

## Arquitectura del Sistema

```
┌─────────────────────────────────────────────────────────┐
│                 SISTEMA DE CONTROL                      │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌───────────────┐      ┌──────────────────┐           │
│  │  Generador de │      │  Selector de     │           │
│  │  Referencias  │ ──►  │  Modo de Control │           │
│  └───────────────┘      └──────────────────┘           │
│         │                        │                       │
│         ▼                        ▼                       │
│  ┌─────────────────────────────────────────┐            │
│  │  PID Flow    PID Level1    PID Level2   │            │
│  │  Kp/Ki/Kd    Kp/Ki/Kd      Kp/Ki/Kd     │            │
│  └─────────────────────────────────────────┘            │
│         │                                                │
│         ▼                                                │
│  ┌─────────────────┐                                    │
│  │  Motor Driver   │                                    │
│  │  (H-Bridge PWM) │                                    │
│  └─────────────────┘                                    │
│         │                                                │
│         ▼                                                │
│  ┌─────────────────────────────────────────┐            │
│  │  Planta: 2-Tank Hydraulic System        │            │
│  │                                          │            │
│  │    ┌──────┐     ┌──────┐                │            │
│  │    │Tank 1│ ──► │Tank 2│ ──► Drain      │            │
│  │    └──────┘     └──────┘                │            │
│  └─────────────────────────────────────────┘            │
│         │                                                │
│         ▼                                                │
│  ┌─────────────────────────────────────────┐            │
│  │  Sensores (YF-S401 + 2x SE045)          │            │
│  └─────────────────────────────────────────┘            │
│         │                                                │
│         └────────► Retroalimentación                     │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

---

## Pinout Completo — ESP32-S3

### Diagrama de Conexiones

```
                         ┌──────────────────────────────┐
                         │         ESP32-S3             │
                         │                              │
               3.3V ─────┤ 3V3                    GND  ├───── GND (común)
                         │                              │
   ┌─ YF-S401 (Signal) ──┤ GPIO 4    (INPUT PULLUP)    │
   │                     │                              │
   ├─ SE045 Tank 1 ───── ┤ GPIO 5    (ADC1 / INPUT)    │
   │                     │                              │
   ├─ SE045 Tank 2 ───── ┤ GPIO 6    (ADC1 / INPUT)    │
   │                     │                              │
   │  LED de estado ───── ┤ GPIO 7    (OUTPUT)          │
   │                     │                              │
   │  H-Bridge IN1 ───── ┤ GPIO 15   (OUTPUT)          │
   │                     │                              │
   │  H-Bridge IN2 ───── ┤ GPIO 16   (OUTPUT)          │
   │                     │                              │
   └─ H-Bridge ENA (PWM)─ ┤ GPIO 17   (PWM / OUTPUT)   │
                         │                              │
               USB ──────┤ USB-UART  (115200 baud)      │
                         └──────────────────────────────┘
```

---

### Tabla de Pines

| GPIO | Función | Dirección | Componente | Señal |
|------|---------|-----------|------------|-------|
| **4** | Caudalímetro | INPUT PULLUP | YF-S401 | Pulsos digitales |
| **5** | Nivel Tanque 1 | INPUT (ADC) | SE045 | 0 – 3.3 V analógico |
| **6** | Nivel Tanque 2 | INPUT (ADC) | SE045 | 0 – 3.3 V analógico |
| **7** | LED de estado | OUTPUT | LED + resistencia | HIGH = activo |
| **15** | Motor IN1 | OUTPUT | H-Bridge | DIR giro |
| **16** | Motor IN2 | OUTPUT | H-Bridge | DIR giro |
| **17** | Motor ENA | OUTPUT (PWM) | H-Bridge | Control velocidad |

---

### Configuración del PWM (GPIO 17)

| Parámetro | Valor |
|-----------|-------|
| Frecuencia | 10 000 Hz |
| Resolución | 8 bits (0 – 255) |
| API | `ledcAttach()` (ESP32 Core ≥ 3.0) |

---

### Sensores — Detalles de Conexión

#### YF-S401 — Caudalímetro (GPIO 4)

```
YF-S401          ESP32-S3
  VCC  ─────────  5V  (o 3.3V según módulo)
  GND  ─────────  GND
  OUT  ─────────  GPIO 4  (INPUT_PULLUP activado internamente)
```

> **Factor de conversión:** 7.5 pulsos/segundo → 1 L/min  
> **Rango típico:** 0.3 – 6 L/min

---

#### SE045 — Sensor de Nivel Tanque 1 (GPIO 5)

```
SE045 (Tank 1)   ESP32-S3
  VCC  ─────────  3.3V
  GND  ─────────  GND
  OUT  ─────────  GPIO 5  (ADC1, 12 bits, promedio 10 muestras)
```

---

#### SE045 — Sensor de Nivel Tanque 2 (GPIO 6)

```
SE045 (Tank 2)   ESP32-S3
  VCC  ─────────  3.3V
  GND  ─────────  GND
  OUT  ─────────  GPIO 6  (ADC1, 12 bits, promedio 10 muestras)
```

> **Nota SE045:** Sensor ultrasónico / presión diferencial. La lectura ADC (0–4095)  
> se convierte a mm mediante mapeo lineal configurado en el código.

---

#### H-Bridge — Driver de Motor

```
H-Bridge         ESP32-S3          Bomba
  ENA  ─────────  GPIO 17 (PWM)
  IN1  ─────────  GPIO 15
  IN2  ─────────  GPIO 16
  OUT1 ─────────────────────────── Motor +
  OUT2 ─────────────────────────── Motor −
  VCC  ─────────  12V (fuente externa)
  GND  ─────────  GND (común con ESP32)
```

> **Sentido de giro:** IN1=HIGH, IN2=LOW → giro horario (impulsión)  
> **Frenado:** IN1=LOW, IN2=LOW + ENA=0

---

### Fuentes de Alimentación

| Componente | Tensión | Corriente máx. |
|------------|---------|----------------|
| ESP32-S3 | 5V (USB) / 3.3V (regulado) | 500 mA |
| H-Bridge + Motor | 12V DC | 2–5 A (según bomba) |
| YF-S401 | 5V DC | 15 mA |
| SE045 (×2) | 3.3V DC | 15 mA c/u |

> ⚠️ **GND común obligatorio** entre la fuente de 12V del H-Bridge y el GND del ESP32-S3.

---

## Modos de Operación

### 1. MANUAL

Control directo de PWM sin lazo cerrado.

**Uso:**
```
SETMODE,MANUAL
```

Sin controladores activos. Usuario ajusta PWM directamente desde Sprint 1.

---

### 2. AUTO_FLOW

Control PID de flujo (idéntico a Sprint 4).

**Objetivo:** Regular flujo de entrada al tanque 1

**Uso:**
```
SETMODE,AUTO_FLOW
SETREF,STEP,0.5,1.5,10    # Escalón 0.5→1.5 L/min
STARTCTRL
DATALOG
```

**Ecuación de control:**
```
u(k) = Kp·(Qsp - Q) + Ki·∫(Qsp - Q)dτ + Kd·d(Qsp - Q)/dt
```

Donde:
- Qsp = Setpoint de flujo (L/min)
- Q = Flujo medido por YF-S401 (L/min)
- u(k) = PWM motor (0-255)

---

### 3. AUTO_LEVEL1

Control PID del nivel del Tanque 1.

**Objetivo:** Mantener nivel constante manipulando flujo de entrada

**Uso:**
```
SETMODE,AUTO_LEVEL1
SETPID1,30.0,5.0,3.0      # Ajustar PID
SETREF,STEP,10.0,25.0,15  # Escalón 10→25mm
STARTCTRL
METRICS                    # Iniciar medición de desempeño
DATALOG
```

**Ecuación:**
```
u(k) = Kp·(h1sp - h1) + Ki·∫(h1sp - h1)dτ + Kd·d(h1sp - h1)/dt
```

---

### 4. AUTO_LEVEL2

Control PID del nivel del Tanque 2.

**Objetivo:** Regular nivel del segundo tanque (más difícil por retraso adicional)

**Uso:**
```
SETMODE,AUTO_LEVEL2
SETPID2,25.0,4.0,5.0      # Kd mayor por mayor retraso
SETREF,RAMP,5.0,30.0,40.0 # Rampa de 40 segundos
STARTCTRL
```

**Nota:** Control de nivel 2 es más lento debido a la dinámica acoplada (Tank1 → Tank2).

---

### 5. CASCADE

Control cascada: Nivel 1 → Flujo → Motor

**Arquitectura:**
```
┌─────────────┐     ┌────────────┐     ┌───────┐
│ Setpoint h1 │ ──► │ PID Nivel  │ ──► │ Qsp   │
└─────────────┘     └────────────┘     └───────┘
                                           │
                                           ▼
  ┌─────────┐     ┌────────────┐     ┌───────┐
  │ Q real  │ ◄── │ PID Flujo  │ ◄── │ Qsp   │
  └─────────┘     └────────────┘     └───────┘
        │                                │
        │                                ▼
        │                          ┌─────────┐
        │                          │  Motor  │
        │                          └─────────┘
        │                                │
        └────────────────────────────────┘
```

**Ventajas:**
- Mejor rechazo de perturbaciones
- Respuesta más rápida
- Mayor robustez

**Uso:**
```
SETMODE,CASCADE
SETPID1,30.0,5.0,3.0      # PID externo (nivel)
# PID interno (flujo) ya configurado en AUTO_FLOW
SETREF,STEP,15.0,25.0,20
STARTCTRL
```

## Generador de Referencias

### 1. STEP (Escalón)

```
SETREF,STEP,<inicial>,<final>,<duración>
```

**Ejemplo:**
```
SETREF,STEP,0.5,1.5,10    → Escalón de 0.5 a 1.5 en t=10s
```

**Aplicación:** Análisis de respuesta transitoria (overshoot, settling time)

---

### 2. RAMP (Rampa)

```
SETREF,RAMP,<inicial>,<final>,<duración>
```

**Ejemplo:**
```
SETREF,RAMP,10.0,30.0,45.0    → Rampa lineal de 45 segundos
```

**Ecuación:**
```
r(t) = r₀ + (rf - r₀)·(t/T)    para 0 ≤ t ≤ T
```

**Aplicación:** Prueba de seguimiento, error en régimen permanente

---

### 3. PARA (Parabólica suavizada)

```
SETREF,PARA,<inicial>,<final>,<duración>
```

**Ejemplo:**
```
SETREF,PARA,5.0,35.0,30.0    → Transición suave de 30s
```

**Ecuación (polinomio 3-2-3):**
```
τ = t/T
r(t) = r₀ + (rf - r₀)·(3τ² - 2τ³)
```

**Aplicación:** Minimizar sobreimpulso, transiciones suaves

---

### 4. SINE (Senoidal) - Próxima versión

Referencia sinusoidal para análisis en frecuencia (Bode experimental).

## Métricas de Desempeño Automáticas

### Activación

```
STARTCTRL
METRICS    → Inicia medición automática
... esperar 60 segundos ...
METRICS    → Muestra resultados
```

### Métricas Calculadas

| Métrica | Descripción | Fórmula |
|---------|-------------|---------|
| **Overshoot** | Sobreimpulso máximo | Mp = ((Ypeak - Yss) / Yss) × 100% |
| **Rise Time** | Tiempo 10% → 90% | tr = t(90%) - t(10%) |
| **Settling Time** | Tiempo hasta banda ±2% | ts (criterio 2%) |
| **Steady-State Error** | Error permanente | ess = \|Yss - Ysp\| / Ysp × 100% |

### Ejemplo de Salida

```
╔════════════════════════════════════════════════════╗
║          MÉTRICAS DE DESEMPEÑO                     ║
╠════════════════════════════════════════════════════╣
║  Overshoot máximo:      12.34%
║  Rise Time:             3.45 s
║  Settling Time (2%):    8.12 s
║  Error estado estable:  1.23%
║  Valor pico:            1.123
╚════════════════════════════════════════════════════╝
```

## Experimentos Predefinidos

### 1. STEP_FLOW

Respuesta escalón en control de flujo.

```
EXPERIMENT,STEP_FLOW
```

Ejecuta:
- Modo AUTO_FLOW
- Escalón 0.5 → 1.5 L/min a los 10s
- Logging activo 60s
- Métricas automáticas

---

### 2. RAMP_LEVEL

Seguimiento de rampa en control de nivel.

```
EXPERIMENT,RAMP_LEVEL
```

Ejecuta:
- Modo AUTO_LEVEL1
- Rampa 10 → 30mm en 30s
- Logging CSV

---

### 3. DISTURBANCE

Simulación de perturbación (reducción temporal de PWM).

```
EXPERIMENT,DISTURBANCE
```

Aplica:
- Reducción de PWM al 50% durante 2 segundos
- Observar rechazo de perturbación del PID

## Data Logging Avanzado

### Formato CSV Completo

```
Time_s,Mode,RefType,Ref,Flow,Level1,Level2,PWM,PID_P,PID_I,PID_D,Error,Volume
0.100,AUTO_FLOW,1,1.000,0.234,12.4,8.3,153,38.3,7.7,38.3,0.766,0.001
0.200,AUTO_FLOW,1,1.000,0.456,13.2,8.9,189,27.2,12.9,11.1,0.544,0.002
...
```

### Exportación a MATLAB

```matlab
% Leer datos
data = readtable('experiment_data.csv');

% Graficar respuesta
figure;
subplot(2,1,1);
plot(data.Time_s, data.Ref, 'r--', 'LineWidth', 2);
hold on;
plot(data.Time_s, data.Flow, 'b-', 'LineWidth', 1.5);
xlabel('Time (s)');
ylabel('Flow (L/min)');
legend('Setpoint', 'Real');
grid on;

subplot(2,1,2);
plot(data.Time_s, data.PWM);
xlabel('Time (s)');
ylabel('PWM (0-255)');
grid on;

% Calcular métricas
[peak, idx] = max(data.Flow);
overshoot = ((peak - data.Ref(end)) / data.Ref(end)) * 100;
fprintf('Overshoot: %.2f%%\n', overshoot);
```

### Exportación a Python

```python
import pandas as pd
import matplotlib.pyplot as plt

# Cargar datos
df = pd.read_csv('experiment_data.csv')

# Graficar
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))

ax1.plot(df['Time_s'], df['Ref'], 'r--', label='Setpoint', linewidth=2)
ax1.plot(df['Time_s'], df['Flow'], 'b-', label='Flow', linewidth=1.5)
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('Flow (L/min)')
ax1.legend()
ax1.grid(True)

ax2.plot(df['Time_s'], df['PWM'], 'g-')
ax2.set_xlabel('Time (s)')
ax2.set_ylabel('PWM')
ax2.grid(True)

plt.tight_layout()
plt.show()

# Métricas
peak = df['Flow'].max()
setpoint = df['Ref'].iloc[-1]
overshoot = ((peak - setpoint) / setpoint) * 100
print(f'Overshoot: {overshoot:.2f}%')
```

## Comandos Completos

```
SETMODE,<mode>              - MANUAL | AUTO_FLOW | AUTO_LEVEL1 | 
                              AUTO_LEVEL2 | CASCADE

SETREF,<type>,<params>      - STEP,<ini>,<fin>,<dur>
                              RAMP,<ini>,<fin>,<dur>
                              PARA,<ini>,<fin>,<dur>

SETPID1,<Kp>,<Ki>,<Kd>      - Ajustar PID Tanque 1

SETPID2,<Kp>,<Ki>,<Kd>      - Ajustar PID Tanque 2

STARTCTRL                   - Iniciar control automático

STOPCTRL                    - Detener control y motor

METRICS                     - Toggle medición métricas

DATALOG                     - Toggle logging CSV

EXPERIMENT,<name>           - STEP_FLOW | RAMP_LEVEL | DISTURBANCE

STATUS                      - Estado completo del sistema

HELP                        - Lista de comandos
```

## Ejemplo de Sesión Completa

### Experimento 1: Control de Flujo con Escalón

```
# 1. Configurar modo
SETMODE,AUTO_FLOW

# 2. Definir referencia escalón
SETREF,STEP,0.5,1.5,10

# 3. Iniciar control
STARTCTRL

# 4. Activar logging
DATALOG

# 5. Iniciar medición de métricas
METRICS

# 6. [Esperar 60 segundos]

# 7. Ver métricas
METRICS

# 8. Detener
STOPCTRL

# 9. Copiar CSV desde Serial Monitor → experiment1.csv
```

### Experimento 2: Control Cascada de Nivel

```
# 1. Modo cascada
SETMODE,CASCADE

# 2. Ajustar PIDs
SETPID1,30.0,5.0,3.0

# 3. Referencia parabólica
SETREF,PARA,15.0,30.0,25.0

# 4. Iniciar
STARTCTRL
DATALOG
METRICS

# 5. [Esperar completar transición]

# 6. Simular perturbación
EXPERIMENT,DISTURBANCE

# 7. [Observar rechazo]

# 8. Detener
STOPCTRL
```

## Indicadores LED

| Patrón | Modo |
|--------|------|
| Parpadeo lento (1s) | MANUAL |
| Parpadeo rápido (250ms) | AUTO_FLOW, AUTO_LEVEL1, AUTO_LEVEL2 |
| Parpadeo muy rápido (100ms) | CASCADE |

## Especificaciones Técnicas

### Rangos Operativos

| Variable | Mínimo | Máximo | Nominal |
|----------|--------|--------|---------|
| Flujo | 0.3 L/min | 6.0 L/min | 1.0 L/min |
| Nivel Tank 1 | 5 mm | 40 mm | 20 mm |
| Nivel Tank 2 | 5 mm | 40 mm | 15 mm |
| PWM | 0 | 255 | 150 |

### Frecuencias de Muestreo

| Sensor/Actuador | Frecuencia | Período |
|-----------------|------------|---------|
| YF-S401 (flujo) | 1 Hz | 1000 ms |
| SE045 (niveles) | 1 Hz | 1000 ms |
| Loop control PID | 10 Hz | 100 ms |
| Logging CSV | 10 Hz | 100 ms |
| PWM motor | 10 kHz | 0.1 ms |

## Troubleshooting

### Problema: Control inestable (oscilaciones)

**Diagnóstico:**
```
STATUS    → Ver parámetros PID
```

**Solución:**
```
# Reducir ganancias
STOPCTRL
SETPID1,20.0,3.0,1.0    # Reducir todas las ganancias 30%
STARTCTRL
```

---

### Problema: Error en estado estable alto

**Diagnóstico:**
```
METRICS    → Ver steady-state error
```

**Solución:**
```
# Aumentar Ki
STOPCTRL
SETKI,15.0    # Incrementar ganancia integral
STARTCTRL
```

---

### Problema: Overshoot excesivo

**Solución:**
```
# Reducir Kp, aumentar Kd
STOPCTRL
SETKP,35.0    # Reducir proporcional
SETKD,8.0     # Aumentar derivativo
STARTCTRL
```

---

### Problema: Datos CSV corruptos

**Causa:** Buffer serial lleno

**Solución:**
1. Aumentar `logInterval` a 200ms
2. Reducir duración del experimento
3. Usar herramientas tipo CoolTerm en lugar de Serial Monitor

## Compilación y Carga

**Plataforma:**
- ESP32-S3 DevKit
- ESP32 Arduino Core >= 3.0.0

**Settings Arduino IDE:**
```
Board: ESP32S3 Dev Module
Upload Speed: 921600
USB CDC On Boot: Enabled
CPU Frequency: 240MHz
Flash Size: 8MB
Partition Scheme: Default 4MB with spiffs
```

**Bibliotecas:**
- Ninguna externa (solo Arduino.h y ESP32 HAL)

## Próximos Desarrollos

1. **Comunicación WiFi** → Monitoreo remoto vía web dashboard
2. **Control adaptativo** → Ajuste automático de parámetros PID
3. **Predicción MPC** → Model Predictive Control para referencias complejas
4. **IoT logging** → ThingSpeak/InfluxDB para almacenamiento en nube

## Referencias Técnicas

- **Control cascada:** "Process Control: Designing Processes and Control Systems for Dynamic Performance" - T. Marlin
- **Sintonización PID:** "Advanced PID Control" - K.J. Åström & T. Hägglund
- **Sistemas hidráulicos:** "Modeling and Control of Dynamical Systems" - F.L. Lewis

---

**Versión:** 2.0  
**Autores:** Daniel García Araque, David Santiago García Suarez  
**Fecha:** Febrero 2026  
**Licencia:** Uso académico UMNG
