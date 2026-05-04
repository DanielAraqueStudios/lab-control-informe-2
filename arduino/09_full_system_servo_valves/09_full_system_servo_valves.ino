#include <ESP32Servo.h>

/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE FLUJO HIDRÁULICO
 * Universidad Militar Nueva Granada
 * Ingeniería Mecatrónica - Control Lineal y Laboratorio
 * ============================================================================
 * 
 * SPRINT 9: SISTEMA COMPLETO + VALVULAS SERVO
 * 
 * Descripción:
 *   Sistema de control integral con control PID de dos tanques, generación
 *   de referencias múltiples (escalón, rampa, parábola), rechazo de
 *   perturbaciones, data logging avanzado y análisis de desempeño.
 * 
 * Características:
 *   - Control PID dual (Tanque 1 y Tanque 2)
 *   - Generador de referencias: Step, Ramp, Parabolic
 *   - Análisis de métricas: overshoot, settling time, steady-state error
 *   - Logging CSV para MATLAB/Python
 *   - Modo manual y automático
 *   - Control cascada opcional
 * 
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - H-Bridge L298N + Motor 12V
 *   - Sensor de flujo YF-S401
 *   - 2x Sensores de nivel HC-SR04
 *   - 2x Valvulas servo
 * 
 * Conexiones:
 *   GPIO 17 → H-Bridge ENA (PWM)
 *   GPIO 15 → H-Bridge IN1
 *   GPIO 16 → H-Bridge IN2
 *   GPIO 4  → YF-S401 Signal
 *   GPIO 5  → HC-SR04 Tank1 TRIG
 *   GPIO 6  → HC-SR04 Tank1 ECHO
 *   GPIO 8  → HC-SR04 Tank2 TRIG
 *   GPIO 9  → HC-SR04 Tank2 ECHO
 *   GPIO 7  → LED Status
 *   GPIO 18 → Servo valve Tank1
 *   GPIO 19 → Servo valve Tank2
 * 
 * Comandos UART (115200 baud):
 *   SETMODE,<mode>       - Mode: MANUAL, AUTO_FLOW, AUTO_LEVEL1, AUTO_LEVEL2, CASCADE
 *   SETREF,<type>,<params> - Tipo: STEP, RAMP, PARA
 *   SETPID1,<Kp>,<Ki>,<Kd> - PID Tanque 1
 *   SETPID2,<Kp>,<Ki>,<Kd> - PID Tanque 2
 *   STARTCTRL            - Iniciar control automático
 *   STOPCTRL             - Detener control
 *   DISTURBANCE          - Simular perturbación
 *   METRICS              - Calcular métricas de desempeño
 *   DATALOG              - Logging CSV completo
 *   EXPERIMENT,<name>    - Ejecutar experimento predefinido
 *   STATUS               - Estado del sistema
 *   HELP                 - Comandos disponibles
 * 
 * Autor: Daniel García Araque, David Santiago García Suarez
 * Fecha: Febrero 2026
 * Versión: 2.0
 * ============================================================================
 */

// ============================================================================
// DEFINICIÓN DE PINES
// ============================================================================

#define MOTOR_PWM_PIN    17
#define MOTOR_IN1_PIN    15
#define MOTOR_IN2_PIN    16
#define FLOW_SENSOR_PIN  4
#define TRIG_PIN_1       5
#define ECHO_PIN_1       6
#define TRIG_PIN_2       8
#define ECHO_PIN_2       9
#define LED_STATUS_PIN   7
#define SERVO_TANK1_PIN  18
#define SERVO_TANK2_PIN  19

// ============================================================================
// CONFIGURACIÓN PWM, ADC Y ULTRASÓNICO
// ============================================================================

#define PWM_FREQUENCY    10000
#define PWM_RESOLUTION   8
#define PWM_MIN          0
#define PWM_MAX          255

// Calibracion mecanica de valvulas servo
const int VALVE_FULL_OPEN_ANGLE = 170;
const int VALVE_CLOSED_ANGLE = 180;
const float VALVE_DEADBAND_MM = 3.0;
const float VALVE_FULL_OPEN_ERROR_MM = 50.0;
const unsigned long VALVE_CONTROL_INTERVAL_MS = 200;

// Dimensiones de los tanques (en milímetros)
const float TANK1_HEIGHT_MM = 150.0;  // 15 cm
const float TANK2_HEIGHT_MM = 160.0;  // 16 cm

// Límites de seguridad (Banda muerta para proteger los sensores)
const float MAX_LEVEL_ALLOW = 110.0;  // 11 cm máximo nivel de agua
const float MIN_LEVEL_ALLOW = 70.0;   // 7 cm límite inferior a considerar como vacío

#define ADC_RESOLUTION   4095
#define ADC_VREF         3.3
#define ADC_SAMPLES      10

// Parámetros antiguos (obsoletos ahora, pero se conservan para compilación y evitar dependencias rotas en memoria)
const float VOLTAGE_DIVIDER_FACTOR = 0.6875;
const float SENSOR_MAX_VOLTAGE = 4.5;
const float SENSOR_MAX_HEIGHT = 40.0;
float flowCalibrationFactor = 98.0;

// ============================================================================
// MODOS DE OPERACIÓN
// ============================================================================

enum ControlMode {
  MODE_MANUAL,
  MODE_AUTO_FLOW,
  MODE_AUTO_LEVEL1,
  MODE_AUTO_LEVEL2,
  MODE_CASCADE
};

ControlMode currentMode = MODE_MANUAL;
String modeNames[] = {"MANUAL", "AUTO_FLOW", "AUTO_LEVEL1", "AUTO_LEVEL2", "CASCADE"};

// ============================================================================
// GENERADOR DE REFERENCIAS
// ============================================================================

enum ReferenceType {
  REF_CONSTANT,
  REF_STEP,
  REF_RAMP,
  REF_PARABOLIC,
  REF_SINE
};

ReferenceType refType = REF_CONSTANT;
float refValue = 0.0;
float refInitial = 0.0;
float refFinal = 1.0;
float refDuration = 10.0;
float refAmplitude = 0.5;
float refFrequency = 0.1;

unsigned long refStartTime = 0;
bool refActive = false;

// ============================================================================
// CONTROLADORES PID
// ============================================================================

// PID para control de flujo
struct PIDController {
  float Kp;
  float Ki;
  float Kd;
  float setpoint;
  float processValue;
  float error;
  float lastError;
  float integral;
  float derivative;
  float output;
  bool enabled;
};

