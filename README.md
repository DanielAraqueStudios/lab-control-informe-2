# LABORATORIO 2: Control de Flujo en un Sistema Hidráulico

## Información General

**Universidad:** Universidad Militar Nueva Granada  
**Facultad:** Ingeniería  
**Programa:** Ingeniería en Mecatrónica  
**Asignatura:** Control Lineal y Laboratorio  
**Semestre:** Séptimo  
**Fecha:** 2026-1  

**Plataforma de Control:** ESP32-S3 DevKit  
**Entorno de Desarrollo:** Arduino IDE  
**Repositorio:** lab-control-informe-2

---

## Objetivo General

Controlar el flujo de agua de entrada a un sistema de dos tanques mediante la implementación de un controlador PID digital en microcontrolador ESP32-S3, sintonizado por el método de Ziegler-Nichols, con seguimiento de referencias tipo escalón, rampa y aceleración.

---

## Objetivos Específicos

1. **Implementar e instrumentar** un sistema hidráulico similar al mostrado en la Figura 1, incluyendo sensores y acondicionamiento de señales.

2. **Encontrar la representación matemática** de la dinámica del sistema hidráulico y compararla contra la respuesta del sistema real.

3. **Determinar un regulador PID** utilizando el método de sintonización de Ziegler-Nichols, validándolo en simulación para asegurar que el flujo de entrada siga referencias tipo:
   - Posición (escalón)
   - Velocidad (rampa)

4. **Implementar en electrónica análoga** el controlador obtenido para que el flujo de entrada haga seguimiento a referencias tipo escalón, validando el funcionamiento en el sistema real.

---

## Descripción del Sistema

### Arquitectura del Sistema

El sistema hidráulico implementado consta de un sistema de dos tanques interconectados con las siguientes características:

#### Subsistema Hidráulico

- **Tanque 1 (Principal):** Recibe flujo de entrada desde bomba peristáltica
- **Tanque 2 (Secundario):** Recibe flujo desde Tanque 1 por gravedad
- **Bomba peristáltica 12V DC:** Elemento actuante para control de flujo de entrada
- **Válvulas de bola:** Control manual de interconexión entre tanques
- **Reservorio:** Almacenamiento de agua con recirculación

#### Subsistema de Control y Adquisición

- **Microcontrolador:** ESP32-S3 DevKit (arquitectura Xtensa dual-core 240 MHz)
- **Driver de potencia:** H-Bridge para control bidireccional de bomba
- **Sensor de flujo:** YF-S401 (rango 0.3-6 L/min, salida digital por pulsos)
- **Sensores de nivel:** 2x Water Level Sensor SE045 (uno por tanque)
- **Comunicación:** UART/USB para monitoreo y programación
- *Especificaciones de Hardware

### Lista de Componentes

#### Microcontrolador y Electrónica

| Componente | Modelo/Especificación | Cantidad | Función |
|------------|----------------------|----------|---------|
| Microcontrolador | ESP32-S3 DevKit (Dual Core 240MHz, WiFi/BLE) | 1 | Control principal y procesamiento |
| Driver Motor | H-Bridge L298N o TB6612FNG | 1 | Control bidireccional de bomba 12V |
| Fuente DC | 12V/2A switching power supply | 1 | Alimentación bomba y electrónica |
| Regulador | AMS1117-5V o similar | 1 | Regulación 5V para ESP32 (si no USB) |
| Protoboard | 830 puntos | 2 | Montaje circuito de control |
| Cables Dupont | M-M, M-F, F-F | 40+ | Conexiones |

#### Sensores e Instrumentación

| Componente | Modelo/Especificación | Cantidad | Función |
|------------|----------------------|----------|---------|
| Sensor de Flujo | **YF-S401** (0.3-6 L/min, hall effect) | 1 | Medición caudal entrada |
| Sensor de Nivel | **Water Level Sensor SE045** (analógico) | 2 | Nivel Tanque 1 y Tanque 2 |
| Multímetro Digital | Resolución 0.1V | 1 | Verificación voltajes |
| Osciloscopio Digital | 2+ canales, 50MHz+ | 1 | Análisis señales PWM/sensores |

#### Sistema Hidráulico

| Componente | Especificación | Cantidad | Función |
|------------|---------------|----------|---------|
| Bomba Peristáltica | 12V DC, 0-2 L/min | 1 | Actuador de flujo |
| Tanque 1 | Vaso precipitado 1-2L | 1 | Tanque principal |
| Tanque 2 | Vaso precipitado 1-2L | 1 | Tanque secundario |
| Válvulas de Bola | 1/4" o 3/8" | 2-3 | Control manual flujo |
| Mangueras | Transparente 1/4" ID | 3-5 m | Conducción agua |
| Reservorio | Contenedor 5L | 1 | Almacenamiento agua |
| Abrazaderas | Metálicas ajustables | 10-15 | Fijación mangueras |

#### Software y Herramientas

| Software | Versión | Función |
|----------|---------|---------|
| Arduino IDE | 2.3.0+ | Programación ESP32-S3 |
| ESP32 Board Package | 3.0.0+ | Soporte ESP32-S3 en Arduino |
| MATLAB/Simulink | R2020a+ | Simulación y análisis |
| Serial Plotter/Monitor | Integrado Arduino | Visualización datos tiempo real |
| Python (opcional) | 3.9+ con matplotlib | Análisis avanzado de datoariable)
3. YF-S401 mide flujo de entrada (pulsos → L/min)
4. SE045 sensores miden niveles en ambos tanques
5. Controlador PID digital ajusta PWM según error de referencia
6. Datos transmitidos a PC para monitoreo en tiempo real

---

## Materiales y Equipos

### Equipos del Laboratorio

| Descripción | Cantidad |
|-------------|----------|
| Computador con MATLAB | 1 por grupo |
| Fuente de voltaje | 1 por grupo |
#### 1.1 Montaje del Sistema Hidráulico

