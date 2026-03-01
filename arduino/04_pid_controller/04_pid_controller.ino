/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE FLUJO HIDRÁULICO
 * Universidad Militar Nueva Granada
 * Ingeniería Mecatrónica - Control Lineal y Laboratorio
 * ============================================================================
 * 
 * SPRINT 4: CONTROLADOR PID
 * 
 * Descripción:
 *   Control en lazo cerrado del flujo de agua mediante controlador PID digital.
 *   Sintonizado por método de Ziegler-Nichols para seguimiento de referencias.
 * 
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - H-Bridge L298N + Motor 12V
 *   - Sensor de flujo YF-S401
 *   - 2x Sensores de nivel SE045
 * 
 * Conexiones:
 *   GPIO 17 → H-Bridge ENA (PWM)
 *   GPIO 15 → H-Bridge IN1
 *   GPIO 16 → H-Bridge IN2
 *   GPIO 4  → YF-S401 Signal
 *   GPIO 5  → SE045 Tank1
 *   GPIO 6  → SE045 Tank2
 *   GPIO 7  → LED Status
 * 
 * Comandos UART (115200 baud):
 *   SETSP,<val>          - Establecer setpoint (L/min)
 *   SETKP,<val>          - Ajustar Kp
 *   SETKI,<val>          - Ajustar Ki
 *   SETKD,<val>          - Ajustar Kd
 *   STARTPID             - Iniciar control PID
 *   STOPPID              - Detener control PID
 *   TUNEAUTO             - Sintonización automática (Ziegler-Nichols)
 *   STEP,<val>           - Respuesta escalón (análisis)
 *   LOGPID               - Logging con PID activo
 *   STATUS               - Estado del sistema
 *   HELP                 - Comandos disponibles
 * 
 * Autor: Daniel García Araque, David Santiago García Suarez
 * Fecha: Febrero 2026
 * Versión: 1.0
 * ============================================================================
 */

// ============================================================================
// DEFINICIÓN DE PINES
// ============================================================================

#define MOTOR_PWM_PIN    17
#define MOTOR_IN1_PIN    15
#define MOTOR_IN2_PIN    16
#define FLOW_SENSOR_PIN  4
#define LEVEL1_PIN       5
#define LEVEL2_PIN       6
#define LED_STATUS_PIN   7

// ============================================================================
// CONFIGURACIÓN PWM Y ADC
// ============================================================================

#define PWM_FREQUENCY    10000
#define PWM_RESOLUTION   8
#define PWM_MIN          0
#define PWM_MAX          255

#define ADC_RESOLUTION   4095
#define ADC_VREF         3.3
#define ADC_SAMPLES      10

const float VOLTAGE_DIVIDER_FACTOR = 0.6875;
const float SENSOR_MAX_VOLTAGE = 4.5;
const float SENSOR_MAX_HEIGHT = 40.0;
float flowCalibrationFactor = 98.0;

// ============================================================================
// PARÁMETROS PID
// ============================================================================

// Sintonización inicial (ajustar según sistema real)
float Kp = 50.0;     // Ganancia proporcional
float Ki = 10.0;     // Ganancia integral
float Kd = 5.0;      // Ganancia derivativa

// Variables PID
float setpoint = 1.0;        // Referencia de flujo (L/min)
float processValue = 0.0;    // Flujo medido (L/min)
float error = 0.0;
float lastError = 0.0;
float integral = 0.0;
float derivative = 0.0;
float output = 0.0;

// Control de tiempo
float dt = 0.1;              // Período de muestreo (100ms = 10Hz)
unsigned long lastPIDTime = 0;

// Anti-windup
const float INTEGRAL_MAX = 200.0;
const float INTEGRAL_MIN = -200.0;

// Límites de salida
const float OUTPUT_MIN = 0.0;
const float OUTPUT_MAX = 255.0;

// Estado del controlador
bool pidEnabled = false;

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
int filter_index = 0;

// ============================================================================
// VARIABLES MOTOR
// ============================================================================

int currentPWM = 0;
bool motorDirection = 0;
bool motorRunning = false;

// ============================================================================
// LOGGING Y ANÁLISIS
// ============================================================================