PIDController pidFlow = {50.0, 10.0, 5.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false};
PIDController pidLevel1 = {30.0, 5.0, 3.0, 20.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false};
PIDController pidLevel2 = {30.0, 5.0, 3.0, 20.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false};

float dt = 0.1;  // 100ms control loop
unsigned long lastControlTime = 0;

const float INTEGRAL_MAX = 200.0;
const float INTEGRAL_MIN = -200.0;
const float OUTPUT_MIN = 0.0;
const float OUTPUT_MAX = 255.0;

// ============================================================================
// VARIABLES SENSORES
// ============================================================================

volatile unsigned long flowPulseCount = 0;
float flowRate = 0.0;
float totalVolume = 0.0;
float waterLevel1 = 0.0;
float waterLevel2 = 0.0;

unsigned long lastFlowUpdate = 0;
const unsigned long flowUpdateInterval = 1000;

const int FILTER_SIZE = 5;
float level1_buffer[FILTER_SIZE] = {0};
float level2_buffer[FILTER_SIZE] = {0};
int level1_filter_index = 0;
int level2_filter_index = 0;

// ============================================================================
// VALVULAS SERVO PARA CONTROL DE ALTURA
// ============================================================================

Servo tank1ValveServo;
Servo tank2ValveServo;

float targetLevel1 = 90.0;
float targetLevel2 = 90.0;
int tank1ValveAngle = VALVE_CLOSED_ANGLE;
int tank2ValveAngle = VALVE_CLOSED_ANGLE;
bool levelValveControlEnabled = false;
unsigned long lastValveControlTime = 0;

// ============================================================================
// CALIBRACIÓN DE SENSORES
// ============================================================================

bool calModeActive = false;
unsigned long lastCalLogTime = 0;
const unsigned long CAL_LOG_INTERVAL = 300;  // ms entre lecturas de calibración

// Calibración Tanque 1 (ADC raw)
float cal1_empty_adc = 0.0;      // ADC cuando tanque vacío
float cal1_full_adc  = 4095.0;   // ADC cuando tanque lleno
bool  cal1_calibrated = false;

// Calibración Tanque 2 (ADC raw)
float cal2_empty_adc = 0.0;
float cal2_full_adc  = 4095.0;
bool  cal2_calibrated = false;

// Lee ADC bruto (sin conversión) para calibración
float readRawADC(int pin) {
  long adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(pin);
    delayMicroseconds(100);
  }
  return adcSum / (float)ADC_SAMPLES;
}

// Convierte ADC bruto a mm usando puntos de calibración
float adcToHeight(float raw, float emptyADC, float fullADC, float maxHeight) {
  if (abs(fullADC - emptyADC) < 10.0) return 0.0;  // Sin calibración válida
  float ratio = (raw - emptyADC) / (fullADC - emptyADC);
  ratio = constrain(ratio, 0.0, 1.0);
  return ratio * maxHeight;
}

// ============================================================================
// VARIABLES VARIABLES MOTOR
// ============================================================================

int currentPWM = 0;
bool motorDirection = 0;
bool motorRunning = false;

// ============================================================================
// MÉTRICAS DE DESEMPEÑO
// ============================================================================

struct PerformanceMetrics {
  float maxOvershoot;
  float settlingTime;
  float riseTime;
  float steadyStateError;
  float peakValue;
  unsigned long testStartTime;
  float initialValue;
  bool measuring;
};

PerformanceMetrics metrics = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0.0, false};

// ============================================================================
// LOGGING Y ANÁLISIS
// ============================================================================

bool loggingEnabled = true;
unsigned long lastLogTime = 0;
unsigned long logInterval = 100;
int logCounter = 0;
String experimentName = "test";

// ============================================================================
// COMUNICACIÓN
// ============================================================================

String inputCommand = "";
bool commandComplete = false;

// ============================================================================
// ISR
// ============================================================================

void IRAM_ATTR flowPulseCounter() {
  flowPulseCount++;
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n");
  Serial.println("============================================================");
  Serial.println("  LABORATORIO 2 - SISTEMA DE CONTROL COMPLETO");
  Serial.println("  Universidad Militar Nueva Granada");
  Serial.println("  ESP32-S3 - Sprint 9: Full System + Servo Valves");
  Serial.println("============================================================");
  Serial.println();
  
  // Configurar motor
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);
  
  ledcAttach(MOTOR_PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcWrite(MOTOR_PWM_PIN, 0);

  // Configurar valvulas servo
  tank1ValveServo.attach(SERVO_TANK1_PIN);
  tank2ValveServo.attach(SERVO_TANK2_PIN);
  closeAllValves();
  
  // Configurar sensores
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
  analogReadResolution(12);

  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  digitalWrite(TRIG_PIN_1, LOW);
  
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);
  digitalWrite(TRIG_PIN_2, LOW);
  
  // Dirección adelante
  setMotorDirection(0);

  primeLevelFilters();
  printInitialHeights();

  Serial.println("[OK] Hardware inicializado");
  Serial.println("[MODE] " + modeNames[currentMode]);
  Serial.println("[READY] Sistema listo - Use HELP para comandos");
  Serial.println();
  
  printHelp();
  
  for(int i=0; i<3; i++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(150);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(150);
  }
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  processSerialCommands();
  
  // Actualizar flujo
  if (millis() - lastFlowUpdate >= flowUpdateInterval) {
    updateFlowRate();
    readAllSensors();
    lastFlowUpdate = millis();
  }
  
  // Loop de control
  if (millis() - lastControlTime >= dt * 1000) {
    updateReference();
    executeControl();
    updateMetrics();
    lastControlTime = millis();
  }

  if (levelValveControlEnabled &&
      (millis() - lastValveControlTime >= VALVE_CONTROL_INTERVAL_MS)) {
    updateValveHeightControl();
    lastValveControlTime = millis();
  }
  
  // Logging
  if (loggingEnabled && (millis() - lastLogTime >= logInterval)) {
    logCompleteData();
    lastLogTime = millis();
  }

  // Calibration streaming (ahora manda alturas en mm igual)
  if (calModeActive && (millis() - lastCalLogTime >= CAL_LOG_INTERVAL)) {
    float r1 = readWaterLevel1();
    float r2 = readWaterLevel2();
    Serial.print("[CAL] T1:");
    Serial.print(r1, 1);
    Serial.print(" T2:");
    Serial.print(r2, 1);
    Serial.print(" PWM:");
    Serial.println(currentPWM);
    lastCalLogTime = millis();
  }
  
  // LED indica modo
  updateStatusLED();
  
  delay(5);
}

