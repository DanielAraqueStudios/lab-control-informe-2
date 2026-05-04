/*
 * ============================================================================
 * LABORATORIO 2 - CONTROL DE SERVOMOTORES
 * Universidad Militar Nueva Granada
 * Ingenieria Mecatronica - Control Lineal y Laboratorio
 * ============================================================================
 *
 * SPRINT 7: CONTROL DUAL DE SERVOMOTORES
 *
 * Descripcion:
 *   Prueba aislada para controlar dos servomotores desde UART/USB.
 *   Este sprint usa el pinout vigente de Sprint 9 para validar las salidas de
 *   las valvulas servo antes de integrarlas al sistema completo.
 *
 * Hardware:
 *   - ESP32-S3 DevKit
 *   - 2x Servomotores 5V
 *   - Fuente externa 5V para servos
 *
 * Conexiones:
 *   GPIO 18 -> Servo 1 / valvula Tank 1 Signal
 *   GPIO 19 -> Servo 2 / valvula Tank 2 Signal
 *   GND ESP32 -> GND fuente servos
 *
 * Comandos UART (115200 baud):
 *   S1,<angle>             - Mover Servo 1 a angulo 0-180
 *   S2,<angle>             - Mover Servo 2 a angulo 0-180
 *   BOTH,<angle1>,<angle2> - Mover ambos servos
 *   DIR,<servo>,LEFT       - Mover servo a posicion izquierda
 *   DIR,<servo>,CENTER     - Mover servo a posicion centro
 *   DIR,<servo>,RIGHT      - Mover servo a posicion derecha
 *   STEP,<servo>,LEFT      - Disminuir angulo en pasos
 *   STEP,<servo>,RIGHT     - Aumentar angulo en pasos
 *   CR,<servo>,CW,<speed>  - Giro continuo horario 0-100
 *   CR,<servo>,CCW,<speed> - Giro continuo antihorario 0-100
 *   STOP,<servo>           - Parar servo continuo en pulso neutro
 *   DISABLE,<servo>        - Desactivar PWM de un servo
 *   STATUS                 - Estado actual
 *   HELP                   - Comandos disponibles
 *
 * Nota:
 *   Este sketch usa LEDC directamente para evitar depender de librerias externas.
 * ============================================================================
 */

// ============================================================================
// DEFINICION DE PINES
// ============================================================================

#define SERVO1_PIN 18
#define SERVO2_PIN 19

// ============================================================================
// CONFIGURACION SERVO PWM
// ============================================================================

#define SERVO_FREQUENCY 50
#define SERVO_RESOLUTION 16

const int SERVO_MIN_US = 500;
const int SERVO_NEUTRAL_US = 1500;
const int SERVO_MAX_US = 2500;
const int SERVO_PERIOD_US = 20000;

const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;

const int LEFT_ANGLE = 45;
const int CENTER_ANGLE = 90;
const int RIGHT_ANGLE = 135;
const int STEP_ANGLE = 10;

// ============================================================================
// ESTADO
// ============================================================================

int servo1Angle = CENTER_ANGLE;
int servo2Angle = CENTER_ANGLE;
bool servo1Enabled = true;
bool servo2Enabled = true;
String servo1Mode = "POSITION";
String servo2Mode = "POSITION";

String inputCommand = "";
bool commandComplete = false;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println("  LABORATORIO 2 - CONTROL DUAL DE SERVOMOTORES");
  Serial.println("  ESP32-S3 - Sprint 7: Servo Control");
  Serial.println("============================================================");
  Serial.println();

  ledcAttach(SERVO1_PIN, SERVO_FREQUENCY, SERVO_RESOLUTION);
  ledcAttach(SERVO2_PIN, SERVO_FREQUENCY, SERVO_RESOLUTION);

  moveServo(1, CENTER_ANGLE);
  moveServo(2, CENTER_ANGLE);

  Serial.println("[OK] Servos inicializados en posicion centro");
  Serial.println("[READY] Use HELP para comandos");
  printHelp();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  processSerialCommands();
  delay(5);
}

// ============================================================================
// CONTROL DE SERVOS
// ============================================================================

uint32_t microsecondsToDuty(int pulseUs) {
  pulseUs = constrain(pulseUs, SERVO_MIN_US, SERVO_MAX_US);
  uint32_t maxDuty = (1UL << SERVO_RESOLUTION) - 1;
  return (uint32_t)((pulseUs * (uint64_t)maxDuty) / SERVO_PERIOD_US);
}

