/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE FLUJO HIDRÁULICO
 * Universidad Militar Nueva Granada
 * Ingeniería Mecatrónica - Control Lineal y Laboratorio
 * ============================================================================
 * 
 * SPRINT 2: LECTURA DE SENSORES
 * 
 * Descripción:
 *   Lectura de sensor de flujo YF-S401 (pulsos) y sensores de nivel SE045 (analógico).
 *   Nota: Sprint 9 es la referencia vigente del pinout integrado; este sketch
 *   conserva la prueba histórica SE045/ADC.
 *   con procesamiento, calibración y transmisión de datos vía UART.
 * 
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - Sensor de flujo YF-S401 (hall effect, 0.3-6 L/min)
 *   - 2x Sensor de nivel SE045 (analogico, 0-40mm, historico)
 *   - Divisores resistivos 10kΩ/22kΩ para protección ADC
 * 
 * Conexiones historicas de este sketch. Referencia vigente de sistema: Sprint 9.
 *   GPIO 4  → YF-S401 Signal (interrupción)
 *   GPIO 5  → SE045 Tank1 (ADC historico; Sprint 9: HC-SR04 T1 TRIG)
 *   GPIO 6  → SE045 Tank2 (ADC historico; Sprint 9: HC-SR04 T1 ECHO)
 *   GPIO 7  → LED status
 * 
 * Pines disponibles segun referencia Sprint 9: 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 46
 * 
 * Comandos UART (115200 baud):
 *   FLOWRAW          - Lectura cruda del sensor de flujo (pulsos/s)
 *   FLOWLPM          - Flujo en litros por minuto
 *   LEVEL1           - Nivel tanque 1 (mm)
 *   LEVEL2           - Nivel tanque 2 (mm)
 *   ALLSENSORS       - Lectura de todos los sensores
 *   STREAM,<ms>      - Streaming continuo cada <ms> milisegundos
 *   STOPSTREAM       - Detener streaming
 *   CALIBFLOW,<K>    - Calibrar factor K del sensor de flujo
 *   CALIBLEVEL1,<m>,<b> - Calibrar nivel 1 (ADC = m*altura + b)
 *   CALIBLEVEL2,<m>,<b> - Calibrar nivel 2
 *   RESETVOL         - Resetear volumen acumulado
 *   STATUS           - Estado del sistema
 *   HELP             - Listar comandos
 * 
 * Autor: Daniel García Araque, David Santiago García Suarez
 * Fecha: Febrero 2026
 * Versión: 1.0
 * ============================================================================
 */

// ============================================================================
// DEFINICIÓN DE PINES
// ============================================================================
// RESTRICCION: Sprint 9 es la referencia vigente de pinout.
// Solo usar GPIOs 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 46

// Pines sensores
#define FLOW_SENSOR_PIN    4     // YF-S401 pulsos (interrupción)
#define LEVEL1_SENSOR_PIN  5     // SE045 historico Tank 1 (Sprint 9: HC-SR04 T1 TRIG)
#define LEVEL2_SENSOR_PIN  6     // SE045 historico Tank 2 (Sprint 9: HC-SR04 T1 ECHO)

// Pines indicadores
#define LED_STATUS_PIN     7     // LED status externo

// ============================================================================
// CONFIGURACIÓN ADC
// ============================================================================

#define ADC_RESOLUTION     4095  // 12 bits (0-4095)
#define ADC_VREF           3.3   // Voltaje referencia ESP32 (V)
#define ADC_SAMPLES        10    // Muestras para promedio

// ============================================================================
// PARÁMETROS SENSOR DE FLUJO YF-S401
// ============================================================================

// Factor de calibración: pulsos por litro
// Valor nominal: 5880 pulsos/L = 98 pulsos/(L/min)
float flowCalibrationFactor = 98.0;  // Pulsos por L/min