// ============================================================================
// GENERADOR DE REFERENCIAS
// ============================================================================

float generateReference() {
  if (!refActive) return refValue;
  
  float elapsed = (millis() - refStartTime) / 1000.0;
  
  switch(refType) {
    case REF_CONSTANT:
      return refValue;
      
    case REF_STEP:
      if (elapsed >= refDuration) return refFinal;
      return refInitial;
      
    case REF_RAMP:
      if (elapsed >= refDuration) return refFinal;
      return refInitial + (refFinal - refInitial) * (elapsed / refDuration);
      
    case REF_PARABOLIC: {
      if (elapsed >= refDuration) return refFinal;
      float t_norm = elapsed / refDuration;
      return refInitial + (refFinal - refInitial) * (3*t_norm*t_norm - 2*t_norm*t_norm*t_norm);
    }
    case REF_SINE:
      return refValue + refAmplitude * sin(2 * PI * refFrequency * elapsed);
      
    default:
      return refValue;
  }
}

void updateReference() {
  float newRef = generateReference();
  
  switch(currentMode) {
    case MODE_AUTO_FLOW:
      pidFlow.setpoint = newRef;
      break;
    case MODE_AUTO_LEVEL1:
      pidLevel1.setpoint = newRef;
      break;
    case MODE_AUTO_LEVEL2:
      pidLevel2.setpoint = newRef;
      break;
    case MODE_CASCADE:
      pidLevel1.setpoint = newRef;
      break;
  }
}

// ============================================================================
// CONTROL
// ============================================================================

void executeControl() {
  switch(currentMode) {
    case MODE_MANUAL:
      // PWM manual ya establecido
      break;
      
    case MODE_AUTO_FLOW:
      pidFlow.processValue = flowRate;
      computePIDSingle(&pidFlow);
      setMotorPWM((int)pidFlow.output);
      break;
      
    case MODE_AUTO_LEVEL1:
      pidLevel1.processValue = waterLevel1;
      computePIDSingle(&pidLevel1);
      setMotorPWM((int)pidLevel1.output);
      break;
      
    case MODE_AUTO_LEVEL2:
      pidLevel2.processValue = waterLevel2;
      computePIDSingle(&pidLevel2);
      setMotorPWM((int)pidLevel2.output);
      break;
      
    case MODE_CASCADE:
      // Control cascada: Nivel1 → Flujo → Motor
      pidLevel1.processValue = waterLevel1;
      computePIDSingle(&pidLevel1);
      
      pidFlow.setpoint = pidLevel1.output / 50.0;  // Escalar output nivel a setpoint flujo
      pidFlow.processValue = flowRate;
      computePIDSingle(&pidFlow);
      
      setMotorPWM((int)pidFlow.output);
      break;
  }
}

void computePIDSingle(PIDController* pid) {
  if (!pid->enabled) return;
  
  // Error
  pid->error = pid->setpoint - pid->processValue;
  
  // Proporcional
  float P = pid->Kp * pid->error;
  
  // Integral con anti-windup
  pid->integral += pid->error * dt;
  if (pid->integral > INTEGRAL_MAX) pid->integral = INTEGRAL_MAX;
  if (pid->integral < INTEGRAL_MIN) pid->integral = INTEGRAL_MIN;
  float I = pid->Ki * pid->integral;
  
  // Derivativo
  pid->derivative = (pid->error - pid->lastError) / dt;
  float D = pid->Kd * pid->derivative;
  
  // Output total
  pid->output = P + I + D;
  
  // Saturación
  if (pid->output > OUTPUT_MAX) pid->output = OUTPUT_MAX;
  if (pid->output < OUTPUT_MIN) pid->output = OUTPUT_MIN;
  
  pid->lastError = pid->error;
}

void resetPIDController(PIDController* pid) {
  pid->integral = 0;
  pid->lastError = 0;
  pid->derivative = 0;
  pid->output = 0;
  pid->error = 0;
}

// ============================================================================
// MÉTRICAS DE DESEMPEÑO
// ============================================================================

void startMetrics() {
  metrics.testStartTime = millis();
  metrics.initialValue = (currentMode == MODE_AUTO_FLOW) ? flowRate : waterLevel1;
  metrics.maxOvershoot = 0;
  metrics.peakValue = metrics.initialValue;
  metrics.measuring = true;
  Serial.println("[METRICS] Medición iniciada");
}

void updateMetrics() {
  if (!metrics.measuring) return;
  
  float currentValue = (currentMode == MODE_AUTO_FLOW) ? flowRate : waterLevel1;
  float setpoint = (currentMode == MODE_AUTO_FLOW) ? pidFlow.setpoint : pidLevel1.setpoint;
  
  // Peak value
  if (currentValue > metrics.peakValue) {
    metrics.peakValue = currentValue;
  }
  
  // Overshoot
  float overshoot = ((metrics.peakValue - setpoint) / setpoint) * 100.0;
  if (overshoot > metrics.maxOvershoot) {
    metrics.maxOvershoot = overshoot;
  }
  
  // Settling time (criterio 2%)
  float error_pct = abs((currentValue - setpoint) / setpoint) * 100.0;
  if (error_pct < 2.0 && metrics.settlingTime == 0) {
    metrics.settlingTime = (millis() - metrics.testStartTime) / 1000.0;
  }
  
  // Rise time (10% a 90%)
  float range = setpoint - metrics.initialValue;
  float current_pct = ((currentValue - metrics.initialValue) / range) * 100.0;
  if (current_pct >= 90.0 && metrics.riseTime == 0) {
    metrics.riseTime = (millis() - metrics.testStartTime) / 1000.0;
  }
}