bool loggingEnabled = false;
unsigned long lastLogTime = 0;
unsigned long logInterval = 100;  // 100ms para capturar dinámica PID
int logCounter = 0;

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
  Serial.println("  LABORATORIO 2 - CONTROLADOR PID");
  Serial.println("  Universidad Militar Nueva Granada");
  Serial.println("  ESP32-S3 - Sprint 4: Closed-Loop Control");
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
  
  // Configurar sensores
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
  analogReadResolution(12);
  
  // Dirección adelante
  setMotorDirection(0);
  
  Serial.println("[OK] Sistema inicializado");
  Serial.print("[PID] Kp=");
  Serial.print(Kp, 2);
  Serial.print(" Ki=");
  Serial.print(Ki, 2);
  Serial.print(" Kd=");
  Serial.println(Kd, 2);
  Serial.print("[PID] Setpoint=");
  Serial.print(setpoint, 3);
  Serial.println(" L/min");
  Serial.println("[READY] Use STARTPID para iniciar control");
  Serial.println();
  
  printHelp();
  
  for(int i=0; i<3; i++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(200);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(200);
  }
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  processSerialCommands();
  
  // Actualizar flujo cada segundo
  if (millis() - lastFlowUpdate >= flowUpdateInterval) {
    updateFlowRate();
    lastFlowUpdate = millis();
  }
  
  // Ejecutar PID a frecuencia fija (10Hz)
  if (pidEnabled && (millis() - lastPIDTime >= dt * 1000)) {
    computePID();
    lastPIDTime = millis();
  }
  
  // Logging
  if (loggingEnabled && (millis() - lastLogTime >= logInterval)) {
    logPIDData();
    lastLogTime = millis();
  }
  
  // LED indica PID activo
  if (pidEnabled) {
    digitalWrite(LED_STATUS_PIN, (millis() / 250) % 2);  // Parpadeo rápido
  } else {
    digitalWrite(LED_STATUS_PIN, (millis() / 1000) % 2);  // Parpadeo lento
  }
  
  delay(5);
}

// ============================================================================
// ALGORITMO PID
// ============================================================================

void computePID() {
  // Leer variable de proceso (flujo)
  processValue = flowRate;
  
  // Calcular error
  error = setpoint - processValue;
  
  // Término proporcional
  float P = Kp * error;
  
  // Término integral con anti-windup
  integral += error * dt;
  if (integral > INTEGRAL_MAX) integral = INTEGRAL_MAX;
  if (integral < INTEGRAL_MIN) integral = INTEGRAL_MIN;
  float I = Ki * integral;
  
  // Término derivativo con filtrado
  derivative = (error - lastError) / dt;
  float D = Kd * derivative;
  
  // Señal de control total
  output = P + I + D;
  
  // Saturación de salida
  if (output > OUTPUT_MAX) output = OUTPUT_MAX;
  if (output < OUTPUT_MIN) output = OUTPUT_MIN;
  
  // Aplicar al motor
  setMotorPWM((int)output);
  
  // Actualizar error anterior
  lastError = error;
}

/**
 * Resetear PID (cuando se inicia o cambia setpoint)
 */
void resetPID() {
  integral = 0;
  lastError = 0;
  derivative = 0;
  output = 0;
  Serial.println("[PID] Variables reseteadas");
}

// ============================================================================
// FUNCIONES MOTOR
// ============================================================================

void setMotorPWM(int pwmValue) {
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

float readWaterLevel1() {
  long adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(LEVEL1_PIN);
    delayMicroseconds(100);
  }
  float adcValue = adcSum / (float)ADC_SAMPLES;
  float voltage_pin = (adcValue / ADC_RESOLUTION) * ADC_VREF;
  float voltage_sensor = voltage_pin / VOLTAGE_DIVIDER_FACTOR;
  float height = (voltage_sensor / SENSOR_MAX_VOLTAGE) * SENSOR_MAX_HEIGHT;
  
  level1_buffer[filter_index] = height;
  float filtered = 0;
  for (int i = 0; i < FILTER_SIZE; i++) filtered += level1_buffer[i];
  return filtered / FILTER_SIZE;
}

float readWaterLevel2() {
  long adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(LEVEL2_PIN);
    delayMicroseconds(100);
  }
  float adcValue = adcSum / (float)ADC_SAMPLES;
  float voltage_pin = (adcValue / ADC_RESOLUTION) * ADC_VREF;
  float voltage_sensor = voltage_pin / VOLTAGE_DIVIDER_FACTOR;
  float height = (voltage_sensor / SENSOR_MAX_VOLTAGE) * SENSOR_MAX_HEIGHT;
  
  level2_buffer[filter_index] = height;
  filter_index = (filter_index + 1) % FILTER_SIZE;
  
  float filtered = 0;
  for (int i = 0; i < FILTER_SIZE; i++) filtered += level2_buffer[i];
  return filtered / FILTER_SIZE;
}