- Ensamblar Tanque 1 y Tanque 2 con soporte elevado
- Conectar mangueras entre bomba → Tanque 1 → Tanque 2 → Reservorio
- Instalar válvulas de bola para aislamiento de tanques
- Garantizar hermeticidad con abrazaderas en todas las conexiones
- Implementar bandejas de contención para protección

#### 1.2 Instrumentación y Sensores

**Sensor de Flujo YF-S401:**
- Instalación: En línea entre bomba y Tanque 1
- Conexión: 
  - VCC → 5V ESP32
  - GND → GND
  - Signal → GPIO (configurar como INPUT con interrupción)
- Calibración: 5880 pulsos/litro (verificar experimentalmente)
- Rango operación: 0.3 - 6 L/min

**Sensores de Nivel SE045 (x2):**
- Instalación: Montaje vertical dentro de cada tanque
- Conexión:
  - VCC → 5V ESP32
  - GND → GND
  - Analog Out → GPIO ADC (36-39 en ESP32-S3)
- Rango: 0-40mm altura de agua
- Salida: 0-4.5V proporcional a nivel

**Driver H-Bridge:**
- Modelo: L298N o TB6612FNG
- Conexión:
  - IN1, IN2 → GPIO ESP32 (control dirección)
  - ENA → GPIO PWM ESP32 (control velocidad)
  - OUT1, OUT2 → Motor bomba 12V
  - VCC → 12V fuente externa
  - 5V OUT → No usar (usar regulador dedicado para ESP32)
- PWM: Frecuencia 1-25 kHz, resolución 8-10 bits | 1 sistema completo |
| Sensores de flujo y nivel | 3 sensores |
| Protoboard | 1 por grupo |
| Amplificadores operacionales | 10 por grupo |
| Conjunto de cables | 1 conjunto |
| Resistencias y condensadores | 50 componentes |

---

## Metodología

### Fase 1: Implementación del Sistema Físico

1. **Montaje del sistema hidráulico:**
   - Ensamblar tanques, válvulas y bomba peristáltica
   - Garantizar hermeticidad para evitar derramamientos
   - Implementar medidas de seguridad para protección de equipos electrónicos

2. **Instrumentación:**
   - Instalar sensor de flujo de entrada
   - Instalar sensores de nivel en tanques
   - Implementar acondicionamiento de señales
   - Diseñar manejadores de potencia para la bomba

### Fase 2: Modelado Matemático

1. **Identificación del sistema:**
   - Realizar pruebas en lazo abierto con ESP32
   - Obtener datos experimentales entrada-salida vía Serial
   - Aplicar leyes de conservación de masa
   - Exportar datos a MATLAB para identificación
   - Considerar dinámica de flujo por gravedad

2. **Función de transferencia:**
   - Determinar F(s) = Qin(s) / VDC(s)
   - Donde:
     - qin(t): flujo de entrada al sistema (variable medida)
     - vDC(t): voltaje DC aplicado al motor de la bomba (variable de control)

3. **Validación del modelo:**
   - Comparar respuesta simulada vs. respuesta real
   - Ajustar parámetros si es necesario

### Fase 3: Diseño del Controlador

1. **Sintonización por Ziegler-Nichols:**
   - Método de respuesta a lazo abierto o método de oscilación sostenida
   - Obtener parámetros Kp, Ki, Kd del controlador PID
   - Objetivo: seguimiento de referencias con error de estado estable cero

2. **Referencias a seguir:**
   - **Escalón:** posición constante del flujo
   - **Rampa:** velocidad constante de cambio de flujo
   - **Parábola:** aceleración constante de flujo

3. **Simulación:**
   - Validar diseño en MATLAB/Simulink
   - Verificar especificaciones de desempeño
   - Analizar estabilidad y robustez

### Fase 4: Implementación Digital en ESP32-S3

#### 4.1 Configuración del Entorno Arduino IDE

**Instalación del soporte ESP32-S3:**
```
1. Arduino IDE → Preferences → Additional Board Manager URLs:
   https://espressif.github.io/arduino-esp32/package_esp32_index.json

2. Tools → Board → Boards Manager → buscar "esp32" → Install

3. Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
```

**Configuración de compilación:**
- Board: "ESP32S3 Dev Module"
- USB CDC On Boot: "Enabled"
- CPU Frequency: "240MHz (WiFi)"
- Flash Mode: "QIO 80MHz"
- Flash Size: "4MB (32Mb)"
- Partition Scheme: "Default 4MB with spiffs"
- PSRAM: "Disabled" (o "OPI PSRAM" si disponible)

#### 4.2 Asignación de Pines ESP32-S3

⚠️ **RESTRICCIÓN DE PINES:** Solo se usarán los siguientes GPIOs: **3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 46**

| Función | GPIO | Tipo | Notas |
|---------|------|------|-------|
| **Motor Control (H-Bridge)** ||||
| Motor IN1 | GPIO 15 | Output Digital | Dirección adelante |
| Motor IN2 | GPIO 16 | Output Digital | Dirección reversa |
| PWM Motor (ENA) | GPIO 17 | Output PWM | Canal LEDC 0, 10kHz, 8-bit |
| **Sensores** ||||
| YF-S401 Signal | GPIO 4 | Input (INT) | Interrupción flanco ascendente |
| SE045 Tank 1 | GPIO 5 (ADC1_CH4) | Input Analog | ADC 12-bit, divisor resistivo |
| SE045 Tank 2 | GPIO 6 (ADC1_CH5) | Input Analog | ADC 12-bit, divisor resistivo |
| **Indicadores** ||||
| LED Status | GPIO 7 | Output | Indicador estado sistema |
| **Comunicación** ||||
| UART TX | USB | UART0_TX | Serial debug/data vía USB |
| UART RX | USB | UART0_RX | Serial debug/data vía USB |
| **Pines Disponibles para Expansión** ||||
| Reserva 1 | GPIO 3 (ADC1_CH2) | Multipropósito | ADC/Digital I/O |
| Reserva 2 | GPIO 8 | Multipropósito | Digital I/O |
| Reserva 3 | GPIO 9 | Multipropósito | Digital I/O |
| Reserva 4 | GPIO 10 | Multipropósito | Digital I/O |
| Reserva 5 | GPIO 11 | Multipropósito | Digital I/O |
| Reserva 6 | GPIO 12 | Multipropósito | Digital I/O |
| Reserva 7 | GPIO 13 | Multipropósito | Digital I/O |
| Reserva 8 | GPIO 18 (ADC2_CH7) | Multipropósito | ADC/Digital I/O |
| Reserva 9 | GPIO 46 | Input Only | Solo entrada |