void printMetrics() {
  float setpoint = (currentMode == MODE_AUTO_FLOW) ? pidFlow.setpoint : pidLevel1.setpoint;
  float finalValue = (currentMode == MODE_AUTO_FLOW) ? flowRate : waterLevel1;
  
  metrics.steadyStateError = abs((finalValue - setpoint) / setpoint) * 100.0;
  
  Serial.println("\n╔════════════════════════════════════════════════════╗");
  Serial.println("║          MÉTRICAS DE DESEMPEÑO                     ║");
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.print("║  Overshoot máximo:      ");
  Serial.print(metrics.maxOvershoot, 2);
  Serial.println("%");
  Serial.print("║  Rise Time:             ");
  Serial.print(metrics.riseTime, 2);
  Serial.println(" s");
  Serial.print("║  Settling Time (2%):    ");
  Serial.print(metrics.settlingTime, 2);
  Serial.println(" s");
  Serial.print("║  Error estado estable:  ");
  Serial.print(metrics.steadyStateError, 2);
  Serial.println("%");
  Serial.print("║  Valor pico:            ");
  Serial.println(metrics.peakValue, 3);
  Serial.println("╚════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// FUNCIONES MOTOR
// ============================================================================

void setMotorPWM(int pwmValue) {
  // ==========================================
  // BLOQUEO DE EMERGENCIA (HARDWARE PANIC)
  // Sobrescribe cualquier intento, automático o manual
  // ==========================================
  if (pwmValue > 0 && (waterLevel1 >= MAX_LEVEL_ALLOW || waterLevel2 >= MAX_LEVEL_ALLOW)) {
    pwmValue = 0;
    closeAllValves();
    // Enviaremos el mensaje de error por serial solo cuando intenten pedir velocidad
    Serial.println("[ERROR] HARDWARE PANIC: Nivel excede " + String(MAX_LEVEL_ALLOW) + "mm. BOMBA BLOQUEADA.");
  }
  
  if (pwmValue < PWM_MIN) pwmValue = PWM_MIN;
  if (pwmValue > PWM_MAX) pwmValue = PWM_MAX;
  
  ledcWrite(MOTOR_PWM_PIN, pwmValue);
  currentPWM = pwmValue;
  motorRunning = (pwmValue > 0);
}

void setMotorDirection(bool direction) {
  motorDirection = direction;
  if (direction == 0) {
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
  } else {
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);
  }
}

void stopMotor() {
  setMotorPWM(0);
  motorRunning = false;
}

// ============================================================================
// FUNCIONES SENSORES
// ============================================================================

void updateFlowRate() {
  detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
  
  float frequency = flowPulseCount / (flowUpdateInterval / 1000.0);
  flowRate = frequency / (flowCalibrationFactor / 60.0);
  totalVolume += (flowRate / 60.0) * (flowUpdateInterval / 1000.0);
  
  flowPulseCount = 0;
  
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
}

float measureDistanceMm(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return TANK1_HEIGHT_MM; // Si falla, que simule vacío
  return (duration * 0.343) / 2.0;
}

float readWaterLevel1() {
  float distancia_mm = measureDistanceMm(TRIG_PIN_1, ECHO_PIN_1);
  float height = TANK1_HEIGHT_MM - distancia_mm;
  
  if (height < 0) height = 0; // Prevenir negativos

  // Margen de seguridad: Forzar apagado de bomba si se supera el nivel permitido
  if (height >= MAX_LEVEL_ALLOW) {
    if (currentPWM > 0) {
      Serial.println("[ERROR] T1 Nivel critico " + String(height) + "mm. BOMBA APAGADA.");
      setMotorPWM(0); // Ahora setMotorPWM está fuertemente bloqueado
    }
  }

  level1_buffer[level1_filter_index] = height;
  level1_filter_index = (level1_filter_index + 1) % FILTER_SIZE;
  float filtered = 0;
  for (int i = 0; i < FILTER_SIZE; i++) filtered += level1_buffer[i];
  return filtered / FILTER_SIZE;
}

float readWaterLevel2() {
  float distancia_mm = measureDistanceMm(TRIG_PIN_2, ECHO_PIN_2);
  float height = TANK2_HEIGHT_MM - distancia_mm;
  
  if (height < 0) height = 0;

  // Margen de seguridad para Tanque 2
  if (height >= MAX_LEVEL_ALLOW) {
    if (currentPWM > 0) {
      Serial.println("[ERROR] T2 Nivel critico " + String(height) + "mm. BOMBA APAGADA.");
      setMotorPWM(0); // Bloqueo blindado en la función maestra
    }
  }

  level2_buffer[level2_filter_index] = height;
  level2_filter_index = (level2_filter_index + 1) % FILTER_SIZE;
  float filtered = 0;
  for (int i = 0; i < FILTER_SIZE; i++) filtered += level2_buffer[i];
  return filtered / FILTER_SIZE;
}

void readAllSensors() {
  waterLevel1 = readWaterLevel1();
  waterLevel2 = readWaterLevel2();
}

void primeLevelFilters() {
  for (int i = 0; i < FILTER_SIZE; i++) {
    readAllSensors();
    delay(50);
  }
}

// ============================================================================
// CONTROL DE VALVULAS SERVO POR ALTURA
// ============================================================================

int normalizeValveAngle(int angle) {
  return constrain(angle, VALVE_FULL_OPEN_ANGLE, VALVE_CLOSED_ANGLE);
}

int normalizeServoAngle(int angle) {
  return constrain(angle, 0, 180);
}

int calculateValveAngle(float targetLevel, float actualLevel) {
  float error = targetLevel - actualLevel;

  if (error <= VALVE_DEADBAND_MM) {
    return VALVE_CLOSED_ANGLE;
  }

  float ratio = constrain(error / VALVE_FULL_OPEN_ERROR_MM, 0.0, 1.0);
  int openingDegrees = round(ratio * (VALVE_CLOSED_ANGLE - VALVE_FULL_OPEN_ANGLE));
  return normalizeValveAngle(VALVE_CLOSED_ANGLE - openingDegrees);
}

void setValveAngle(int tank, int angle) {
  angle = normalizeServoAngle(angle);

  if (tank == 1) {
    bool changed = (tank1ValveAngle != angle);
    tank1ValveAngle = angle;
    tank1ValveServo.write(tank1ValveAngle);
    if (changed) {
      Serial.print("[VALVE] T1 angle=");
      Serial.println(tank1ValveAngle);
    }
  } else if (tank == 2) {
    bool changed = (tank2ValveAngle != angle);
    tank2ValveAngle = angle;
    tank2ValveServo.write(tank2ValveAngle);
    if (changed) {
      Serial.print("[VALVE] T2 angle=");
      Serial.println(tank2ValveAngle);
    }
  } else {
    Serial.println("[ERROR] Valve tank must be 1 or 2");
  }
}

void openValve(int tank) {
  setValveAngle(tank, VALVE_FULL_OPEN_ANGLE);
}

void closeValve(int tank) {
  setValveAngle(tank, VALVE_CLOSED_ANGLE);
}

void openAllValves() {
  setValveAngle(1, VALVE_FULL_OPEN_ANGLE);
  setValveAngle(2, VALVE_FULL_OPEN_ANGLE);
}

void closeAllValves() {
  setValveAngle(1, VALVE_CLOSED_ANGLE);
  setValveAngle(2, VALVE_CLOSED_ANGLE);
}

void updateValveHeightControl() {
  if (waterLevel1 >= MAX_LEVEL_ALLOW || waterLevel2 >= MAX_LEVEL_ALLOW) {
    levelValveControlEnabled = false;
    stopMotor();
    closeAllValves();
    Serial.println("[ERROR] Level safety limit reached. Valve control OFF, pump OFF, valves CLOSED.");
    return;
  }

  int valve1 = calculateValveAngle(targetLevel1, waterLevel1);
  int valve2 = calculateValveAngle(targetLevel2, waterLevel2);

  setValveAngle(1, valve1);
  setValveAngle(2, valve2);
}

void printInitialHeights() {
  Serial.println("[HEIGHT] Initial ultrasonic readings:");
  Serial.print("[HEIGHT] Tank1=");
  Serial.print(waterLevel1, 1);
  Serial.print(" mm target=");
  Serial.print(targetLevel1, 1);
  Serial.println(" mm");
  Serial.print("[HEIGHT] Tank2=");
  Serial.print(waterLevel2, 1);
  Serial.print(" mm target=");
  Serial.print(targetLevel2, 1);
  Serial.println(" mm");
}

void printValveStatus() {
  Serial.println();
  Serial.println("========== VALVE HEIGHT CONTROL ==========");
  Serial.print("Control: ");
  Serial.println(levelValveControlEnabled ? "ON" : "OFF");
  Serial.print("T1 actual/target: ");
  Serial.print(waterLevel1, 1);
  Serial.print(" / ");
  Serial.print(targetLevel1, 1);
  Serial.print(" mm | valve=");
  Serial.println(tank1ValveAngle);
  Serial.print("T2 actual/target: ");
  Serial.print(waterLevel2, 1);
  Serial.print(" / ");
  Serial.print(targetLevel2, 1);
  Serial.print(" mm | valve=");
  Serial.println(tank2ValveAngle);
  Serial.print("Open angle: ");
  Serial.print(VALVE_FULL_OPEN_ANGLE);
  Serial.print(" | Closed angle: ");
  Serial.println(VALVE_CLOSED_ANGLE);
  Serial.println("==========================================");
  Serial.println();
}

// ============================================================================
// LOGGING
// ============================================================================

void logCompleteData() {
  if (logCounter % 30 == 0) {
    Serial.println("\nTime_s,Mode,RefType,Ref,Flow,Level1,Level2,PWM,PID_P,PID_I,PID_D,Error,Volume");
  }
  
  float currentSetpoint = 0;
  float currentError = 0;
  float P=0, I=0, D=0;
  
  switch(currentMode) {
    case MODE_AUTO_FLOW:
      currentSetpoint = pidFlow.setpoint;
      currentError = pidFlow.error;
      P = pidFlow.Kp * pidFlow.error;
      I = pidFlow.Ki * pidFlow.integral;
      D = pidFlow.Kd * pidFlow.derivative;
      break;
    case MODE_AUTO_LEVEL1:
      currentSetpoint = pidLevel1.setpoint;
      currentError = pidLevel1.error;
      P = pidLevel1.Kp * pidLevel1.error;
      I = pidLevel1.Ki * pidLevel1.integral;
      D = pidLevel1.Kd * pidLevel1.derivative;
      break;
    case MODE_AUTO_LEVEL2:
      currentSetpoint = pidLevel2.setpoint;
      currentError = pidLevel2.error;
      P = pidLevel2.Kp * pidLevel2.error;
      I = pidLevel2.Ki * pidLevel2.integral;
      D = pidLevel2.Kd * pidLevel2.derivative;
      break;
  }
  
  Serial.print(millis() / 1000.0, 3);
  Serial.print(",");
  Serial.print(modeNames[currentMode]);
  Serial.print(",");
  Serial.print(refType);
  Serial.print(",");
  Serial.print(currentSetpoint, 3);
  Serial.print(",");
  Serial.print(flowRate, 3);
  Serial.print(",");
  Serial.print(waterLevel1, 2);
  Serial.print(",");
  Serial.print(waterLevel2, 2);
  Serial.print(",");
  Serial.print(currentPWM);
  Serial.print(",");
  Serial.print(P, 2);
  Serial.print(",");
  Serial.print(I, 2);
  Serial.print(",");
  Serial.print(D, 2);
  Serial.print(",");
  Serial.print(currentError, 3);
  Serial.print(",");
  Serial.println(totalVolume, 2);
  
  logCounter++;
}

// ============================================================================
// EXPERIMENTOS PREDEFINIDOS
// ============================================================================

void runExperiment(String expName) {
  Serial.println("\n[EXP] Ejecutando experimento: " + expName);
  
  if (expName == "STEP_FLOW") {
    Serial.println("[EXP] Respuesta escalón - Control de flujo");
    currentMode = MODE_AUTO_FLOW;
    pidFlow.enabled = true;
    pidFlow.setpoint = 0.5;
    refType = REF_STEP;
    refInitial = 0.5;
    refFinal = 1.5;
    refDuration = 10.0;
    refActive = true;
    refStartTime = millis();
    loggingEnabled = true;
    startMetrics();
  }
  else if (expName == "RAMP_LEVEL") {
    Serial.println("[EXP] Seguimiento rampa - Control de nivel");
    currentMode = MODE_AUTO_LEVEL1;
    pidLevel1.enabled = true;
    refType = REF_RAMP;
    refInitial = 10.0;
    refFinal = 30.0;
    refDuration = 30.0;
    refActive = true;
    refStartTime = millis();
    loggingEnabled = true;
  }
  else if (expName == "DISTURBANCE") {
    Serial.println("[EXP] Rechazo de perturbación");
    // Simular perturbación reduciendo PWM temporalmente
    int originalPWM = currentPWM;
    setMotorPWM(currentPWM * 0.5);
    delay(2000);
    setMotorPWM(originalPWM);
    Serial.println("[EXP] Perturbación aplicada");
  }
  else {
    Serial.println("[ERROR] Experimento desconocido");
  }
}

// ============================================================================
// LED STATUS
// ============================================================================

void updateStatusLED() {
  switch(currentMode) {
    case MODE_MANUAL:
      digitalWrite(LED_STATUS_PIN, (millis() / 1000) % 2);  // Parpadeo lento
      break;
    case MODE_AUTO_FLOW:
    case MODE_AUTO_LEVEL1:
    case MODE_AUTO_LEVEL2:
      digitalWrite(LED_STATUS_PIN, (millis() / 250) % 2);  // Parpadeo rápido
      break;
    case MODE_CASCADE:
      digitalWrite(LED_STATUS_PIN, (millis() / 100) % 2);  // Parpadeo muy rápido
      break;
  }
}

// ============================================================================
// COMANDOS
// ============================================================================

void processSerialCommands() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputCommand.length() > 0) commandComplete = true;
    } else {
      inputCommand += inChar;
    }
  }
  
  if (commandComplete) {
    inputCommand.trim();
    inputCommand.toUpperCase();
    Serial.print("\n[CMD] ");
    Serial.println(inputCommand);
    parseCommand(inputCommand);
    inputCommand = "";
    commandComplete = false;
  }
}

