/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE FLUJO HIDRÁULICO
 * Universidad Militar Nueva Granada
 * Ingeniería Mecatrónica - Control Lineal y Laboratorio
 * ============================================================================
 * 
 * SPRINT 1: CONTROL DE MOTOR DC CON H-BRIDGE
 * 
 * Descripción:
 *   Control de bomba peristáltica 12V mediante H-Bridge (L298N/TB6612)
 *   con interfaz UART para comandos y monitoreo en tiempo real.
 * 
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - H-Bridge L298N o TB6612FNG
 *   - Motor DC 12V (bomba peristáltica)
 *   - Fuente 12V/2A
 * 
 * Conexiones:
 *   GPIO 17 → ENA (Enable/PWM)
 *   GPIO 15 → IN1 (Dirección)
 *   GPIO 16 → IN2 (Dirección)
 *   GPIO 7  → LED status
 * 
 * Pines disponibles: 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 46
 * 
 * Comandos UART (115200 baud):
 *   PWM,<valor>      - Establecer PWM (0-255)
 *   DIR,<dir>        - Cambiar dirección (0=adelante, 1=reversa)
 *   SPEED,<pct>      - Velocidad en porcentaje (0-100)
 *   STOP             - Detener motor inmediatamente
 *   RAMP,<target>    - Rampa suave a velocidad objetivo
 *   STATUS           - Mostrar estado actual
 *   HELP             - Listar comandos disponibles
 * 
 * Autor: Daniel García Araque, David Santiago García Suarez
 * Fecha: Febrero 2026
 * Versión: 1.0
 * ============================================================================
 */

// ============================================================================
// DEFINICIÓN DE PINES
// ============================================================================
// RESTRICCIÓN: Solo usar GPIOs 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 46

// Pines control H-Bridge
#define MOTOR_PWM_PIN    17    // Pin PWM (ENA en L298N)
#define MOTOR_IN1_PIN    15    // Control dirección 1
#define MOTOR_IN2_PIN    16    // Control dirección 2

// Pines indicadores
#define LED_STATUS_PIN   7     // LED status externo
#define LED_ERROR_PIN    8     // LED externo error (opcional)

// ============================================================================
// CONFIGURACIÓN PWM (LEDC - LED Controller)
// ============================================================================

#define PWM_CHANNEL      0     // Canal LEDC (0-15 disponibles)
#define PWM_FREQUENCY    10000 // 10 kHz (óptimo para motores DC)
#define PWM_RESOLUTION   8     // 8 bits = 0-255

// ============================================================================
// PARÁMETROS DE SEGURIDAD Y LÍMITES
// ============================================================================

#define PWM_MIN          0     // PWM mínimo (motor detenido)
#define PWM_MAX          255   // PWM máximo (100% duty cycle)
#define PWM_STARTUP_MIN  50    // PWM mínimo para arranque (evita carga muerta)
#define PWM_EMERGENCY    0     // PWM en caso de emergencia

#define RAMP_STEP        5     // Incremento PWM por paso en rampa
#define RAMP_DELAY_MS    50    // Delay entre pasos de rampa (ms)

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Estado del motor
int currentPWM = 0;            // Valor PWM actual (0-255)
int targetPWM = 0;             // Valor PWM objetivo (para rampas)
bool motorDirection = 0;       // 0 = adelante, 1 = reversa
bool motorRunning = false;     // Estado motor (on/off)
bool emergencyStop = false;    // Bandera de paro de emergencia

// Comunicación UART
String inputCommand = "";      // Buffer comando serial
bool commandComplete = false;  // Bandera comando completo

// Timing
unsigned long lastStatusUpdate = 0;
unsigned long statusUpdateInterval = 1000; // Actualización cada 1s