void readAllSensors() {
  waterLevel1 = readWaterLevel1();
  waterLevel2 = readWaterLevel2();
}

// ============================================================================
// LOGGING Y ANÁLISIS
// ============================================================================

void logPIDData() {
  readAllSensors();
  
  if (logCounter % 50 == 0) {
    Serial.println("\nTime_s,Setpoint,Flow,Error,PWM,P,I,D,Level1,Level2");
  }
  
  // CSV para análisis
  Serial.print(millis() / 1000.0, 3);
  Serial.print(",");
  Serial.print(setpoint, 3);
  Serial.print(",");
  Serial.print(processValue, 3);
  Serial.print(",");
  Serial.print(error, 3);
  Serial.print(",");
  Serial.print(currentPWM);
  Serial.print(",");
  Serial.print(Kp * error, 2);
  Serial.print(",");
  Serial.print(Ki * integral, 2);
  Serial.print(",");
  Serial.print(Kd * derivative, 2);
  Serial.print(",");
  Serial.print(waterLevel1, 2);
  Serial.print(",");
  Serial.println(waterLevel2, 2);
  
  logCounter++;
}

/**
 * Respuesta a escalón para análisis
 */
void stepResponse(float targetFlow) {
  Serial.println("\n[STEP] Iniciando prueba de escalón");
  Serial.println("[STEP] Aplicando setpoint: " + String(targetFlow, 3) + " L/min");
  
  setpoint = targetFlow;
  resetPID();
  
  pidEnabled = true;
  loggingEnabled = true;
  logCounter = 0;
  
  Serial.println("\nTime_s,Setpoint,Flow,Error,PWM");
  Serial.println("Recolectando datos durante 60 segundos...");
  
  unsigned long startTime = millis();
  while (millis() - startTime < 60000) {  // 60 segundos
    delay(100);
  }
  
  loggingEnabled = false;
  pidEnabled = false;
  stopMotor();
  
  Serial.println("\n[STEP] Prueba completada");
  Serial.println("[INFO] Exportar datos a MATLAB para análisis de desempeño");
}

// ============================================================================
// SINTONIZACIÓN AUTOMÁTICA (SIMPLIFICADA)
// ============================================================================