int angleToMicroseconds(int angle) {
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  return map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_US, SERVO_MAX_US);
}

void writeServoPulse(int pin, int angle) {
  int pulseUs = angleToMicroseconds(angle);
  ledcWrite(pin, microsecondsToDuty(pulseUs));
}

void writeServoPulseUs(int pin, int pulseUs) {
  ledcWrite(pin, microsecondsToDuty(pulseUs));
}

void moveServo(int servo, int angle) {
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

  if (servo == 1) {
    servo1Angle = angle;
    servo1Enabled = true;
    servo1Mode = "POSITION";
    writeServoPulse(SERVO1_PIN, servo1Angle);
    Serial.print("[SERVO1] Angle=");
    Serial.println(servo1Angle);
  } else if (servo == 2) {
    servo2Angle = angle;
    servo2Enabled = true;
    servo2Mode = "POSITION";
    writeServoPulse(SERVO2_PIN, servo2Angle);
    Serial.print("[SERVO2] Angle=");
    Serial.println(servo2Angle);
  } else {
    Serial.println("[ERROR] Servo debe ser 1 o 2");
  }
}

void moveServoDirection(int servo, String direction) {
  if (direction == "LEFT") {
    moveServo(servo, LEFT_ANGLE);
  } else if (direction == "CENTER") {
    moveServo(servo, CENTER_ANGLE);
  } else if (direction == "RIGHT") {
    moveServo(servo, RIGHT_ANGLE);
  } else {
    Serial.println("[ERROR] Direccion invalida. Use LEFT, CENTER o RIGHT");
  }
}

void stepServo(int servo, String direction) {
  int currentAngle = (servo == 1) ? servo1Angle : servo2Angle;

  if (direction == "LEFT") {
    moveServo(servo, currentAngle - STEP_ANGLE);
  } else if (direction == "RIGHT") {
    moveServo(servo, currentAngle + STEP_ANGLE);
  } else {
    Serial.println("[ERROR] STEP usa LEFT o RIGHT");
  }
}

void moveContinuousServo(int servo, String direction, int speedPct) {
  speedPct = constrain(speedPct, 0, 100);

  int pulseUs = SERVO_NEUTRAL_US;
  int spanUs = SERVO_MAX_US - SERVO_NEUTRAL_US;

  if (direction == "CW") {
    pulseUs = SERVO_NEUTRAL_US + ((spanUs * speedPct) / 100);
  } else if (direction == "CCW") {
    pulseUs = SERVO_NEUTRAL_US - ((spanUs * speedPct) / 100);
  } else {
    Serial.println("[ERROR] Direccion continua invalida. Use CW o CCW");
    return;
  }

  if (servo == 1) {
    servo1Enabled = true;
    servo1Mode = String("CONTINUOUS_") + direction;
    writeServoPulseUs(SERVO1_PIN, pulseUs);
    Serial.print("[SERVO1] Continuous ");
    Serial.print(direction);
    Serial.print(" speed=");
    Serial.print(speedPct);
    Serial.println("%");
  } else if (servo == 2) {
    servo2Enabled = true;
    servo2Mode = String("CONTINUOUS_") + direction;
    writeServoPulseUs(SERVO2_PIN, pulseUs);
    Serial.print("[SERVO2] Continuous ");
    Serial.print(direction);
    Serial.print(" speed=");
    Serial.print(speedPct);
    Serial.println("%");
  } else {
    Serial.println("[ERROR] Servo debe ser 1 o 2");
  }
}

void stopContinuousServo(int servo) {
  if (servo == 1) {
    servo1Enabled = true;
    servo1Mode = "STOPPED";
    writeServoPulseUs(SERVO1_PIN, SERVO_NEUTRAL_US);
    Serial.println("[SERVO1] Stop/neutral");
  } else if (servo == 2) {
    servo2Enabled = true;
    servo2Mode = "STOPPED";
    writeServoPulseUs(SERVO2_PIN, SERVO_NEUTRAL_US);
    Serial.println("[SERVO2] Stop/neutral");
  } else {
    Serial.println("[ERROR] Servo debe ser 1 o 2");
  }
}

void disableServo(int servo) {
  if (servo == 1) {
    ledcWrite(SERVO1_PIN, 0);
    servo1Enabled = false;
    Serial.println("[SERVO1] PWM desactivado");
  } else if (servo == 2) {
    ledcWrite(SERVO2_PIN, 0);
    servo2Enabled = false;
    Serial.println("[SERVO2] PWM desactivado");
  } else {
    Serial.println("[ERROR] Servo debe ser 1 o 2");
  }
}