#### 4.3 Estructura de Programas Arduino (.ino)

El proyecto se divide en módulos incrementales para facilitar desarrollo y pruebas:

**Sprint 1: Control Motor (`01_motor_control.ino`)**
- Inicialización H-Bridge
- Control PWM básico
- Rampa suave de velocidad
- Protección anti-arranque brusco
- Test via Serial Monitor

**Sprint 2: Lectura Sensores (`02_sensor_reading.ino`)**
- Lectura YF-S401 con interrupciones
- Cálculo de flujo en L/min
- Lectura SE045 (ADC) para niveles
- Calibración y filtrado
- Visualización Serial Plotter

**Sprint 3: Integración Sensores-Actuador (`03_sensor_actuator.ino`)**
- Lazo abierto: PWM vs flujo medido
- Caracterización de bomba
- Adquisición de datos para identificación
- Exportar datos a MATLAB

**Sprint 4: Controlador PID (`04_pid_controller.ino`)**
- Implementación algoritmo PID digital
- Anti-windup integrador
- Filtro derivativo
- Seguimiento de referencia escalón
- Ajuste fino de parámetros

**Sprint 5: Sistema Completo (`05_complete_system.ino`)**
- Control de dos tanques
- Múltiples referencias
- Interfaz Serial avanzada
- Registro de datos (SD opcional)
- Monitoreo en tiempo real

---

## Fundamentos Técnicos de Implementación

### Calibración del Sensor YF-S401

**Principio de Operación:**
El sensor YF-S401 genera pulsos digitales mediante efecto Hall proporcionales al flujo volumétrico.

**Ecuación de Conversión:**
```cpp
Frecuencia (Hz) = Factor_K × Flujo (L/min)
Factor_K nominal = 98 pulsos/litro = 5880 pulsos/min/L
```

**Cálculo de Flujo en Código:**
```cpp
volatile unsigned long pulseCount = 0;
float flowRate = 0.0;  // L/min
float totalVolume = 0.0;  // Litros

void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

void calculateFlow() {
    // Frecuencia (Hz) = pulsos/segundo
    float frequency = pulseCount / 1.0;  // Si se llama cada 1 segundo
    
    // Flujo (L/min) = Frecuencia / (Factor_K / 60)
    flowRate = frequency / (98.0 / 60.0);  // L/min
    
    // Volumen acumulado
    totalVolume += (flowRate / 60.0);  // Litros
    
    pulseCount = 0;  // Reset contador
}
```

**Calibración Experimental:**
1. Llenar recipiente graduado durante tiempo conocido (ej: 60 segundos)
2. Contar pulsos totales del sensor
3. Calcular: `Factor_K_real = pulsos_totales / volumen_medido (L)`
4. Actualizar constante en código

### Calibración de Sensores SE045

**Principio de Operación:**
Sensor resistivo analógico que varía voltaje de salida según altura de agua.

**Características:**
- Rango de medición: 0-40 mm (altura agua)
- Voltaje salida: 0V (seco) a 4.5V (sumergido completo)
- Lectura ADC ESP32: 0-4095 (12 bits)

**Ecuación de Conversión:**
```cpp
const int ADC_RESOLUTION = 4095;  // 12-bit ADC
const float ADC_VREF = 3.3;        // Voltaje referencia ESP32
const float SENSOR_MAX_VOLTAGE = 4.5;
const float SENSOR_MAX_HEIGHT_MM = 40.0;

// Factor divisor resistivo: R2/(R1+R2) = 22k/(10k+22k) = 0.6875
const float VOLTAGE_DIVIDER_FACTOR = 0.6875;

float readWaterLevel(int adcPin) {
    // adcPin debe ser GPIO5 (Tank1) o GPIO6 (Tank2)
    int adcValue = analogRead(adcPin);
    
    // Convertir ADC a voltaje en el pin (después del divisor)
    float voltage_pin = (adcValue / (float)ADC_RESOLUTION) * ADC_VREF;
    
    // Compensar divisor resistivo para obtener voltaje real del sensor
    float voltage_sensor = voltage_pin / VOLTAGE_DIVIDER_FACTOR;
    
    // Convertir voltaje del sensor a altura (mm)
    float height_mm = (voltage_sensor / SENSOR_MAX_VOLTAGE) * SENSOR_MAX_HEIGHT_MM;
    
    return height_mm;
}
```

**Calibración Experimental:**
1. Sumergir sensor a alturas conocidas (0, 10, 20, 30, 40 mm)
2. Registrar valores ADC correspondientes
3. Realizar regresión lineal: `ADC = m × altura + b`
4. Actualizar ecuación en código

### Algoritmo PID Digital

**Ecuación Discreta del PID:**

```
u(k) = Kp × e(k) + Ki × sum(e) × dt + Kd × (e(k) - e(k-1)) / dt
```

Donde:
- `u(k)`: Señal de control en instante k (PWM duty cycle)
- `e(k)`: Error en instante k (referencia - medición)
- `dt`: Período de muestreo (segundos)
- `Kp, Ki, Kd`: Parámetros del controlador

**Implementación en ESP32:**