// Variables de flujo
volatile unsigned long flowPulseCount = 0;  // Contador de pulsos (volátil para ISR)
float flowRate = 0.0;                        // Flujo instantáneo (L/min)
float totalVolume = 0.0;                     // Volumen acumulado (L)
unsigned long lastFlowUpdate = 0;
const unsigned long flowUpdateInterval = 1000;  // Actualizar cada 1 segundo

// ============================================================================
// PARÁMETROS SENSORES DE NIVEL SE045
// ============================================================================

// Divisor resistivo: R1=10kΩ, R2=22kΩ
// Voltaje máximo sensor: 4.5V
// Voltaje ADC máximo: 3.3V
const float VOLTAGE_DIVIDER_FACTOR = 0.6875;  // R2/(R1+R2) = 22k/(10k+22k)

// Rango de medición
const float SENSOR_MAX_VOLTAGE = 4.5;  // Voltaje máximo del sensor (V)
const float SENSOR_MAX_HEIGHT = 40.0;  // Altura máxima medible (mm)

// NOTA IMPORTANTE: Valores típicos observados experimentalmente:
// - Sensor SECO (fuera del agua): lectura ~37-45 mm (debido a resistencia base)
// - Sensor MOJADO (sumergido): aumenta proporcionalmente con profundidad
// - Para calibración precisa, usar CALIBLEVEL1/CALIBLEVEL2 con datos reales

// Calibración lineal: ADC = m * altura + b
// Valores por defecto (ajustar con calibración experimental)
float level1_slope = (ADC_RESOLUTION * VOLTAGE_DIVIDER_FACTOR) / SENSOR_MAX_HEIGHT;
float level1_offset = 0;
float level2_slope = (ADC_RESOLUTION * VOLTAGE_DIVIDER_FACTOR) / SENSOR_MAX_HEIGHT;
float level2_offset = 0;

// Variables de nivel
float waterLevel1 = 0.0;  // Nivel tanque 1 (mm)
float waterLevel2 = 0.0;  // Nivel tanque 2 (mm)

// ============================================================================
// FILTRADO Y PROCESAMIENTO
// ============================================================================

// Filtro paso bajo simple (media móvil)
const int FILTER_SIZE = 5;
float level1_buffer[FILTER_SIZE] = {0};
float level2_buffer[FILTER_SIZE] = {0};
int filter_index = 0;

// ============================================================================
// STREAMING DE DATOS
// ============================================================================

bool streamingEnabled = false;
unsigned long streamInterval = 500;  // ms
unsigned long lastStreamTime = 0;

// ============================================================================
// COMUNICACIÓN UART
// ============================================================================

String inputCommand = "";
bool commandComplete = false;

// ============================================================================
// INTERRUPCIÓN SENSOR DE FLUJO
// ============================================================================

/**
 * ISR para contar pulsos del sensor de flujo
 * IRAM_ATTR: coloca función en RAM para ejecución rápida
 */
void IRAM_ATTR flowPulseCounter() {
  flowPulseCount++;
}

// ============================================================================
// CONFIGURACIÓN INICIAL
// ============================================================================

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n\n");
  Serial.println("============================================================");
  Serial.println("  LABORATORIO 2 - LECTURA DE SENSORES");
  Serial.println("  Universidad Militar Nueva Granada");
  Serial.println("  ESP32-S3 - Sprint 2: Sensor Reading");
  Serial.println("============================================================");
  Serial.println();
  
  // Configurar pines
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);
  
  // Configurar interrupción para sensor de flujo
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
  
  // Configurar ADC
  analogReadResolution(12);  // 12 bits
  
  Serial.println("[OK] Pines configurados");
  Serial.println("[OK] ADC inicializado (12 bits)");
  Serial.println("[OK] Interrupción de flujo habilitada");
  Serial.println("[READY] Sistema listo para lectura de sensores");
  Serial.println();
  
  printHelp();
  
  // Indicador visual: sistema listo
  blinkLED(LED_STATUS_PIN, 3, 200);
}