void parseCommand(String cmd) {
  // SETLEVEL1,<mm>
  if (cmd.startsWith("SETLEVEL1,")) {
    targetLevel1 = constrain(cmd.substring(10).toFloat(), 0.0, MAX_LEVEL_ALLOW);
    Serial.print("[LEVEL] Tank1 target=");
    Serial.print(targetLevel1, 1);
    Serial.println(" mm");
  }

  // SETLEVEL2,<mm>
  else if (cmd.startsWith("SETLEVEL2,")) {
    targetLevel2 = constrain(cmd.substring(10).toFloat(), 0.0, MAX_LEVEL_ALLOW);
    Serial.print("[LEVEL] Tank2 target=");
    Serial.print(targetLevel2, 1);
    Serial.println(" mm");
  }

  // LEVELCTRL,ON / LEVELCTRL,OFF
  else if (cmd == "LEVELCTRL,ON") {
    readAllSensors();
    levelValveControlEnabled = true;
    lastValveControlTime = 0;
    Serial.println("[LEVEL] Automatic valve height control ON");
    printInitialHeights();
  }

  else if (cmd == "LEVELCTRL,OFF") {
    levelValveControlEnabled = false;
    closeAllValves();
    Serial.println("[LEVEL] Automatic valve height control OFF - valves CLOSED");
  }

  // S1,<angle> / S2,<angle> manual valve angle
  else if (cmd.startsWith("S1,")) {
    levelValveControlEnabled = false;
    setValveAngle(1, cmd.substring(3).toInt());
  }

  else if (cmd.startsWith("S2,")) {
    levelValveControlEnabled = false;
    setValveAngle(2, cmd.substring(3).toInt());
  }

  // V1,OPEN / V1,CLOSE / V2,OPEN / V2,CLOSE
  else if (cmd == "V1,OPEN") {
    levelValveControlEnabled = false;
    openValve(1);
  }

  else if (cmd == "V1,CLOSE") {
    levelValveControlEnabled = false;
    closeValve(1);
  }

  else if (cmd == "V2,OPEN") {
    levelValveControlEnabled = false;
    openValve(2);
  }

  else if (cmd == "V2,CLOSE") {
    levelValveControlEnabled = false;
    closeValve(2);
  }

  else if (cmd == "VALVES,OPEN") {
    levelValveControlEnabled = false;
    openAllValves();
  }

  else if (cmd == "VALVES,CLOSE") {
    levelValveControlEnabled = false;
    closeAllValves();
  }

  else if (cmd == "VALVESTATUS") {
    printValveStatus();
  }

  // SETMODE,<mode>
  else if (cmd.startsWith("SETMODE,")) {
    String mode = cmd.substring(8);
    if (mode == "MANUAL") {
      currentMode = MODE_MANUAL;
      pidFlow.enabled = false;
      pidLevel1.enabled = false;
      pidLevel2.enabled = false;
    }
    else if (mode == "AUTO_FLOW") currentMode = MODE_AUTO_FLOW;
    else if (mode == "AUTO_LEVEL1") currentMode = MODE_AUTO_LEVEL1;
    else if (mode == "AUTO_LEVEL2") currentMode = MODE_AUTO_LEVEL2;
    else if (mode == "CASCADE") currentMode = MODE_CASCADE;
    else {
      Serial.println("[ERROR] Modo inválido");
      return;
    }
    Serial.println("[MODE] " + modeNames[currentMode]);
  }
  
  // SETREF,<type>,<param1>,<param2>,<param3>
  else if (cmd.startsWith("SETREF,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    int comma3 = cmd.indexOf(',', comma2 + 1);
    
    String type = cmd.substring(comma1 + 1, comma2);
    
    if (type == "STEP") {
      refType = REF_STEP;
      refInitial = cmd.substring(comma2 + 1, comma3).toFloat();
      refFinal = cmd.substring(comma3 + 1).toFloat();
      Serial.println("[REF] Escalón: " + String(refInitial) + " → " + String(refFinal));
    }
    else if (type == "RAMP") {
      refType = REF_RAMP;
      refInitial = cmd.substring(comma2 + 1, comma3).toFloat();
      int comma4 = cmd.indexOf(',', comma3 + 1);
      refFinal = cmd.substring(comma3 + 1, comma4).toFloat();
      refDuration = cmd.substring(comma4 + 1).toFloat();
      Serial.println("[REF] Rampa: " + String(refDuration) + "s");
    }
    else if (type == "PARA") {
      refType = REF_PARABOLIC;
      refInitial = cmd.substring(comma2 + 1, comma3).toFloat();
      int comma4 = cmd.indexOf(',', comma3 + 1);
      refFinal = cmd.substring(comma3 + 1, comma4).toFloat();
      refDuration = cmd.substring(comma4 + 1).toFloat();
      Serial.println("[REF] Parábola: " + String(refDuration) + "s");
    }
  }
  
  // SETPID1,<Kp>,<Ki>,<Kd>
  else if (cmd.startsWith("SETPID1,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    int comma3 = cmd.indexOf(',', comma2 + 1);
    pidLevel1.Kp = cmd.substring(comma1 + 1, comma2).toFloat();
    pidLevel1.Ki = cmd.substring(comma2 + 1, comma3).toFloat();
    pidLevel1.Kd = cmd.substring(comma3 + 1).toFloat();
    Serial.println("[PID1] Set");
  }
  
  // SETPID2,<Kp>,<Ki>,<Kd>
  else if (cmd.startsWith("SETPID2,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    int comma3 = cmd.indexOf(',', comma2 + 1);
    pidLevel2.Kp = cmd.substring(comma1 + 1, comma2).toFloat();
    pidLevel2.Ki = cmd.substring(comma2 + 1, comma3).toFloat();
    pidLevel2.Kd = cmd.substring(comma3 + 1).toFloat();
    Serial.println("[PID2] Set");
  }
  
  // SETPWM,<0-255>  — manual velocity in MANUAL mode
  else if (cmd.startsWith("SETPWM,")) {
    int pwmVal = cmd.substring(7).toInt();
    if (currentMode != MODE_MANUAL) {
      Serial.println("[ERROR] SETPWM solo funciona en modo MANUAL");
    } else if (pwmVal < 0 || pwmVal > 255) {
      Serial.println("[ERROR] PWM fuera de rango (0-255)");
    } else {
      setMotorPWM(pwmVal);
      Serial.print("[PWM] ");
      Serial.print(pwmVal);
      Serial.print(" (");
      Serial.print((pwmVal * 100.0) / 255.0, 1);
      Serial.println("%)");
    }
  }

  // CALMODE,<pwm_pct>  — entra en modo calibración
  else if (cmd.startsWith("CALMODE")) {
    int pwmPct = 60;  // default 60%
    if (cmd.indexOf(',') != -1) {
      pwmPct = constrain(cmd.substring(cmd.indexOf(',') + 1).toInt(), 0, 100);
    }
    int pwmVal = (pwmPct * 255) / 100;
    // Detener PIDs antes de calibrar
    pidFlow.enabled = false;
    pidLevel1.enabled = false;
    pidLevel2.enabled = false;
    calModeActive = true;
    setMotorPWM(pwmVal);
    Serial.print("[CAL] Modo calibración activo. PWM=");
    Serial.print(pwmVal);
    Serial.print(" (");
    Serial.print(pwmPct);
    Serial.println("%). Use TANK1_EMPTY / TANK1_FULL / TANK2_EMPTY / TANK2_FULL para marcar niveles");
  }

  // CALSTOP  — salir de modo calibración
  else if (cmd == "CALSTOP") {
    calModeActive = false;
    stopMotor();
    Serial.println("[CAL] Modo calibración detenido");
  }

  // SETCAL,<tank>,<empty_adc>,<full_adc>  — aplica calibración
  else if (cmd.startsWith("SETCAL,")) {
    int c1 = cmd.indexOf(',');
    int c2 = cmd.indexOf(',', c1 + 1);
    int c3 = cmd.indexOf(',', c2 + 1);
    int tank = cmd.substring(c1 + 1, c2).toInt();
    float emptyADC = cmd.substring(c2 + 1, c3).toFloat();
    float fullADC  = cmd.substring(c3 + 1).toFloat();
    if (tank == 1) {
      cal1_empty_adc = emptyADC;
      cal1_full_adc  = fullADC;
      cal1_calibrated = true;
      Serial.print("[CAL] Tank1 calibrado: empty=");
      Serial.print(emptyADC, 0);
      Serial.print(" full=");
      Serial.println(fullADC, 0);
    } else if (tank == 2) {
      cal2_empty_adc = emptyADC;
      cal2_full_adc  = fullADC;
      cal2_calibrated = true;
      Serial.print("[CAL] Tank2 calibrado: empty=");
      Serial.print(emptyADC, 0);
      Serial.print(" full=");
      Serial.println(fullADC, 0);
    } else {
      Serial.println("[ERROR] Tank debe ser 1 o 2");
    }
  }

  // GETCAL  — muestra calibración actual
  else if (cmd == "GETCAL") {
    Serial.println("[CAL] === Calibración actual ===");
    Serial.print("[CAL] Tank1: empty="); Serial.print(cal1_empty_adc,0);
    Serial.print(" full="); Serial.print(cal1_full_adc,0);
    Serial.println(cal1_calibrated ? " [OK]" : " [sin calibrar]");
    Serial.print("[CAL] Tank2: empty="); Serial.print(cal2_empty_adc,0);
    Serial.print(" full="); Serial.print(cal2_full_adc,0);
    Serial.println(cal2_calibrated ? " [OK]" : " [sin calibrar]");
  }

  // STARTCTRL
  else if (cmd == "STARTCTRL") {
    readAllSensors();
    printInitialHeights();
    switch(currentMode) {
      case MODE_AUTO_FLOW:
        pidFlow.enabled = true;
        resetPIDController(&pidFlow);
        break;
      case MODE_AUTO_LEVEL1:
        pidLevel1.enabled = true;
        resetPIDController(&pidLevel1);
        break;
      case MODE_AUTO_LEVEL2:
        pidLevel2.enabled = true;
        resetPIDController(&pidLevel2);
        break;
      case MODE_CASCADE:
        pidFlow.enabled = true;
        pidLevel1.enabled = true;
        resetPIDController(&pidFlow);
        resetPIDController(&pidLevel1);
        break;
    }
    refActive = true;
    refStartTime = millis();
    Serial.println("[CTRL] Control iniciado");
  }
  
  // STOPCTRL
  else if (cmd == "STOPCTRL") {
    pidFlow.enabled = false;
    pidLevel1.enabled = false;
    pidLevel2.enabled = false;
    refActive = false;
    levelValveControlEnabled = false;
    stopMotor();
    closeAllValves();
    Serial.println("[CTRL] Control detenido");
  }
  
  // METRICS
  else if (cmd == "METRICS") {
    if (metrics.measuring) {
      metrics.measuring = false;
      printMetrics();
    } else {
      startMetrics();
    }
  }
  
  // DATALOG
  else if (cmd == "DATALOG") {
    loggingEnabled = !loggingEnabled;
    if (loggingEnabled) {
      logCounter = 0;
      Serial.println("[LOG] Data logging iniciado");
    } else {
      Serial.println("[LOG] Data logging detenido");
    }
  }
  
  // EXPERIMENT,<name>
  else if (cmd.startsWith("EXPERIMENT,")) {
    String expName = cmd.substring(11);
    runExperiment(expName);
  }
  
  // STATUS
  else if (cmd == "STATUS") {
    printStatus();
  }
  
  // HELP
  else if (cmd == "HELP") {
    printHelp();
  }
  
  else {
    Serial.println("[ERROR] Comando no reconocido - Use HELP");
  }
}

// ============================================================================
// INFORMACIÓN
// ============================================================================

void printStatus() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║          ESTADO COMPLETO DEL SISTEMA                  ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  
  Serial.println("\n[MODO DE OPERACIÓN]");
  Serial.println("  " + modeNames[currentMode]);
  
  Serial.println("\n[SENSORES]");
  Serial.print("  Flujo: ");
  Serial.print(flowRate, 3);
  Serial.println(" L/min");
  Serial.print("  Nivel Tank 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  Serial.print("  Nivel Tank 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  Serial.print("  Volumen total: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  
  Serial.println("\n[MOTOR]");
  Serial.print("  PWM: ");
  Serial.print(currentPWM);
  Serial.print(" (");
  Serial.print((currentPWM * 100.0) / 255, 1);
  Serial.println("%)");

  Serial.println("\n[VALVULAS SERVO]");
  Serial.print("  Control altura: ");
  Serial.println(levelValveControlEnabled ? "ON" : "OFF");
  Serial.print("  Tank 1 objetivo: ");
  Serial.print(targetLevel1, 1);
  Serial.print(" mm | valvula: ");
  Serial.println(tank1ValveAngle);
  Serial.print("  Tank 2 objetivo: ");
  Serial.print(targetLevel2, 1);
  Serial.print(" mm | valvula: ");
  Serial.println(tank2ValveAngle);
  Serial.println("  170 = abierta, 180 = cerrada");
  
  Serial.println("\n[PID FLUJO]");
  Serial.print("  Activo: ");
  Serial.println(pidFlow.enabled ? "SÍ" : "NO");
  Serial.print("  Kp/Ki/Kd: ");
  Serial.print(pidFlow.Kp);
  Serial.print(" / ");
  Serial.print(pidFlow.Ki);
  Serial.print(" / ");
  Serial.println(pidFlow.Kd);
  
  Serial.println("\n[PID NIVEL 1]");
  Serial.print("  Activo: ");
  Serial.println(pidLevel1.enabled ? "SÍ" : "NO");
  Serial.print("  Kp/Ki/Kd: ");
  Serial.print(pidLevel1.Kp);
  Serial.print(" / ");
  Serial.print(pidLevel1.Ki);
  Serial.print(" / ");
  Serial.println(pidLevel1.Kd);
  
  Serial.println("\n════════════════════════════════════════════════════════\n");
}

void printHelp() {
  Serial.println();
  Serial.println("Commands - Sprint 09 Full System + Servo Valves");
  Serial.println("  SETMODE,<mode>        MANUAL, AUTO_FLOW, AUTO_LEVEL1, AUTO_LEVEL2, CASCADE");
  Serial.println("  SETPWM,<0-255>        Manual pump PWM in MANUAL mode");
  Serial.println("  SETREF,<type>,...     STEP, RAMP, PARA");
  Serial.println("  SETPID1,<Kp>,<Ki>,<Kd>");
  Serial.println("  SETPID2,<Kp>,<Ki>,<Kd>");
  Serial.println("  STARTCTRL             Start pump/PID control and print current heights");
  Serial.println("  STOPCTRL              Stop control, stop pump, close valves");
  Serial.println("  SETLEVEL1,<mm>        Set Tank 1 height target");
  Serial.println("  SETLEVEL2,<mm>        Set Tank 2 height target");
  Serial.println("  LEVELCTRL,ON          Enable automatic valve height control");
  Serial.println("  LEVELCTRL,OFF         Disable valve control and close valves");
  Serial.println("  S1,<0-180>            Manual Tank 1 valve angle");
  Serial.println("  S2,<0-180>            Manual Tank 2 valve angle");
  Serial.println("  V1,OPEN / V1,CLOSE    Manual Tank 1 valve");
  Serial.println("  V2,OPEN / V2,CLOSE    Manual Tank 2 valve");
  Serial.println("  VALVES,OPEN/CLOSE     Manual both valves");
  Serial.println("  VALVESTATUS           Print actual heights, targets, valve angles");
  Serial.println("  STATUS                Full system status");
  Serial.println("  HELP                  Show commands");
  Serial.println();
  Serial.println("Valve calibration: 170=open, 180=closed.");
  Serial.println();
  return;
  Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║        COMANDOS - SISTEMA DE CONTROL COMPLETO               ║");
  Serial.println("╠══════════════════════════════════════════════════════════════╣");
  Serial.println("║ SETMODE,<mode>         - Modos: MANUAL, AUTO_FLOW,          ║");
  Serial.println("║                          AUTO_LEVEL1, AUTO_LEVEL2, CASCADE   ║");
  Serial.println("║ SETPWM,<0-255>         - Velocidad manual (solo MANUAL)     ║");
  Serial.println("║ SETREF,<type>,<params> - Tipo: STEP,0,1                     ║");
  Serial.println("║                          RAMP,0,1,10  PARA,0,1,10            ║");
  Serial.println("║ SETPID1,<Kp>,<Ki>,<Kd> - Ajustar PID Nivel 1                ║");
  Serial.println("║ SETPID2,<Kp>,<Ki>,<Kd> - Ajustar PID Nivel 2                ║");
  Serial.println("║ STARTCTRL              - Iniciar control automático         ║");
  Serial.println("║ STOPCTRL               - Detener control                    ║");
  Serial.println("║ METRICS                - Medir/mostrar métricas             ║");
  Serial.println("║ DATALOG                - Toggle logging completo            ║");
  Serial.println("║ EXPERIMENT,<name>      - STEP_FLOW, RAMP_LEVEL, DISTURBANCE ║");
  Serial.println("║ STATUS                 - Estado completo sistema            ║");
  Serial.println("║ HELP                   - Mostrar comandos                   ║");
  Serial.println("╠══════════════════════════════════════════════════════════════╣");
  Serial.println("║ Ejemplo de uso:                                             ║");
  Serial.println("║   1. SETMODE,AUTO_FLOW                                       ║");
  Serial.println("║   2. SETREF,STEP,0.5,1.5,10                                  ║");
  Serial.println("║   3. STARTCTRL                                               ║");
  Serial.println("║   4. DATALOG                                                 ║");
  Serial.println("║   5. METRICS                                                 ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// FIN DEL PROGRAMA
// ============================================================================