```cpp
// Variables globales PID
float Kp = 10.0, Ki = 2.0, Kd = 1.0;
float setpoint = 1.0;  // Referencia (L/min)
float error = 0, lastError = 0;
float integral = 0, derivative = 0;
float output = 0;
float dt = 0.1;  // 100ms período muestreo

void computePID(float measurement) {
    // Calcular error
    error = setpoint - measurement;
    
    // Término Proporcional
    float P = Kp * error;
    
    // Término Integral con anti-windup
    integral += error * dt;
    if (integral > 255) integral = 255;  // Saturación máxima PWM
    if (integral < 0) integral = 0;      // Saturación mínima PWM
    float I = Ki * integral;
    
    // Término Derivativo con filtrado
    derivative = (error - lastError) / dt;
    float D = Kd * derivative;
    
    // Señal de control total
    output = P + I + D;
    
    // Saturación de salida (0-255 para PWM 8-bit)
    if (output > 255) output = 255;
    if (output < 0) output = 0;
    
    // Actualizar error anterior
    lastError = error;
    
    // Aplicar PWM al motor
    ledcWrite(0, (int)output);  // Canal 0 del LEDC
}
```

**Método de Sintonización Ziegler-Nichols (Implementación):**

```cpp
// Método de Ganancia Última
// 1. Poner Ki = 0, Kd = 0
// 2. Aumentar Kp hasta oscilación sostenida
// 3. Registrar Ku (ganancia última) y Pu (período oscilación)

float Ku = 15.0;  // Ganancia última experimental
float Pu = 5.0;   // Período de oscilación (segundos)

// Calcular parámetros PID según Ziegler-Nichols
Kp = 0.6 * Ku;        // = 9.0
Ki = 2 * Kp / Pu;     // = 3.6
Kd = Kp * Pu / 8;     // = 5.625
```

### Control PWM con LEDC en ESP32-S3

**Configuración del PWM:**

```cpp
// ===== CONFIGURACIÓN DE PINES =====
const int MOTOR_IN1 = 15;        // H-Bridge dirección 1
const int MOTOR_IN2 = 16;        // H-Bridge dirección 2
const int MOTOR_PWM_PIN = 17;    // H-Bridge velocidad (ENA)
const int FLOW_SENSOR_PIN = 4;   // YF-S401 pulsos
const int LEVEL1_PIN = 5;        // SE045 Tank 1 (ADC)
const int LEVEL2_PIN = 6;        // SE045 Tank 2 (ADC)
const int LED_STATUS_PIN = 7;    // LED indicador

// Parámetros PWM
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 10000;      // 10 kHz
const int PWM_RESOLUTION = 8;    // 8 bits (0-255)

void setupMotorPWM() {
    // Configurar pines motor
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    
    // Configurar canal LEDC
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, 0);  // Iniciar detenido
}

void setMotorSpeed(int dutyCycle) {
    // dutyCycle: 0-255
    if (dutyCycle < 0) dutyCycle = 0;
    if (dutyCycle > 255) dutyCycle = 255;
    
    ledcWrite(PWM_CHANNEL, dutyCycle);
}
```

### Manejo de Interrupciones para YF-S401

```cpp
// Pin de interrupción
const int FLOW_SENSOR_PIN = 4;
volatile unsigned long pulseCount = 0;

void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

void setupFlowSensor() {
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                    pulseCounter, RISING);
}
```

---

## Estructura del Repositorio

```
lab-control-informe-2/
│
├── README.md                          # Este archivo
├── informe_lab2.tex                   # Informe técnico IEEE (LaTeX)
├── base.tex                           # Plantilla base LaTeX
│
├── arduino/                           # Programas ESP32-S3
│   ├── 01_motor_control/
│   │   └── 01_motor_control.ino      # Sprint 1: Control motor H-Bridge
│   ├── 02_sensor_reading/
│   │   └── 02_sensor_reading.ino     # Sprint 2: Lectura sensores
│   ├── 03_sensor_actuator/
│   │   └── 03_sensor_actuator.ino   # Sprint 3: Integración
│   ├── 04_pid_controller/
│   │   └── 04_pid_controller.ino    # Sprint 4: PID básico
│   └── 05_complete_system/
│       └── 05_complete_system.ino   # Sprint 5: Sistema completo
│
├── matlab/                            # Scripts MATLAB
│   ├── identificacion_sistema.m       # Identificación experimental
│   ├── sintonia_ziegler_nichols.m     # Cálculo parámetros PID
│   ├── simulacion_lazo_cerrado.m      # Simulación control
│   └── analisis_datos.m               # Procesamiento datos
│
├── docs/                              # Documentación adicional
│   ├── datasheets/                    # Hojas de datos componentes
│   │   ├── ESP32-S3_datasheet.pdf
│   │   ├── YF-S401_specs.pdf
│   │   ├── SE045_manual.pdf
│   │   └── L298N_datasheet.pdf
│   ├── esquematicos/                  # Diagramas circuitos
│   │   ├── esquema_conexiones.png
│   │   ├── pcb_layout.png
│   │   └── diagrama_bloques.svg
│   └── calibracion/                   # Procedimientos calibración
│       ├── calibracion_YFS401.md
│       └── calibracion_SE045.md
│
├── data/                              # Datos experimentales
│   ├── raw/                           # Datos crudos (.csv)
│   ├── processed/                     # Datos procesados
│   └── plots/                         # Gráficas generadas
│
└── images/                            # Imágenes del sistema
    ├── sistema_montado.jpg
    ├── circuito_control.jpg
    └── resultados_graficos.png
```

---

## Guía de Inicio Rápido

### 1. Configuración Inicial

