/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE FLUJO HIDRÁULICO
 * Universidad Militar Nueva Granada
 * Ingeniería Mecatrónica - Control Lineal y Laboratorio
 * ============================================================================
 * 
 * SPRINT 3: INTEGRACIÓN SENSOR-ACTUADOR
 * 
 * Descripción:
 *   Integración completa de control de motor (Sprint 1) y lectura de sensores
 *   (Sprint 2) para caracterización del sistema en lazo abierto.
 *   Nota: Sprint 9 es la referencia vigente del pinout integrado; este sketch
 *   conserva la ruta histórica SE045/ADC para caracterización.
 *   Permite identificar la relación PWM → Flujo para modelado matemático.
 * 
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - H-Bridge L298N + Motor 12V
 *   - Sensor de flujo YF-S401
 *   - 2x Sensores de nivel SE045 (historico; Sprint 9 usa HC-SR04)
 *   - Divisores resistivos 10kΩ/22kΩ para la ruta historica
 * 
 * Conexiones historicas de este sketch. Referencia vigente de sistema: Sprint 9.
 *   GPIO 17 → H-Bridge ENA (PWM)
 *   GPIO 15 → H-Bridge IN1
 *   GPIO 16 → H-Bridge IN2
 *   GPIO 4  → YF-S401 Signal
 *   GPIO 5  → SE045 Tank1 historico (Sprint 9: HC-SR04 T1 TRIG)
 *   GPIO 6  → SE045 Tank2 historico (Sprint 9: HC-SR04 T1 ECHO)
 *   GPIO 7  → LED Status
 * 
 * Comandos UART (115200 baud):
 *   SETPWM,<val>         - Establecer PWM motor (0-255)
 *   SWEEP,<start>,<end>,<step>,<delay> - Barrido PWM automático
 *   LOGDATA              - Iniciar logging de datos (CSV)
 *   STOPLOG              - Detener logging
 *   CHARACTERIZE         - Caracterización automática completa
 *   ALLDATA              - Lectura completa (motor + sensores)
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

// Pines motor (H-Bridge)
#define MOTOR_PWM_PIN    17
#define MOTOR_IN1_PIN    15
#define MOTOR_IN2_PIN    16

// Pines sensores
#define FLOW_SENSOR_PIN  4
#define LEVEL1_PIN       5
#define LEVEL2_PIN       6

// Indicadores
#define LED_STATUS_PIN   7

// ============================================================================
// CONFIGURACIÓN PWM
// ============================================================================

#define PWM_FREQUENCY    10000
#define PWM_RESOLUTION   8
#define PWM_MIN          0
#define PWM_MAX          255

// ============================================================================
// CONFIGURACIÓN SENSORES
// ============================================================================

#define ADC_RESOLUTION   4095
#define ADC_VREF         3.3
#define ADC_SAMPLES      10

const float VOLTAGE_DIVIDER_FACTOR = 0.6875;
const float SENSOR_MAX_VOLTAGE = 4.5;
const float SENSOR_MAX_HEIGHT = 40.0;

// Calibración flujo
float flowCalibrationFactor = 98.0;  // pulsos/(L/min)

// ============================================================================
// VARIABLES GLOBALES - MOTOR
// ============================================================================

int currentPWM = 0;
bool motorDirection = 0;  // 0=adelante
bool motorRunning = false;

// ============================================================================
// VARIABLES GLOBALES - SENSORES
// ============================================================================

volatile unsigned long flowPulseCount = 0;
float flowRate = 0.0;           // L/min
float totalVolume = 0.0;        // L
float waterLevel1 = 0.0;        // mm (con baseline ~40mm)
float waterLevel2 = 0.0;        // mm

unsigned long lastFlowUpdate = 0;
const unsigned long flowUpdateInterval = 1000;

// Filtro nivel
const int FILTER_SIZE = 5;
float level1_buffer[FILTER_SIZE] = {0};
float level2_buffer[FILTER_SIZE] = {0};
int filter_index = 0;

// ============================================================================
// VARIABLES PARA CARACTERIZACIÓN
// ============================================================================