// ============================================================================
// BUCLE PRINCIPAL
// ============================================================================

void loop() {
  // Procesar comandos UART
  processSerialCommands();
  
  // Actualizar flujo periódicamente
  if (millis() - lastFlowUpdate >= flowUpdateInterval) {
    updateFlowRate();
    lastFlowUpdate = millis();
  }
  
  // Streaming de datos si está habilitado
  if (streamingEnabled && (millis() - lastStreamTime >= streamInterval)) {
    streamSensorData();
    lastStreamTime = millis();
  }
  
  // Indicador visual
  digitalWrite(LED_STATUS_PIN, (millis() / 1000) % 2);
  
  delay(10);
}

// ============================================================================
// FUNCIONES DE SENSORES
// ============================================================================

/**
 * Actualizar cálculo de flujo (llamar cada 1 segundo)
 */
void updateFlowRate() {
  // Desactivar interrupciones temporalmente
  detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
  
  // Calcular frecuencia de pulsos (pulsos/segundo)
  float frequency = flowPulseCount / (flowUpdateInterval / 1000.0);
  
  // Calcular flujo en L/min
  // flowRate (L/min) = frequency (Hz) / (calibrationFactor / 60)
  flowRate = frequency / (flowCalibrationFactor / 60.0);
  
  // Actualizar volumen acumulado
  totalVolume += (flowRate / 60.0) * (flowUpdateInterval / 1000.0);  // Litros
  
  // Resetear contador
  flowPulseCount = 0;
  
  // Reactivar interrupción
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulseCounter, RISING);
}

/**
 * Leer nivel de agua Tank 1 (con filtrado)
 * @return Altura en mm
 */
float readWaterLevel1() {
  // Leer ADC con promedio de muestras
  long adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(LEVEL1_SENSOR_PIN);
    delayMicroseconds(100);
  }
  float adcValue = adcSum / (float)ADC_SAMPLES;
  
  // Convertir ADC a voltaje en el pin (después del divisor)
  float voltage_pin = (adcValue / ADC_RESOLUTION) * ADC_VREF;
  
  // Compensar divisor resistivo
  float voltage_sensor = voltage_pin / VOLTAGE_DIVIDER_FACTOR;
  
  // Convertir a altura usando calibración lineal
  float height = (voltage_sensor / SENSOR_MAX_VOLTAGE) * SENSOR_MAX_HEIGHT;
  
  // Aplicar filtro de media móvil
  level1_buffer[filter_index] = height;
  filter_index = (filter_index + 1) % FILTER_SIZE;
  
  float filtered_height = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    filtered_height += level1_buffer[i];
  }
  filtered_height /= FILTER_SIZE;
  
  return filtered_height;
}

/**
 * Leer nivel de agua Tank 2 (con filtrado)
 * @return Altura en mm
 */
float readWaterLevel2() {
  // Leer ADC con promedio
  long adcSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    adcSum += analogRead(LEVEL2_SENSOR_PIN);
    delayMicroseconds(100);
  }
  float adcValue = adcSum / (float)ADC_SAMPLES;
  
  // Convertir ADC a voltaje
  float voltage_pin = (adcValue / ADC_RESOLUTION) * ADC_VREF;
  float voltage_sensor = voltage_pin / VOLTAGE_DIVIDER_FACTOR;
  
  // Convertir a altura
  float height = (voltage_sensor / SENSOR_MAX_VOLTAGE) * SENSOR_MAX_HEIGHT;
  
  // Aplicar filtro
  level2_buffer[filter_index] = height;
  
  float filtered_height = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    filtered_height += level2_buffer[i];
  }
  filtered_height /= FILTER_SIZE;
  
  return filtered_height;
}

/**
 * Leer todos los sensores
 */
void readAllSensors() {
  waterLevel1 = readWaterLevel1();
  waterLevel2 = readWaterLevel2();
  // flowRate ya se actualiza automáticamente cada segundo
}