```bash
# Clonar repositorio (si aplica)
git clone [URL_REPOSITORIO]
cd lab-control-informe-2

# Instalar Arduino IDE (si no está instalado)
# Descargar desde: https://www.arduino.cc/en/software

# Agregar soporte ESP32
# File → Preferences → Additional Boards Manager URLs:
# https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

### 2. Cargar Primer Programa (Test Motor)

```bash
1. Abrir Arduino IDE
2. File → Open → arduino/01_motor_control/01_motor_control.ino
3. Tools → Board → ESP32S3 Dev Module
4. Tools → Port → [Seleccionar puerto COM del ESP32]
5. Sketch → Upload
6. Tools → Serial Monitor (115200 baud)
7. Enviar comandos: '0'-'9' para PWM 0-100%
```

### 3. Verificación de Hardware

**Checklist antes de alimentar:**
- [ ] ESP32-S3 conectado a USB
- [ ] H-Bridge conectado correctamente (IN1→GPIO15, IN2→GPIO16, ENA→GPIO17)
- [ ] Fuente 12V desconectada inicialmente
- [ ] Motor conectado a salidas H-Bridge (OUT1, OUT2)
- [ ] YF-S401: VCC→5V, GND→GND, Signal→GPIO4
- [ ] SE045 Tank1: Divisor resistivo (10kΩ/22kΩ) → GPIO5
- [ ] SE045 Tank2: Divisor resistivo (10kΩ/22kΩ) → GPIO6
- [ ] LED Status conectado a GPIO7 con resistencia 220Ω
- [ ] No hay cortocircuitos visibles
- [ ] Polaridades verificadas

**Test progresivo:**
1. ✅ Alimentar solo ESP32 con USB → LED debe encender
2. ✅ Cargar sketch 01_motor_control → Compilar sin errores
3. ✅ Conectar H-Bridge (sin motor aún) → Verificar voltajes lógica
4. ✅ Conectar motor → Test PWM bajo (30%)
5. ✅ Aumentar PWM gradualmente → Motor debe girar suavemente

---

## Precauciones de Seguridad ESP32-S3

### Protección del Microcontrolador

⚠️ **CRÍTICO - Evitar daño permanente al ESP32:**

1. **Voltajes de entrada ADC:**
   - Máximo absoluto: **3.3V** (3.6V destruye el ADC)
   - Los sensores SE045 generan hasta 4.5V → **USAR DIVISOR RESISTIVO**
   
   ```
   Sensor SE045 (0-4.5V) → [R1=10kΩ] → GPIO5/GPIO6 (ADC) ← [R2=22kΩ] → GND
   Voltaje ADC = 4.5V × (22kΩ/(10kΩ+22kΩ)) = 3.09V ✅ Seguro (<3.3V)
   
   Conexiones físicas:
   - SE045_Tank1 Signal → 10kΩ → GPIO5 → 22kΩ → GND
   - SE045_Tank2 Signal → 10kΩ → GPIO6 → 22kΩ → GND
   ```

2. **Corriente en pines GPIO:**
   - Máximo por pin: **40mA**
   - Total todos los pines: **200mA**
   - No conectar cargas directamente (usar transistores/MOSFETs)

3. **Alimentación:**
   - USB: 5V regulado a 3.3V internamente
   - VIN externo: 5-9V (regulador onboard)
   - **NO** conectar 12V directamente al ESP32

4. **Protección ESD:**
   - Tocar tierra antes de manipular ESP32
   - Evitar acumulación estática en ambientes secos

### Protección del Sistema Hidráulico

1. **Aislamiento agua-electrónica:**
   - Separación física mínima 30 cm
   - Sensores en área de salpicaduras → sellado IP65
   - Cables con recubrimiento resistente al agua

2. **Desbordamiento de tanques:**
   - Implementar software: detención bomba si nivel > umbral
   - Alarma visual/sonora en ESP32
   - Bandejas contención bajo tanques

3. **Prevención de marcha en seco:**
   ```cpp
   if (waterLevel_Tank1 < MIN_SAFE_LEVEL) {
       setMotorSpeed(0);  // Detener bomba
       Serial.println("ERROR: Nivel bajo en reservorio");
   }
   ```

---

## Resultados Esperados

### Métricas de Desempeño

**Sistema en Lazo Cerrado con PID:**

| Métrica | Especificación | Resultado Esperado |
|---------|---------------|-------------------|
| Error estado estable | < 2% referencia | ≤ 0.01 L/min |
| Tiempo establecimiento | < 20 segundos | 12-18 s |
| Sobreimpulso máximo | < 15% | 5-12% |
| Tiempo subida (10-90%) | < 10 segundos | 4-8 s |
| Frecuencia muestreo | > 10 Hz | 10-20 Hz |
| Resolución PWM | 8-10 bits | 8 bits (0-255) |

**Rangos de Operación:**

| Variable | Mínimo | Nominal | Máximo | Unidad |
|----------|--------|---------|--------|--------|
| Flujo entrada | 0.3 | 1.0-1.5 | 2.0 | L/min |
| Nivel Tanque 1 | 5 | 15 | 35 | mm |
| Nivel Tanque 2 | 5 | 15 | 35 | mm |
| PWM Motor | 0 | 128 | 255 | - |
| Voltaje Motor | 0 | 6 | 12 | V |

### Documentación Generada

1. **Informe Técnico IEEE:** Documento LaTeX compilado (15-25 páginas)
2. **Datos Experimentales:** Archivos CSV con timestamps
3. **Gráficas de Resultados:** PNG/PDF de análisis MATLAB
4. **Código Fuente:** 5 programas .ino documentados
5. **Video Demostración:** Sistema operando en lazo cerrado (opcional)

---

## Solución de Problemas Comunes

### Problemas de Compilación Arduino

**Error: "ESP32S3 Dev Module not found"**
```
Solución:
1. Tools → Board → Boards Manager
2. Buscar "esp32" → Verificar versión ≥ 3.0.0
3. Reinstalar si es necesario
```

**Error: "ledcSetup was not declared"**
```
Solución: Actualizar ESP32 core a versión 3.x
El API de LEDC cambió en v3.0
```

### Problemas de Hardware

**Motor no gira:**
- [ ] Verificar voltaje fuente 12V con multímetro
- [ ] Comprobar conexiones H-Bridge: IN1→GPIO15, IN2→GPIO16, ENA→GPIO17
- [ ] Verificar salidas H-Bridge con osciloscopio
- [ ] Verificar conexiones motor (no invertidas)
- [ ] Test manual: aplicar 12V directo al motor (bypass H-Bridge)
- [ ] Revisar señal PWM en GPIO17 con scope (debe ser 10kHz)
- [ ] Verificar que IN1=HIGH, IN2=LOW (o viceversa) para activar dirección

**Sensor YF-S401 no genera pulsos:**
- [ ] Verificar 5V alimentación con multímetro
- [ ] Girar rotor manualmente → debe generar pulsos
- [ ] Verificar pin señal con osciloscopio (debe ver pulsos)
- [ ] Flujo muy bajo < 0.3 L/min → sensor no detecta
- [ ] Limpiar impeller (puede estar bloqueado)

**Sensores SE045 dan lectura errónea:**
- [ ] Limpiar superficie del sensor (sedimentos)
- [ ] Verificar conexión a pin ADC correcto
- [ ] Usar divisor resistivo si voltaje > 3.3V
- [ ] Calibrar con alturas conocidas

**ESP32 se resetea continuamente:**
- [ ] Corriente motor excede capacidad fuente → usar fuente mayor amperaje
- [ ] Alimentar H-Bridge y ESP32 con fuentes separadas pero GND común
- [ ] Agregar capacitor 1000μF en alimentación motor

### Problemas de Control

**PID oscila excesivamente:**
```
Reducir Kd (acción derivativa amplifica ruido)
Reducir Kp en 30-50%
Agregar filtro pasabajos en medición
```

**Error estacionario grande:**
```
Aumentar Ki (acción integral)
Verificar saturación del integrador
Comprobar calibración sensores
```

**Respuesta muy lenta:**
```
Aumentar Kp (acción proporcional)
Reducir período muestreo (dt)
Verificar que bomba alcanza flujo máximo
```

---

## Mejoras y Expansiones Futuras

### Hardware

- **Display OLED:** Visualización local de variables (I2C)
- **Tarjeta microSD:** Almacenamiento datos a largo plazo
- **Sensor de temperatura:** Compensación temperatura en flujo
- **WiFi/Bluetooth:** Monitoreo remoto y control inalámbrico
- **PCB personalizado:** Reemplazo protoboard para mayor confiabilidad

### Software

- **Control adaptativo:** Ajuste automático parámetros PID
- **Control en cascada:** Lazo interno (flujo) + lazo externo (nivel)
- **Estimación estado:** Filtro Kalman para reducción ruido
- **Interfaz web:** Dashboard HTML con WebSockets ESP32
- **Machine Learning:** Predicción demanda y optimización

### Aplicaciones

- **Sistema multitanques:** Extender a 3-4 tanques interconectados
- **Control de pH:** Adición dosificación química automática
- **Optimización energética:** Minimizar consumo bomba
- **Simulación digital twin:** Modelo virtual sincronizado con físico
   - **Escalón:** posición constante del flujo
   - **Rampa:** velocidad constante de cambio de flujo
   - **Parábola:** aceleración constante de flujo

3. **Simulación:**
   - Validar diseño en MATLAB/Simulink
   - Verificar especificaciones de desempeño
   - Analizar estabilidad y robustez

### Fase 4: Implementación Analógica

1. **Diseño del circuito PID:**
   - Utilizar amplificadores operacionales
   - Implementar:
     - Acción proporcional con resistencias
     - Acción integral con capacitores
     - Acción derivativa con circuito RC
   - Diseñar etapa de suma para combinar las tres acciones

2. **Validación del circuito:**
   - Simular comportamiento del circuito
   - Verificar respuesta en frecuencia
   - Comprobar saturación de amplificadores

### Fase 5: Pruebas Experimentales

1. **Implementación en lazo cerrado:**
   - Conectar sensor → controlador → actuador
   - Aplicar referencia tipo escalón
   - Registrar señales:
     - Referencia
     - Error
     - Señal de control
     - Flujo medido

2. **Análisis comparativo:**
   - Comparar resultados simulación vs. experimentales
   - Evaluar métricas de desempeño:
     - Tiempo de establecimiento
     - Sobreimpulso
     - Error en estado estacionario
     - Tiempo de subida

---

## Modelado Matemático del Sistema Hidráulico

### Balance de Masa en el Tanque

Para un tanque con área transversal constante A:

```
dh/dt = (qin - qout) / A
```

Donde:
- h(t): altura del líquido en el tanque
- qin(t): flujo de entrada (controlado por bomba)
- qout(t): flujo de salida (por gravedad)

### Flujo de Salida

El flujo de salida por orificio depende de la altura:

```
qout(t) = k√h(t)
```

Donde k es una constante que depende del área del orificio y coeficiente de descarga.

### Dinámica No Lineal

La ecuación diferencial que describe el sistema es:

```
A(dh/dt) = qin(t) - k√h(t)
```

### Linealización

Alrededor de un punto de operación h̄:

```
A(dΔh/dt) = Δqin - (k/2√h̄)Δh
```

### Función de Transferencia Tanque

En el dominio de Laplace:

```
H(s)/Qin(s) = 1/(As + k/(2√h̄))
```

### Modelo Bomba-Motor

La relación entre voltaje DC y flujo generado:

```
Qin(s)/VDC(s) = Kb/(τs + 1)
```

Donde:
- Kb: constante de la bomba
- τ: constante de tiempo del motor

### Función de Transferencia Total

```
F(s) = Qin(s)/VDC(s) = [H(s)/Qin(s)] × [Qin(s)/VDC(s)]
```

---

## Sintonización por Ziegler-Nichols

### Método de Respuesta a Lazo Abierto

1. Aplicar entrada escalón al sistema
2. Registrar curva de respuesta (curva en S)
3. Obtener parámetros:
   - L: tiempo de retardo
   - T: constante de tiempo
4. Calcular parámetros PID según tabla de Ziegler-Nichols

### Parámetros del Controlador

Para seguimiento con error de estado estable cero:

| Controlador | Kp | Ti | Td |
|-------------|----|----|-----|
| P | T/L | - | - |
| PI | 0.9T/L | L/0.3 | - |
| PID | 1.2T/L | 2L | 0.5L |

---

## Implementación Analógica del PID

### Circuito Proporcional

```
Vp(t) = Kp × e(t)
```

Implementado con amplificador inversor:
- Kp = -Rf/Rin

### Circuito Integral

```
Vi(t) = (Kp/Ti) × ∫e(t)dt
```

Implementado con integrador operacional:
- Ti = Rin × Cin

### Circuito Derivativo

```
Vd(t) = Kp × Td × de(t)/dt
```

Implementado con derivador operacional:
- Td = Rf × Cf

### Sumador

Combina las tres acciones:
```
u(t) = Vp(t) + Vi(t) + Vd(t)
```

---

## Resultados Esperados

1. **Sistema hidráulico instrumentado** completamente funcional con sensores calibrados

2. **Modelo matemático validado** que describe con precisión la dinámica del sistema

3. **Controlador PID sintonizado** capaz de realizar seguimiento a referencias tipo:
   - Escalón (error estado estable = 0)
   - Rampa (error estado estable constante)
   - Parábola (comportamiento dinámico adecuado)

4. **Implementación analógica funcional** con circuito PID operando en lazo cerrado

5. **Informe técnico en formato IEEE** documentando:
   - Metodología experimental
   - Modelado matemático
   - Diseño del controlador
   - Resultados experimentales
   - Análisis comparativo simulación vs. experimental
   - Conclusiones y recomendaciones

---

## Competencias Desarrolladas

### Competencias Técnicas

- Aplicación de conocimientos de matemáticas, ciencias e ingeniería
- Diseño y conducción de experimentos
- Análisis e interpretación de datos experimentales
- Diseño de sistemas de control que cumplan especificaciones realistas
- Utilización de técnicas y herramientas modernas de ingeniería

### Competencias Profesionales

- Comunicación técnica efectiva
- Redacción de informes con formatos estandarizados
- Uso de lenguaje técnico preciso
- Presentación oral de ideas clara y concisa

### Indicadores de Evaluación

- Correcta expresión del modelo matemático
- Aplicación adecuada de métodos de solución
- Manejo de herramientas computacionales (MATLAB/Simulink)
- Identificación de parámetros y variables del sistema
- Formulación y ejecución del protocolo experimental
- Análisis crítico de resultados
- Establecimiento de requerimientos de ingeniería
- Evaluación de alternativas de solución
- Soporte técnico de la solución propuesta
- Implementación y validación exitosa

---

## Precauciones de Seguridad

### Generales

- Uso obligatorio de bata blanca en el laboratorio
- Uso adecuado de computadores y equipos electrónicos
- Apagar elementos antes de realizar cambios en circuitos

### Eléctricas

- No exceder valores máximos de voltaje y corriente
- Consultar datasheets de componentes
- No sobrepasar potencia disipada por resistencias
- Verificar polaridad en componentes polarizados

### Sistema Hidráulico

- Garantizar hermeticidad del sistema
- Proteger equipos electrónicos de derramamientos
- Mantener área de trabajo limpia y seca
- Verificar conexiones antes de encender bomba

---

## Restricciones Importantes

⚠️ **IMPORTANTE:** El diseño mecánico de la planta debe ser **ORIGINAL** para cada grupo de estudiantes. 

- No se permite compartir plantas entre grupos del semestre 2026-1
- Los ajustes durante el semestre deben quedar registrados
- El incumplimiento se considera falta al reglamento estudiantil
- Se informará a decanatura para proceso disciplinario

---

## Referencias Técnicas

### Software

- MATLAB R2020a o superior
- Simulink
- Control System Toolbox

### Normativas

- Formato IEEE para informes técnicos
- Normas de seguridad del laboratorio UMNG

### Bibliografía Recomendada

- Ogata, K. (2010). *Ingeniería de Control Moderna*. Pearson.
- Ziegler, J. G., & Nichols, N. B. (1942). *Optimum Settings for Automatic Controllers*. Trans. ASME, 64, 759-768.
- Franklin, G. F., Powell, J. D., & Emami-Naeini, A. (2015). *Feedback Control of Dynamic Systems*. Pearson.

---

## Autores del Laboratorio

**Elaborado por:**
- Lic. Andrés Castro, Ph.D. - Docente Tiempo Completo
- Ing. Adriana Riveros, M.Sc. - Docente Tiempo Completo
- Ing. Leonardo Solaque, Ph.D. - Docente Tiempo Completo
- Ing. Angélica Nivia, M.Sc. - Docente Cátedra

**Revisado por:**
- Ing. Olga Lucía Ramos, Ph.D. - Jefe de Área (Calle 100)
- Ing. Adriana Riveros, M.Sc. - Jefe de Área (Campus)

**Aprobado por:**
- Ing. Darío Amaya, Ph.D. - Director Programa Ingeniería en Mecatrónica (Calle 100)
- Ing. José Luis Caballero, M.Sc. - Director Programa Ingeniería en Mecatrónica (Campus)

---

## Control de Versiones

| Fecha | Descripción del Cambio | Justificación |
|-------|------------------------|---------------|
| 25/06/2024 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 10/12/2024 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 02/07/2025 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 10/12/2025 | Cambio de sistemas a trabajar | Renovación semestral de guías |

---

**Última actualización:** Febrero 28, 2026  
**Versión:** 3.0 - Implementación ESP32-S3  
**Estado:** En desarrollo - Semestre 2026-1

---

## Contacto y Soporte

### Estudiantes Responsables

**Daniel García Araque**
- Programa: Ingeniería Mecatrónica  
- Email: u3902276@unimilitar.edu.co  
- GitHub: [Agregar perfil]

**David Santiago García Suarez**
- Programa: Ingeniería Mecatrónica  
- Email: [Completar]  
- GitHub: [Agregar perfil]

### Docentes

**Laboratorio de Control Lineal**
- Universidad Militar Nueva Granada
- Facultad de Ingeniería
- Email: control.mecaronica@unimilitar.edu.co

---

## Licencia

Este proyecto es desarrollado con fines académicos para el curso de Control Lineal y Laboratorio de la Universidad Militar Nueva Granada.

**Uso Académico:**
- Permitido para estudiantes del curso con atribución apropiada
- Prohibida la copia literal sin comprensión del contenido
- Cada grupo debe desarrollar implementación original

**Restricciones:**
- El diseño físico del sistema debe ser único por grupo
- No se permite compartir código entre grupos del mismo semestre
- Las calibraciones y parámetros son específicos de cada implementación

---

## Agradecimientos

- **Docentes del área de Control:** Por la guía y supervisión técnica
- **Personal de laboratorio UMNG:** Por el soporte en equipos e instrumentación
- **Comunidad Arduino/ESP32:** Por documentación y librerías open source
- **Espressif Systems:** Por el soporte técnico del ESP32-S3

---

## Referencias y Recursos

### Documentación Técnica ESP32

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [Arduino-ESP32 Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [LEDC PWM Controller Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/ledc.html)

### Tutoriales y Ejemplos

- [YF-S401 Flow Sensor Arduino Tutorial](https://www.instructables.com/YF-S401-Water-Flow-Sensor-Arduino/)
- [PID Library for Arduino](https://github.com/br3ttb/Arduino-PID-Library/)
- [ESP32 Interrupt Tutorial](https://randomnerdtutorials.com/esp32-pir-motion-sensor-interrupts-timers/)
- [ADC Calibration ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_calibration.html)

### Control Theory Resources

- MIT OpenCourseWare: [Feedback Control Systems](https://ocw.mit.edu/courses/mechanical-engineering/)
- Brian Douglas YouTube: [Control Systems Lectures](https://www.youtube.com/user/ControlLectures)
- MATLAB Control Tutorials: [University of Michigan](http://ctms.engin.umich.edu/CTMS/index.php)

### Community Forums

- [Arduino Forum - ESP32](https://forum.arduino.cc/c/hardware/esp32/85)
- [ESP32.com Developer Forum](https://www.esp32.com/)
- [Electrical Engineering Stack Exchange](https://electronics.stackexchange.com/)

---

## Anexos

### A. Tabla de Conversión Rápida

**PWM a Voltaje Motor (8-bit, 12V nominal):**

| PWM | % | Voltaje | Flujo Aprox. |
|-----|---|---------|--------------|
| 0 | 0% | 0V | 0 L/min |
| 64 | 25% | 3V | 0.4 L/min |
| 128 | 50% | 6V | 0.9 L/min |
| 192 | 75% | 9V | 1.4 L/min |
| 255 | 100% | 12V | 2.0 L/min |

**Nivel de Agua a ADC (SE045 con divisor 10k/22k):**

| Nivel (mm) | Voltaje Sensor | Voltaje ADC | Valor ADC (12-bit) |
|------------|---------------|-------------|-------------------|
| 0 | 0V | 0V | 0 |
| 10 | 1.125V | 0.77V | ~960 |
| 20 | 2.25V | 1.54V | ~1920 |
| 30 | 3.375V | 2.32V | ~2880 |
| 40 | 4.5V | 3.09V | ~3840 |

### B. Comandos Serial Monitor Útiles

```
Formato: comando,valor