// ============================================================================
// CONFIGURACIÓN INICIAL
// ============================================================================

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  delay(500);  // Esperar estabilización
  
  Serial.println("\n\n");
  Serial.println("============================================================");
  Serial.println("  LABORATORIO 2 - CONTROL DE MOTOR DC");
  Serial.println("  Universidad Militar Nueva Granada");
  Serial.println("  ESP32-S3 - Sprint 1: H-Bridge Control");
  Serial.println("============================================================");
  Serial.println();
  
  // Configurar pines de salida
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  pinMode(LED_ERROR_PIN, OUTPUT);
  
  // Estado inicial seguro: motor detenido
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  digitalWrite(LED_STATUS_PIN, LOW);
  digitalWrite(LED_ERROR_PIN, LOW);
  
  // Configurar PWM usando LEDC
  setupPWM();
  
  // Mensaje de inicio
  Serial.println("[OK] Pines configurados");
  Serial.println("[OK] PWM inicializado");
  Serial.println("[READY] Sistema listo");
  Serial.println();
  
  printHelp();
  printStatus();
  
  // Indicador visual: sistema listo
  blinkLED(LED_STATUS_PIN, 3, 200);
}

// ============================================================================
// BUCLE PRINCIPAL
// ============================================================================

void loop() {
  // Procesar comandos UART
  processSerialCommands();
  
  // Actualizar estado periódicamente
  if (millis() - lastStatusUpdate >= statusUpdateInterval) {
    // printStatus();  // Comentado para reducir spam, descomentar si necesario
    lastStatusUpdate = millis();
  }
  
  // Indicador visual: parpadeo LED si motor activo
  if (motorRunning && currentPWM > 0) {
    digitalWrite(LED_STATUS_PIN, (millis() / 500) % 2);  // Parpadeo 1 Hz
  } else {
    digitalWrite(LED_STATUS_PIN, LOW);
  }
  
  // Pequeño delay para evitar saturar CPU
  delay(10);
}

// ============================================================================
// CONFIGURACIÓN PWM (LEDC)
// ============================================================================

void setupPWM() {
  // Configurar canal LEDC
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  
  // Asociar pin al canal
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
  
  // Iniciar con PWM = 0 (motor detenido)
  ledcWrite(PWM_CHANNEL, 0);
  
  Serial.println("[PWM] Configurado:");
  Serial.print("  - Frecuencia: ");
  Serial.print(PWM_FREQUENCY);
  Serial.println(" Hz");
  Serial.print("  - Resolución: ");
  Serial.print(PWM_RESOLUTION);
  Serial.println(" bits");
  Serial.print("  - Rango: 0-");
  Serial.println(PWM_MAX);
}

// ============================================================================
// FUNCIONES DE CONTROL DEL MOTOR
// ============================================================================

/**
 * Establecer velocidad del motor (PWM directo)
 * @param pwmValue: Valor PWM (0-255)
 */
void setMotorPWM(int pwmValue) {
  // Validar rango
  if (pwmValue < PWM_MIN) pwmValue = PWM_MIN;
  if (pwmValue > PWM_MAX) pwmValue = PWM_MAX;
  
  // Aplicar PWM
  ledcWrite(PWM_CHANNEL, pwmValue);
  currentPWM = pwmValue;
  
  // Actualizar estado
  motorRunning = (pwmValue > 0);
  
  Serial.print("[MOTOR] PWM establecido: ");
  Serial.print(pwmValue);
  Serial.print(" (");
  Serial.print((pwmValue * 100.0) / PWM_MAX, 1);
  Serial.println("%)");
}

/**
 * Establecer velocidad por porcentaje (0-100%)
 * @param percentage: Porcentaje de velocidad (0-100)
 */
void setMotorSpeed(float percentage) {
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  int pwmValue = (int)((percentage / 100.0) * PWM_MAX);
  
  setMotorPWM(pwmValue);
}

/**
 * Cambiar dirección del motor
 * @param direction: 0 = adelante, 1 = reversa
 */