bool loggingEnabled = false;
unsigned long logInterval = 2000;  // 2 segundos por defecto
unsigned long lastLogTime = 0;
int logCounter = 0;

// ============================================================================
// COMUNICACIÓN
// ============================================================================

String inputCommand = "";
bool commandComplete = false;

// ============================================================================
// ISR SENSOR DE FLUJO
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
  Serial.println("  LABORATORIO 2 - INTEGRACIÓN SENSOR-ACTUADOR");
  Serial.println("  Universidad Militar Nueva Granada");
  Serial.println("  ESP32-S3 - Sprint 3: Open-Loop Characterization");
  Serial.println("============================================================");
  Serial.println();
  
  // Configurar pines motor
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);
  
  // Configurar PWM
  ledcAttach(MOTOR_PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcWrite(MOTOR_PWM_PIN, 0);
  
  // Configurar sensores
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
  
  analogReadResolution(12);
  
  // Dirección adelante por defecto
  setMotorDirection(0);
  
  Serial.println("[OK] Motor configurado");
  Serial.println("[OK] Sensores configurados");
  Serial.println("[READY] Sistema listo para caracterización");
  Serial.println();
  
  printHelp();
  
  // Blink de inicio
  for(int i=0; i<3; i++) {
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(200);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(200);
  }
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
  // Procesar comandos
  processSerialCommands();
  
  // Actualizar flujo cada segundo
  if (millis() - lastFlowUpdate >= flowUpdateInterval) {
    updateFlowRate();
    lastFlowUpdate = millis();
  }
  
  // Logging automático si está habilitado
  if (loggingEnabled && (millis() - lastLogTime >= logInterval)) {
    logDataPoint();
    lastLogTime = millis();
  }
  
  // LED heartbeat
  digitalWrite(LED_STATUS_PIN, (millis() / 500) % 2);
  
  delay(10);
}

// ============================================================================
// FUNCIONES DE MOTOR
// ============================================================================

void setMotorPWM(int pwmValue) {
  if (pwmValue < PWM_MIN) pwmValue = PWM_MIN;
  if (pwmValue > PWM_MAX) pwmValue = PWM_MAX;
  
  ledcWrite(MOTOR_PWM_PIN, pwmValue);
  currentPWM = pwmValue;
  motorRunning = (pwmValue > 0);
  
  Serial.print("[MOTOR] PWM: ");
  Serial.print(pwmValue);
  Serial.print(" (");
  Serial.print((pwmValue * 100.0) / PWM_MAX, 1);
  Serial.println("%)");
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
  Serial.println("[MOTOR] Detenido");
}

// ============================================================================
// FUNCIONES DE SENSORES
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
  for (int i = 0; i < FILTER_SIZE; i++) {
    filtered += level1_buffer[i];
  }
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
  
  float filtered = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    filtered += level2_buffer[i];
  }
  
  filter_index = (filter_index + 1) % FILTER_SIZE;
  
  return filtered / FILTER_SIZE;
}

void readAllSensors() {
  waterLevel1 = readWaterLevel1();
  waterLevel2 = readWaterLevel2();
}

// ============================================================================
// FUNCIONES DE CARACTERIZACIÓN
// ============================================================================

/**
 * Barrido de PWM para caracterización
 */
void pwmSweep(int startPWM, int endPWM, int stepPWM, int delaySeconds) {
  Serial.println("\n[SWEEP] Iniciando barrido de caracterización");
  Serial.println("Timestamp,PWM,PWM_pct,Flow_Lmin,Level1_mm,Level2_mm,Volume_L");
  Serial.println("─────────────────────────────────────────────────────────────");
  
  for (int pwm = startPWM; pwm <= endPWM; pwm += stepPWM) {
    setMotorPWM(pwm);
    
    // Esperar estabilización
    delay(delaySeconds * 1000);
    
    // Leer sensores
    readAllSensors();
    
    // Imprimir datos (formato CSV)
    Serial.print(millis() / 1000);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print((pwm * 100.0) / PWM_MAX, 2);
    Serial.print(",");
    Serial.print(flowRate, 3);
    Serial.print(",");
    Serial.print(waterLevel1, 2);
    Serial.print(",");
    Serial.print(waterLevel2, 2);
    Serial.print(",");
    Serial.println(totalVolume, 3);
    
    delay(500);
  }
  
  // Detener motor al final
  stopMotor();
  Serial.println("─────────────────────────────────────────────────────────────");
  Serial.println("[SWEEP] Barrido completado");
  Serial.println("[INFO] Copie los datos a Excel/MATLAB para análisis\n");
}

