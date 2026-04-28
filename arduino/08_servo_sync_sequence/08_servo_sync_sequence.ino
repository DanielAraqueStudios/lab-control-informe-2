#include <ESP32Servo.h>

/*
 * Sprint 8 - ESP32-S3 dual servo sequence + UART debug/control
 *
 * Pins:
 *   Servo 1 signal -> GPIO18
 *   Servo 2 signal -> GPIO19
 *
 * UART commands at 115200 baud:
 *   START             Resume automatic sweep
 *   STOP              Stop automatic sweep
 *   HOME              Move both servos to 0
 *   S1,<angle>        Move Servo 1 to angle 0-180
 *   S2,<angle>        Move Servo 2 to angle 0-180
 *   SET,<s>,<angle>   Move servo s to angle 0-180
 *   BOTH,<a1>,<a2>    Move both servos to desired angles
 *   ANGLES            Print commanded angles for both servos
 *   GET,<servo>       Print commanded angle for one servo
 *   DEBUG             Print full debug state
 *   HELP              Print command list
 *
 * Note:
 *   Standard servos do not report real shaft position. ANGLES/GET/DEBUG show
 *   the last commanded angles sent by the ESP32-S3.
 */

Servo servo1;
Servo servo2;

const int pinServo1 = 18;
const int pinServo2 = 19;

const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;
const unsigned long SWEEP_STEP_DELAY_MS = 15;
const unsigned long END_PAUSE_MS = 1000;

int servo1Angle = 0;
int servo2Angle = 0;
bool autoSweepEnabled = true;
bool sweepingUp = true;
int sweepAngle = 0;
unsigned long lastSweepStepTime = 0;
unsigned long pauseStartTime = 0;
bool endPauseActive = false;

String inputCommand = "";
bool commandComplete = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  servo1.attach(pinServo1);
  servo2.attach(pinServo2);

  moveBothServos(0);

  Serial.println();
  Serial.println("============================================================");
  Serial.println("  ESP32-S3 - Sprint 8 Servo UART Debug");
  Serial.println("  Servo 1: GPIO18 | Servo 2: GPIO19");
  Serial.println("============================================================");
  Serial.println("[READY] Auto sweep enabled. Use HELP for commands.");
  printHelp();
}

void loop() {
  processSerialCommands();

  if (autoSweepEnabled) {
    updateAutoSweep();
  }
}

void updateAutoSweep() {
  if (endPauseActive) {
    if (millis() - pauseStartTime >= END_PAUSE_MS) {
      endPauseActive = false;
      sweepingUp = !sweepingUp;
    }
    return;
  }

  if (millis() - lastSweepStepTime < SWEEP_STEP_DELAY_MS) {
    return;
  }

  lastSweepStepTime = millis();
  moveBothServos(sweepAngle);

  if (sweepingUp) {
    sweepAngle++;
    if (sweepAngle > SERVO_MAX_ANGLE) {
      sweepAngle = SERVO_MAX_ANGLE;
      endPauseActive = true;
      pauseStartTime = millis();
    }
  } else {
    sweepAngle--;
    if (sweepAngle < SERVO_MIN_ANGLE) {
      sweepAngle = SERVO_MIN_ANGLE;
      endPauseActive = true;
      pauseStartTime = millis();
    }
  }
}

int normalizeAngle(int angle) {
  return constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
}

void moveServo(int servo, int angle) {
  angle = normalizeAngle(angle);

  if (servo == 1) {
    servo1Angle = angle;
    servo1.write(servo1Angle);
    Serial.print("[SERVO1] Angle=");
    Serial.println(servo1Angle);
  } else if (servo == 2) {
    servo2Angle = angle;
    servo2.write(servo2Angle);
    Serial.print("[SERVO2] Angle=");
    Serial.println(servo2Angle);
  } else {
    Serial.println("[ERROR] Servo must be 1 or 2");
  }
}

void moveBothServos(int angle) {
  angle = normalizeAngle(angle);
  servo1Angle = angle;
  servo2Angle = angle;
  servo1.write(servo1Angle);
  servo2.write(servo2Angle);
}

void moveServosToAngles(int angle1, int angle2) {
  moveServo(1, angle1);
  moveServo(2, angle2);
}

void printAngles() {
  Serial.print("[ANGLES] S1=");
  Serial.print(servo1Angle);
  Serial.print(" S2=");
  Serial.println(servo2Angle);
}