// ============================================================================
// COMANDOS UART
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
    Serial.print("[CMD] ");
    Serial.println(inputCommand);
    parseCommand(inputCommand);
    inputCommand = "";
    commandComplete = false;
  }
}

void parseCommand(String cmd) {
  if (cmd.startsWith("S1,")) {
    moveServo(1, cmd.substring(3).toInt());
  }

  else if (cmd.startsWith("S2,")) {
    moveServo(2, cmd.substring(3).toInt());
  }

  else if (cmd.startsWith("BOTH,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    if (comma2 < 0) {
      Serial.println("[ERROR] Formato: BOTH,<angle1>,<angle2>");
      return;
    }

    int angle1 = cmd.substring(comma1 + 1, comma2).toInt();
    int angle2 = cmd.substring(comma2 + 1).toInt();
    moveServo(1, angle1);
    moveServo(2, angle2);
  }

  else if (cmd.startsWith("DIR,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    if (comma2 < 0) {
      Serial.println("[ERROR] Formato: DIR,<servo>,<LEFT|CENTER|RIGHT>");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1);
    moveServoDirection(servo, direction);
  }

  else if (cmd.startsWith("STEP,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    if (comma2 < 0) {
      Serial.println("[ERROR] Formato: STEP,<servo>,<LEFT|RIGHT>");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1);
    stepServo(servo, direction);
  }

  else if (cmd.startsWith("CR,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    int comma3 = cmd.indexOf(',', comma2 + 1);
    if (comma2 < 0 || comma3 < 0) {
      Serial.println("[ERROR] Formato: CR,<servo>,<CW|CCW>,<speed 0-100>");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1, comma3);
    int speedPct = cmd.substring(comma3 + 1).toInt();
    moveContinuousServo(servo, direction, speedPct);
  }

  else if (cmd.startsWith("STOP,")) {
    stopContinuousServo(cmd.substring(5).toInt());
  }

  else if (cmd.startsWith("DISABLE,")) {
    disableServo(cmd.substring(8).toInt());
  }

  else if (cmd == "STATUS") {
    printStatus();
  }

  else if (cmd == "HELP") {
    printHelp();
  }

  else {
    Serial.println("[ERROR] Comando no reconocido - Use HELP");
  }
}

// ============================================================================
// INFORMACION
// ============================================================================

void printStatus() {
  Serial.println();
  Serial.println("========== ESTADO SERVOS ==========");
  Serial.print("Servo 1 GPIO");
  Serial.print(SERVO1_PIN);
  Serial.print(" Angle=");
  Serial.print(servo1Angle);
  Serial.print(" Mode=");
  Serial.print(servo1Mode);
  Serial.print(" Enabled=");
  Serial.println(servo1Enabled ? "YES" : "NO");

  Serial.print("Servo 2 GPIO");
  Serial.print(SERVO2_PIN);
  Serial.print(" Angle=");
  Serial.print(servo2Angle);
  Serial.print(" Mode=");
  Serial.print(servo2Mode);
  Serial.print(" Enabled=");
  Serial.println(servo2Enabled ? "YES" : "NO");
  Serial.println("===================================");
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("Comandos disponibles:");
  Serial.println("  S1,<angle>             Servo 1 a angulo 0-180");
  Serial.println("  S2,<angle>             Servo 2 a angulo 0-180");
  Serial.println("  BOTH,<angle1>,<angle2> Ambos servos");
  Serial.println("  DIR,<servo>,LEFT       Posicion izquierda");
  Serial.println("  DIR,<servo>,CENTER     Posicion centro");
  Serial.println("  DIR,<servo>,RIGHT      Posicion derecha");
  Serial.println("  STEP,<servo>,LEFT      Paso hacia izquierda");
  Serial.println("  STEP,<servo>,RIGHT     Paso hacia derecha");
  Serial.println("  CR,<servo>,CW,<speed>  Giro continuo horario 0-100");
  Serial.println("  CR,<servo>,CCW,<speed> Giro continuo antihorario 0-100");
  Serial.println("  STOP,<servo>           Parar servo continuo");
  Serial.println("  DISABLE,<servo>        Desactivar PWM");
  Serial.println("  STATUS                 Estado actual");
  Serial.println("  HELP                   Mostrar ayuda");
  Serial.println();
}