void setMotorDirection(bool direction) {
  motorDirection = direction;
  
  if (direction == 0) {
    // Dirección adelante (normal)
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    Serial.println("[DIR] Dirección: ADELANTE");
  } else {
    // Dirección reversa
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);
    Serial.println("[DIR] Dirección: REVERSA");
  }
}

/**
 * Detener motor inmediatamente (paro de emergencia)
 */
void stopMotor() {
  setMotorPWM(0);
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
  motorRunning = false;
  
  Serial.println("[STOP] Motor detenido");
}

/**
 * Rampa suave de velocidad (evita arranque brusco)
 * @param targetValue: PWM objetivo
 */
void rampMotorPWM(int targetValue) {
  // Validar rango
  if (targetValue < PWM_MIN) targetValue = PWM_MIN;
  if (targetValue > PWM_MAX) targetValue = PWM_MAX;
  
  Serial.print("[RAMP] Iniciando rampa desde ");
  Serial.print(currentPWM);
  Serial.print(" hasta ");
  Serial.println(targetValue);
  
  // Rampa ascendente
  if (targetValue > currentPWM) {
    for (int pwm = currentPWM; pwm <= targetValue; pwm += RAMP_STEP) {
      setMotorPWM(pwm);
      delay(RAMP_DELAY_MS);
    }
  }
  // Rampa descendente
  else if (targetValue < currentPWM) {
    for (int pwm = currentPWM; pwm >= targetValue; pwm -= RAMP_STEP) {
      setMotorPWM(pwm);
      delay(RAMP_DELAY_MS);
    }
  }
  
  // Asegurar valor final exacto
  setMotorPWM(targetValue);
  
  Serial.println("[RAMP] Rampa completada");
}

// ============================================================================
// PROCESAMIENTO DE COMANDOS UART
// ============================================================================

/**
 * Procesar comandos recibidos por Serial
 */
void processSerialCommands() {
  // Leer datos disponibles
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    // Agregar a buffer
    if (inChar == '\n' || inChar == '\r') {
      if (inputCommand.length() > 0) {
        commandComplete = true;
      }
    } else {
      inputCommand += inChar;
    }
  }
  
  // Procesar comando completo
  if (commandComplete) {
    inputCommand.trim();  // Eliminar espacios
    inputCommand.toUpperCase();  // Convertir a mayúsculas
    
    Serial.print("\n[CMD] Recibido: ");
    Serial.println(inputCommand);
    
    parseCommand(inputCommand);
    
    // Limpiar buffer
    inputCommand = "";
    commandComplete = false;
  }
}

/**
 * Interpretar y ejecutar comando
 * @param cmd: Comando recibido
 */
void parseCommand(String cmd) {
  // Comando: PWM,<valor>
  if (cmd.startsWith("PWM,")) {
    int value = cmd.substring(4).toInt();
    setMotorPWM(value);
  }
  
  // Comando: DIR,<dirección>
  else if (cmd.startsWith("DIR,")) {
    int dir = cmd.substring(4).toInt();
    setMotorDirection(dir);
  }
  
  // Comando: SPEED,<porcentaje>
  else if (cmd.startsWith("SPEED,")) {
    float pct = cmd.substring(6).toFloat();
    setMotorSpeed(pct);
  }
  
  // Comando: RAMP,<valor>
  else if (cmd.startsWith("RAMP,")) {
    int target = cmd.substring(5).toInt();
    rampMotorPWM(target);
  }
  
  // Comando: STOP
  else if (cmd == "STOP") {
    stopMotor();
  }
  
  // Comando: STATUS
  else if (cmd == "STATUS") {
    printStatus();
  }
  
  // Comando: HELP
  else if (cmd == "HELP") {
    printHelp();
  }
  
  // Comando: TEST (prueba automática)
  else if (cmd == "TEST") {
    runMotorTest();
  }
  
  // Comando desconocido
  else {
    Serial.println("[ERROR] Comando no reconocido");
    Serial.println("[INFO] Escriba HELP para ver comandos disponibles");
  }
}