void printServoAngle(int servo) {
  if (servo == 1) {
    Serial.print("[ANGLE] S1=");
    Serial.println(servo1Angle);
  } else if (servo == 2) {
    Serial.print("[ANGLE] S2=");
    Serial.println(servo2Angle);
  } else {
    Serial.println("[ERROR] Servo must be 1 or 2");
  }
}

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
    Serial.print("[CMD] ");
    Serial.println(inputCommand);
    parseCommand(inputCommand);
    inputCommand = "";
    commandComplete = false;
  }
}

void parseCommand(String cmd) {
  if (cmd == "START") {
    autoSweepEnabled = true;
    endPauseActive = false;
    lastSweepStepTime = millis();
    Serial.println("[SWEEP] Started");
  }

  else if (cmd == "STOP") {
    autoSweepEnabled = false;
    endPauseActive = false;
    Serial.println("[SWEEP] Stopped");
  }

  else if (cmd == "HOME") {
    autoSweepEnabled = false;
    endPauseActive = false;
    sweepAngle = 0;
    moveBothServos(0);
    Serial.println("[HOME] Both servos at 0");
  }

  else if (cmd.startsWith("S1,")) {
    autoSweepEnabled = false;
    moveServo(1, cmd.substring(3).toInt());
  }

  else if (cmd.startsWith("S2,")) {
    autoSweepEnabled = false;
    moveServo(2, cmd.substring(3).toInt());
  }

  else if (cmd.startsWith("SET,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);

    if (comma2 < 0) {
      Serial.println("[ERROR] Format: SET,<servo>,<angle>");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    int angle = cmd.substring(comma2 + 1).toInt();
    autoSweepEnabled = false;
    moveServo(servo, angle);
  }

  else if (cmd.startsWith("BOTH,")) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);

    if (comma2 < 0) {
      Serial.println("[ERROR] Format: BOTH,<angle1>,<angle2>");
      return;
    }

    int angle1 = cmd.substring(comma1 + 1, comma2).toInt();
    int angle2 = cmd.substring(comma2 + 1).toInt();
    autoSweepEnabled = false;
    moveServosToAngles(angle1, angle2);
  }

  else if (cmd == "ANGLES") {
    printAngles();
  }

  else if (cmd.startsWith("GET,")) {
    printServoAngle(cmd.substring(4).toInt());
  }

  else if (cmd == "DEBUG") {
    printDebug();
  }

  else if (cmd == "HELP") {
    printHelp();
  }

  else {
    Serial.println("[ERROR] Unknown command. Use HELP.");
  }
}

void printDebug() {
  Serial.println();
  Serial.println("========== SERVO DEBUG ==========");
  Serial.print("Servo 1 pin: ");
  Serial.println(pinServo1);
  Serial.print("Servo 2 pin: ");
  Serial.println(pinServo2);
  Serial.print("Servo 1 commanded angle: ");
  Serial.println(servo1Angle);
  Serial.print("Servo 2 commanded angle: ");
  Serial.println(servo2Angle);
  Serial.print("Auto sweep: ");
  Serial.println(autoSweepEnabled ? "ON" : "OFF");
  Serial.print("Sweep direction: ");
  Serial.println(sweepingUp ? "UP" : "DOWN");
  Serial.print("Sweep angle: ");
  Serial.println(sweepAngle);
  Serial.print("End pause active: ");
  Serial.println(endPauseActive ? "YES" : "NO");
  Serial.println("Note: angles are commanded values, not measured feedback.");
  Serial.println("=================================");
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  START             Resume automatic 0-180 sweep");
  Serial.println("  STOP              Stop automatic sweep");
  Serial.println("  HOME              Move both servos to 0");
  Serial.println("  S1,<angle>        Move Servo 1 to angle 0-180");
  Serial.println("  S2,<angle>        Move Servo 2 to angle 0-180");
  Serial.println("  SET,<s>,<angle>   Move servo s to angle 0-180");
  Serial.println("  BOTH,<a1>,<a2>    Move both servos to desired angles");
  Serial.println("  ANGLES            Print commanded angles");
  Serial.println("  GET,<servo>       Print commanded angle for servo");
  Serial.println("  DEBUG             Print full debug state");
  Serial.println("  HELP              Print this help");
  Serial.println();
}