// ============================================================================
// STREAMING Y VISUALIZACIÓN
// ============================================================================

/**
 * Transmitir datos de sensores (formato Serial Plotter)
 */
void streamSensorData() {
  readAllSensors();
  
  // Formato para Serial Plotter (separado por tabs)
  Serial.print("Flow:");
  Serial.print(flowRate, 3);
  Serial.print("\t");
  
  Serial.print("Level1:");
  Serial.print(waterLevel1, 2);
  Serial.print("\t");
  
  Serial.print("Level2:");
  Serial.print(waterLevel2, 2);
  Serial.println();
}

/**
 * Mostrar datos de sensores (formato legible)
 */
void printSensorData() {
  readAllSensors();
  
  Serial.println("\n--- LECTURA DE SENSORES ---");
  
  Serial.print("Flujo: ");
  Serial.print(flowRate, 3);
  Serial.println(" L/min");
  
  Serial.print("Volumen total: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  
  Serial.print("Nivel Tanque 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  
  Serial.print("Nivel Tanque 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  
  Serial.println("---------------------------\n");
}

// ============================================================================
// PROCESAMIENTO DE COMANDOS UART
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
    
    Serial.print("\n[CMD] Recibido: ");
    Serial.println(inputCommand);
    
    parseCommand(inputCommand);
    
    inputCommand = "";
    commandComplete = false;
  }
}

void parseCommand(String cmd) {
  // FLOWRAW - Pulsos por segundo
  if (cmd == "FLOWRAW") {
    Serial.print("Pulsos/segundo: ");
    Serial.println(flowPulseCount);
  }
  
  // FLOWLPM - Flujo en litros por minuto
  else if (cmd == "FLOWLPM") {
    Serial.print("Flujo: ");
    Serial.print(flowRate, 3);
    Serial.println(" L/min");
  }
  
  // LEVEL1 - Nivel tanque 1
  else if (cmd == "LEVEL1") {
    waterLevel1 = readWaterLevel1();
    Serial.print("Nivel Tanque 1: ");
    Serial.print(waterLevel1, 2);
    Serial.println(" mm");
    Serial.println("[INFO] Lectura típica: ~37-45mm seco, aumenta con agua");
  }
  
  // LEVEL2 - Nivel tanque 2
  else if (cmd == "LEVEL2") {
    waterLevel2 = readWaterLevel2();
    Serial.print("Nivel Tanque 2: ");
    Serial.print(waterLevel2, 2);
    Serial.println(" mm");
    Serial.println("[INFO] Lectura típica: ~37-45mm seco, aumenta con agua");
  }
  
  // ALLSENSORS - Todos los sensores
  else if (cmd == "ALLSENSORS") {
    printSensorData();
  }
  
  // STREAM,<ms> - Iniciar streaming
  else if (cmd.startsWith("STREAM,")) {
    int interval = cmd.substring(7).toInt();
    if (interval < 100) interval = 100;  // Mínimo 100ms
    streamInterval = interval;
    streamingEnabled = true;
    Serial.print("[STREAM] Iniciado cada ");
    Serial.print(streamInterval);
    Serial.println(" ms");
    Serial.println("[INFO] Abrir Serial Plotter para visualización");
  }
  
  // STOPSTREAM - Detener streaming
  else if (cmd == "STOPSTREAM") {
    streamingEnabled = false;
    Serial.println("[STREAM] Detenido");
  }
  
  // CALIBFLOW,<K> - Calibrar sensor de flujo
  else if (cmd.startsWith("CALIBFLOW,")) {
    float newK = cmd.substring(10).toFloat();
    if (newK > 0) {
      flowCalibrationFactor = newK;
      Serial.print("[CALIB] Factor K actualizado: ");
      Serial.println(flowCalibrationFactor, 2);
    } else {
      Serial.println("[ERROR] Factor K inválido");
    }
  }
  
  // RESETVOL - Resetear volumen
  else if (cmd == "RESETVOL") {
    totalVolume = 0;
    Serial.println("[RESET] Volumen acumulado = 0 L");
  }
  
  // STATUS - Estado
  else if (cmd == "STATUS") {
    printStatus();
  }
  
  // HELP - Ayuda
  else if (cmd == "HELP") {
    printHelp();
  }
  
  // Comando desconocido
  else {
    Serial.println("[ERROR] Comando no reconocido");
    Serial.println("[INFO] Escriba HELP para ver comandos");
  }
}

// ============================================================================
// FUNCIONES DE INFORMACIÓN
// ============================================================================

void printStatus() {
  readAllSensors();
  
  Serial.println("\n========== ESTADO DEL SISTEMA ==========");
  
  // Sensor de flujo
  Serial.println("\n[SENSOR DE FLUJO YF-S401]");
  Serial.print("  Flujo actual: ");
  Serial.print(flowRate, 3);
  Serial.println(" L/min");
  Serial.print("  Volumen total: ");
  Serial.print(totalVolume, 2);
  Serial.println(" L");
  Serial.print("  Factor K: ");
  Serial.print(flowCalibrationFactor, 2);
  Serial.println(" pulsos/(L/min)");
  Serial.print("  Pulsos/seg: ");
  Serial.println(flowPulseCount);
  
  // Sensores de nivel
  Serial.println("\n[SENSORES DE NIVEL SE045]");
  Serial.print("  Tanque 1: ");
  Serial.print(waterLevel1, 2);
  Serial.println(" mm");
  Serial.print("  Tanque 2: ");
  Serial.print(waterLevel2, 2);
  Serial.println(" mm");
  
  // Configuración
  Serial.println("\n[CONFIGURACIÓN]");
  Serial.print("  Streaming: ");
  Serial.println(streamingEnabled ? "ACTIVO" : "INACTIVO");
  if (streamingEnabled) {
    Serial.print("  Intervalo: ");
    Serial.print(streamInterval);
    Serial.println(" ms");
  }
  Serial.print("  ADC resolución: ");
  Serial.print(12);
  Serial.println(" bits");
  Serial.print("  Filtro promedio: ");
  Serial.print(FILTER_SIZE);
  Serial.println(" muestras");
  
  Serial.print("\n  Tiempo activo: ");
  Serial.print(millis() / 1000);
  Serial.println(" s");
  
  Serial.println("========================================\n");
}

void printHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║       COMANDOS DISPONIBLES - SENSOR READING              ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  FLOWRAW          - Pulsos/segundo del sensor de flujo   ║");
  Serial.println("║  FLOWLPM          - Flujo en litros por minuto           ║");
  Serial.println("║  LEVEL1           - Nivel de agua Tanque 1 (mm)          ║");
  Serial.println("║  LEVEL2           - Nivel de agua Tanque 2 (mm)          ║");
  Serial.println("║  ALLSENSORS       - Lectura de todos los sensores        ║");
  Serial.println("║  STREAM,<ms>      - Iniciar streaming (ej: STREAM,500)   ║");
  Serial.println("║  STOPSTREAM       - Detener streaming                    ║");
  Serial.println("║  CALIBFLOW,<K>    - Calibrar factor K (ej: 98.0)         ║");
  Serial.println("║  RESETVOL         - Resetear volumen acumulado           ║");
  Serial.println("║  STATUS           - Mostrar estado completo              ║");
  Serial.println("║  HELP             - Mostrar esta ayuda                   ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  Ejemplos:                                               ║");
  Serial.println("║    ALLSENSORS     → Ver todas las lecturas               ║");
  Serial.println("║    STREAM,500     → Streaming cada 500ms (Serial Plotter)║");
  Serial.println("║    CALIBFLOW,95.5 → Ajustar calibración flujo            ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

void blinkLED(int pin, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
  }
}

// ============================================================================
// FIN DEL PROGRAMA
// ============================================================================