/**
 * Logging continuo de datos
 */
void logDataPoint() {
  readAllSensors();
  
  // Header cada 20 líneas
  if (logCounter % 20 == 0) {
    Serial.println("\nTime_s,PWM,Flow_Lmin,Level1_mm,Level2_mm,Volume_L");
  }
  
  Serial.print(millis() / 1000);
  Serial.print(",");
  Serial.print(currentPWM);
  Serial.print(",");
  Serial.print(flowRate, 3);
  Serial.print(",");
  Serial.print(waterLevel1, 2);
  Serial.print(",");
  Serial.print(waterLevel2, 2);
  Serial.print(",");
  Serial.println(totalVolume, 3);
  
  logCounter++;
}

/**
 * Caracterización automática completa del sistema
 */
void runCharacterization() {
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║     CARACTERIZACIÓN AUTOMÁTICA DEL SISTEMA             ║");
  Serial.println("╚════════════════════════════════════════════════════════╝\n");
  
  Serial.println("[INFO] Este proceso tomará ~10 minutos");
  Serial.println("[INFO] Asegúrese de que el reservorio tenga suficiente agua");
  Serial.println();
  
  delay(2000);
  
  // Resetear volumen
  totalVolume = 0;
  
  // Barrido completo: 0 a 255, pasos de 25, esperar 20s por punto
  Serial.println("[PHASE 1] Barrido ascendente PWM 0→255");
  pwmSweep(0, 255, 25, 20);
  
  delay(5000);
  
  // Barrido descendente
  Serial.println("\n[PHASE 2] Barrido descendente PWM 255→0");
  pwmSweep(255, 0, -25, 15);
  
  Serial.println("\n╔════════════════════════════════════════════════════════╗");
  Serial.println("║     CARACTERIZACIÓN COMPLETADA                         ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println("\n[NEXT] Exportar datos a MATLAB para identificación");
  Serial.println("[NEXT] Determinar función de transferencia F(s)");
  Serial.println("[NEXT] Diseñar controlador PID (Sprint 4)\n");
}

// ============================================================================
// PROCESAMIENTO DE COMANDOS
// ============================================================================

void processSerialCommands() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    if (inChar == '\n' || inChar == '\r') {
      if (inputCommand.length() > 0) {
        commandComplete = true;
      }
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
  // SETPWM,<valor>
  if (cmd.startsWith("SETPWM,")) {
    int pwm = cmd.substring(7).toInt();
    setMotorPWM(pwm);
  }
  
  // SWEEP,<start>,<end>,<step>,<delay>
  else if (cmd.startsWith("SWEEP,")) {
    int idx1 = cmd.indexOf(',', 6);
    int idx2 = cmd.indexOf(',', idx1 + 1);
    int idx3 = cmd.indexOf(',', idx2 + 1);
    
    if (idx1 > 0 && idx2 > 0 && idx3 > 0) {
      int start = cmd.substring(6, idx1).toInt();
      int end = cmd.substring(idx1 + 1, idx2).toInt();
      int step = cmd.substring(idx2 + 1, idx3).toInt();
      int delayS = cmd.substring(idx3 + 1).toInt();
      
      if (step == 0) step = 10;
      if (delayS == 0) delayS = 10;
      
      pwmSweep(start, end, step, delayS);
    } else {
      Serial.println("[ERROR] Formato: SWEEP,start,end,step,delay");
      Serial.println("[EJEMPLO] SWEEP,0,200,25,15");
    }
  }
  
  // LOGDATA
  else if (cmd == "LOGDATA") {
    loggingEnabled = true;
    logCounter = 0;
    lastLogTime = millis();
    Serial.println("[LOG] Logging iniciado (cada 2s)");
    Serial.println("[INFO] Use STOPLOG para detener");
  }
  
  // STOPLOG
  else if (cmd == "STOPLOG") {
    loggingEnabled = false;
    Serial.println("[LOG] Logging detenido");
  }
  
  // CHARACTERIZE
  else if (cmd == "CHARACTERIZE") {
    runCharacterization();
  }
  
  // ALLDATA
  else if (cmd == "ALLDATA") {
    readAllSensors();
    printAllData();
  }
  
  // STOP
  else if (cmd == "STOP") {
    stopMotor();
    loggingEnabled = false;
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
    Serial.println("[ERROR] Comando no reconocido");
    Serial.println("[INFO] Escriba HELP para ver comandos");
  }
}

// ============================================================================
// FUNCIONES DE INFORMACIÓN
// ============================================================================

void printAllData() {
  Serial.println("\n──────── DATOS DEL SISTEMA ────────");
  Serial.print("Motor PWM: ");
  Serial.print(currentPWM);
  Serial.print(" (");
  Serial.print((currentPWM * 100.0) / PWM_MAX, 1);
  Serial.println("%)");
  
  Serial.print("Flujo: ");
  Serial.print(flowRate, 3);
  Serial.println(" L/min");
  
  Serial.print("Volumen: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  
  Serial.print("Nivel Tank 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  
  Serial.print("Nivel Tank 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  Serial.println("───────────────────────────────────\n");
}

void printStatus() {
  readAllSensors();
  
  Serial.println("\n========== ESTADO DEL SISTEMA ==========");
  
  Serial.println("\n[MOTOR]");
  Serial.print("  Estado: ");
  Serial.println(motorRunning ? "ACTIVO" : "DETENIDO");
  Serial.print("  PWM: ");
  Serial.print(currentPWM);
  Serial.print(" / 255 (");
  Serial.print((currentPWM * 100.0) / 255, 1);
  Serial.println("%)");
  Serial.print("  Voltaje estimado: ");
  Serial.print((currentPWM * 12.0) / 255, 2);
  Serial.println(" V");
  
  Serial.println("\n[SENSORES]");
  Serial.print("  Flujo: ");
  Serial.print(flowRate, 3);
  Serial.println(" L/min");
  Serial.print("  Volumen total: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  Serial.print("  Nivel Tank 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  Serial.print("  Nivel Tank 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  
  Serial.println("\n[LOGGING]");
  Serial.print("  Estado: ");
  Serial.println(loggingEnabled ? "ACTIVO" : "INACTIVO");
  if (loggingEnabled) {
    Serial.print("  Muestras: ");
    Serial.println(logCounter);
  }
  
  Serial.print("\n  Tiempo activo: ");
  Serial.print(millis() / 1000);
  Serial.println(" s");
  
  Serial.println("========================================\n");
}

void printHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║      COMANDOS - INTEGRACIÓN SENSOR-ACTUADOR              ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  SETPWM,<val>         - Establecer PWM motor (0-255)     ║");
  Serial.println("║  SWEEP,<s>,<e>,<st>,<d> - Barrido PWM automático         ║");
  Serial.println("║  LOGDATA              - Iniciar logging (CSV)            ║");
  Serial.println("║  STOPLOG              - Detener logging                  ║");
  Serial.println("║  CHARACTERIZE         - Caracterización completa (~10min)||");
  Serial.println("║  ALLDATA              - Ver todos los datos              ║");
  Serial.println("║  STOP                 - Detener motor y logging          ║");
  Serial.println("║  STATUS               - Estado completo                  ║");
  Serial.println("║  HELP                 - Mostrar esta ayuda               ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  Ejemplos:                                               ║");
  Serial.println("║    SETPWM,150         → Motor al 59% (PWM=150)           ║");
  Serial.println("║    SWEEP,0,200,25,15  → Barrido 0→200, paso 25, 15s/pt  ║");
  Serial.println("║    CHARACTERIZE       → Proceso automático completo      ║");
  Serial.println("║    LOGDATA            → Grabar datos continuos           ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// FIN DEL PROGRAMA
// ============================================================================