void autoTune() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║     SINTONIZACIÓN AUTOMÁTICA (Ziegler-Nichols)        ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");
  
  Serial.println("[INFO] Método de oscilación sostenida");
  Serial.println("[PASO 1] Desactivar Ki y Kd");
  
  float original_Kp = Kp;
  float original_Ki = Ki;
  float original_Kd = Kd;
  
  Ki = 0;
  Kd = 0;
  Kp = 10.0;  // Empezar con Kp bajo
  
  Serial.println("[PASO 2] Incrementar Kp hasta oscilación sostenida");
  Serial.println("[INFO] Este proceso es manual - monitoree el flujo");
  Serial.println("[INFO] Cuando oscile establemente, anote Ku y Pu");
  Serial.println();
  Serial.println("Ejemplo de cálculo:");
  Serial.println("  Si Ku = 80 y Pu = 4.0s:");
  Serial.println("  Kp = 0.6 × Ku = 48");');
  Serial.println("  Ki = 2 × Kp / Pu = 24");
  Serial.println("  Kd = Kp × Pu / 8 = 24");
  Serial.println();
  Serial.println("[INFO] Use SETKP, SETKI, SETKD para aplicar");
  
  // Restaurar valores
  Kp = original_Kp;
  Ki = original_Ki;
  Kd = original_Kd;
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
  // SETSP,<valor>
  if (cmd.startsWith("SETSP,")) {
    float sp = cmd.substring(6).toFloat();
    setpoint = sp;
    resetPID();
    Serial.print("[PID] Setpoint = ");
    Serial.print(setpoint, 3);
    Serial.println(" L/min");
  }
  
  // SETKP,<valor>
  else if (cmd.startsWith("SETKP,")) {
    Kp = cmd.substring(6).toFloat();
    Serial.print("[PID] Kp = ");
    Serial.println(Kp, 2);
  }
  
  // SETKI,<valor>
  else if (cmd.startsWith("SETKI,")) {
    Ki = cmd.substring(6).toFloat();
    Serial.print("[PID] Ki = ");
    Serial.println(Ki, 2);
  }
  
  // SETKD,<valor>
  else if (cmd.startsWith("SETKD,")) {
    Kd = cmd.substring(6).toFloat();
    Serial.print("[PID] Kd = ");
    Serial.println(Kd, 2);
  }
  
  // STARTPID
  else if (cmd == "STARTPID") {
    resetPID();
    pidEnabled = true;
    Serial.println("[PID] Control iniciado");
    Serial.println("[PID] Siguiendo setpoint: " + String(setpoint, 3) + " L/min");
  }
  
  // STOPPID
  else if (cmd == "STOPPID") {
    pidEnabled = false;
    stopMotor();
    Serial.println("[PID] Control detenido");
  }
  
  // STEP,<valor>
  else if (cmd.startsWith("STEP,")) {
    float sp = cmd.substring(5).toFloat();
    stepResponse(sp);
  }
  
  // TUNEAUTO
  else if (cmd == "TUNEAUTO") {
    autoTune();
  }
  
  // LOGPID
  else if (cmd == "LOGPID") {
    if (!pidEnabled) {
      Serial.println("[ERROR] Inicie PID primero con STARTPID");
    } else {
      loggingEnabled = !loggingEnabled;
      if (loggingEnabled) {
        logCounter = 0;
        Serial.println("[LOG] Logging PID iniciado");
      } else {
        Serial.println("[LOG] Logging detenido");
      }
    }
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
  readAllSensors();
  
  Serial.println("\n========== ESTADO DEL SISTEMA ==========");
  
  Serial.println("\n[CONTROLADOR PID]");
  Serial.print("  Estado: ");
  Serial.println(pidEnabled ? "ACTIVO" : "INACTIVO");
  Serial.print("  Setpoint: ");
  Serial.print(setpoint, 3);
  Serial.println(" L/min");
  Serial.print("  Proceso (flujo): ");
  Serial.print(processValue, 3);
  Serial.println(" L/min");
  Serial.print("  Error: ");
  Serial.print(error, 3);
  Serial.println(" L/min");
  
  Serial.println("\n[PARÁMETROS PID]");
  Serial.print("  Kp = ");
  Serial.println(Kp, 2);
  Serial.print("  Ki = ");
  Serial.println(Ki, 2);
  Serial.print("  Kd = ");
  Serial.println(Kd, 2);
  
  Serial.println("\n[SALIDA CONTROL]");
  Serial.print("  PWM: ");
  Serial.print(currentPWM);
  Serial.print(" (");
  Serial.print((currentPWM * 100.0) / 255, 1);
  Serial.println("%)");
  Serial.print("  P: ");
  Serial.println(Kp * error, 2);
  Serial.print("  I: ");
  Serial.println(Ki * integral, 2);
  Serial.print("  D: ");
  Serial.println(Kd * derivative, 2);
  
  Serial.println("\n[SENSORES]");
  Serial.print("  Nivel Tank 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  Serial.print("  Nivel Tank 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  Serial.print("  Volumen: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  
  Serial.println("========================================\n");
}

void printHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║           COMANDOS - CONTROLADOR PID                     ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  SETSP,<val>          - Setpoint (L/min, ej: 1.5)        ║");
  Serial.println("║  SETKP,<val>          - Ajustar Kp (ej: 50.0)            ║");
  Serial.println("║  SETKI,<val>          - Ajustar Ki (ej: 10.0)            ║");
  Serial.println("║  SETKD,<val>          - Ajustar Kd (ej: 5.0)             ║");
  Serial.println("║  STARTPID             - Iniciar control PID              ║");
  Serial.println("║  STOPPID              - Detener control PID              ║");
  Serial.println("║  TUNEAUTO             - Guía sintonización Z-N          ║");
  Serial.println("║  STEP,<val>           - Respuesta escalón (60s)          ║");
  Serial.println("║  LOGPID               - Toggle logging datos PID         ║");
  Serial.println("║  STATUS               - Estado completo                  ║");
  Serial.println("║  HELP                 - Mostrar comandos                 ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  Secuencia típica:                                       ║");
  Serial.println("║    1. SETSP,1.2       → Definir referencia              ║");
  Serial.println("║    2. STARTPID        → Activar control                  ║");
  Serial.println("║    3. LOGPID          → Ver datos en tiempo real         ║");
  Serial.println("║    4. Ajustar Kp/Ki/Kd según respuesta                   ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// FIN DEL PROGRAMA
// ============================================================================