// ============================================================================
// FUNCIONES DE INFORMACIÓN Y DEBUG
// ============================================================================

/**
 * Mostrar estado actual del sistema
 */
void printStatus() {
  Serial.println("\n--- ESTADO DEL SISTEMA ---");
  Serial.print("Motor: ");
  Serial.println(motorRunning ? "ACTIVO" : "DETENIDO");
  Serial.print("PWM: ");
  Serial.print(currentPWM);
  Serial.print(" / ");
  Serial.print(PWM_MAX);
  Serial.print(" (");
  Serial.print((currentPWM * 100.0) / PWM_MAX, 1);
  Serial.println("%)");
  Serial.print("Dirección: ");
  Serial.println(motorDirection ? "REVERSA" : "ADELANTE");
  Serial.print("Voltaje estimado: ");
  Serial.print((currentPWM * 12.0) / PWM_MAX, 2);
  Serial.println(" V");
  Serial.print("Tiempo activo: ");
  Serial.print(millis() / 1000);
  Serial.println(" s");
  Serial.println("-------------------------\n");
}

/**
 * Mostrar comandos disponibles
 */
void printHelp() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║          COMANDOS DISPONIBLES - MOTOR CONTROL            ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  PWM,<valor>      - Establecer PWM directo (0-255)       ║");
  Serial.println("║  SPEED,<pct>      - Velocidad en porcentaje (0-100)      ║");
  Serial.println("║  DIR,<dir>        - Dirección (0=adelante, 1=reversa)    ║");
  Serial.println("║  RAMP,<valor>     - Rampa suave a PWM objetivo           ║");
  Serial.println("║  STOP             - Detener motor inmediatamente         ║");
  Serial.println("║  STATUS           - Mostrar estado actual                ║");
  Serial.println("║  TEST             - Ejecutar prueba automática           ║");
  Serial.println("║  HELP             - Mostrar esta ayuda                   ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  Ejemplos:                                               ║");
  Serial.println("║    PWM,128        → PWM a 50%                            ║");
  Serial.println("║    SPEED,75       → Velocidad al 75%                     ║");
  Serial.println("║    DIR,1          → Cambiar a reversa                    ║");
  Serial.println("║    RAMP,200       → Rampa suave hasta PWM 200            ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");
}

/**
 * Ejecutar secuencia de prueba automática
 */
void runMotorTest() {
  Serial.println("\n════════════════════════════════════════");
  Serial.println("  INICIANDO PRUEBA AUTOMÁTICA DEL MOTOR");
  Serial.println("════════════════════════════════════════\n");
  
  // Test 1: Rampa ascendente
  Serial.println("[TEST 1] Rampa ascendente 0 → 100%");
  setMotorDirection(0);  // Adelante
  rampMotorPWM(255);
  delay(2000);
  
  // Test 2: Rampa descendente
  Serial.println("\n[TEST 2] Rampa descendente 100% → 0%");
  rampMotorPWM(0);
  delay(1000);
  
  // Test 3: Velocidad media
  Serial.println("\n[TEST 3] Velocidad media (50%)");
  setMotorSpeed(50);
  delay(3000);
  
  // Test 4: Cambio de dirección
  Serial.println("\n[TEST 4] Cambio a reversa");
  setMotorDirection(1);  // Reversa
  delay(3000);
  
  // Test 5: Detener
  Serial.println("\n[TEST 5] Detención");
  stopMotor();
  delay(1000);
  
  Serial.println("\n════════════════════════════════════════");
  Serial.println("  PRUEBA COMPLETADA");
  Serial.println("════════════════════════════════════════\n");
}

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * Parpadear LED (indicador visual)
 * @param pin: Pin del LED
 * @param times: Número de parpadeos
 * @param delayMs: Delay entre parpadeos (ms)
 */
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