Ejemplos:
SETPWM,128      → Establecer PWM a 128 (50%)
SETSP,1.5       → Establecer setpoint a 1.5 L/min
SETKP,10.5      → Establecer Kp = 10.5
SETKI,2.3       → Establecer Ki = 2.3
SETKD,1.8       → Establecer Kd = 1.8
STARTP          → Iniciar control PID
STOPP           → Detener control PID
RESET           → Reiniciar ESP32
CALIB           → Modo calibración sensores
STATUS          → Mostrar estado del sistema
DATA            → Streaming continuo de datos
HELP            → Mostrar comandos disponibles
```

### C. Checklist de Entregables

#### Avance Parcial (Semana 4):
- [ ] Sistema hidráulico montado y probado
- [ ] Sensores instalados y calibrados
- [ ] Programas Sprint 1-3 funcionando
- [ ] Datos experimentales lazo abierto
- [ ] Identificación preliminar del sistema

#### Entrega Final (Semana 8):
- [ ] Sistema completo operando en lazo cerrado
- [ ] Todos los programas (.ino) documentados
- [ ] Informe técnico IEEE completo (LaTeX)
- [ ] Video demostración (3-5 minutos)
- [ ] Datos experimentales procesados
- [ ] Análisis MATLAB con gráficas
- [ ] Presentación PowerPoint (15 diapositivas)
- [ ] Código fuente en repositorio Git

---

## Changelog

### [3.0] - 2026-02-28
#### Agregado
- Documentación completa ESP32-S3
- Especificaciones hardware YF-S401 y SE045
- Guía de implementación Arduino IDE
- Sección de solución de problemas
- Ejemplos de código completos
- Tabla de conversiones rápidas

#### Modificado
- Migración de control analógico a digital
- Actualización diagrama de bloques del sistema
- Mejora en sección de seguridad

### [2.4] - 2025-12-10
#### Modificado
- Cambio de sistemas a trabajar por renovación semestral

### [2.0] - 2024-06-25
#### Inicial
- Estructura base del laboratorio
- Objetivos y metodología
- Modelo matemático del sistema

---

<div align="center">

**Universidad Militar Nueva Granada**  
*Facultad de Ingeniería - Ingeniería Mecatrónica*  
*Control Lineal y Laboratorio*  
*2026-1*

[![ESP32](https://img.shields.io/badge/ESP32-S3-blue?style=flat&logo=espressif)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?style=flat&logo=arduino)](https://www.arduino.cc/)
[![MATLAB](https://img.shields.io/badge/MATLAB-R2020a+-orange?style=flat&logo=mathworks)](https://www.mathworks.com/products/matlab.html)

</div>
