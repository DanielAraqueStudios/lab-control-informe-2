  /*
  * LABORATORIO 2 - SPRINT 10
  * Tank 1 height control firmware for the hydraulic system.
  *
  * Purpose:
  *   Control Tank 1 height from UART using the filtered HC-SR04 reading and
  *   the Tank 1 outlet servo valve. Manual pump and valve commands remain
  *   available for bring-up.
  *
  * Canonical Sprint 09 pinout:
  *   GPIO17 -> Pump PWM / L298N ENA
  *   GPIO15 -> Pump IN1
  *   GPIO16 -> Pump IN2
  *   GPIO4  -> YF-S401 flow sensor signal
  *   GPIO5  -> HC-SR04 Tank 1 TRIG
  *   GPIO6  -> HC-SR04 Tank 1 ECHO
  *   GPIO8  -> HC-SR04 Tank 2 TRIG
  *   GPIO9  -> HC-SR04 Tank 2 ECHO
  *   GPIO7  -> Status LED
  *   GPIO18 -> Servo valve Tank 1
  *   GPIO19 -> Servo valve Tank 2
  *
  * Serial:
  *   115200 baud, newline enabled.
  */

  #include <ESP32Servo.h>

  // Pins
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

  // Pump PWM
  const int PWM_FREQUENCY = 10000;
  const int PWM_RESOLUTION = 8;
  const int PWM_MIN = 0;
  const int PWM_MAX = 255;

  // Native LEDC Channels (ESP32-S3 has 8 channels: 0-7)
  const int PUMP_LEDC_CH = 0;
  const int SERVO1_LEDC_CH = 1;
  const int SERVO2_LEDC_CH = 2;

  const int SERVO_MIN_US = 500;
  const int SERVO_MAX_US = 2500;
  const int SERVO_NEUTRAL_US = 1500;
  const int SERVO_MIN_ANGLE = 0;
  const int SERVO_MAX_ANGLE = 180;
  const int VALVE_FULL_OPEN_ANGLE = 170;
  const int VALVE_CLOSED_ANGLE = 180;
  const int VALVE_STEP_DEGREES = 1;
  const float TANK1_TARGET_DEFAULT_MM = 80.0;
  const float TANK1_HEIGHT_DEADBAND_MM = 3.0;
  const float TANK1_FULL_OPEN_ERROR_MM = 40.0;
  const unsigned long HEIGHT_CONTROL_INTERVAL_MS = 300;
  const unsigned long HEIGHT_CONTROL_LOG_INTERVAL_MS = 2000;

  // 14-bit resolution for Servos (0-16383) at 50Hz (20ms period)
  // Duty cycle for 500us = (500 / 20000) * 16384 = 410
  // Duty cycle for 2500us = (2500 / 20000) * 16384 = 2048
  const int SERVO_DUTY_MIN = 410;
  const int SERVO_DUTY_MAX = 2048;

  // Sensors
  const float TANK1_HEIGHT_MM = 150.0;
  const float TANK2_HEIGHT_MM = 160.0;
  const float MAX_LEVEL_ALLOW = 110.0;
  const unsigned long ECHO_TIMEOUT_US = 30000;
  const float SOUND_MM_PER_US = 0.343;
  const unsigned long ULTRASONIC_PING_GAP_MS = 60;
  const float MIN_VALID_DISTANCE_MM = 20.0;
  const float MAX_DISTANCE_MARGIN_MM = 30.0;
  const float MAX_DISTANCE_JUMP_MM = 20.0;
  const int DISTANCE_FILTER_SIZE = 5;
  const int OUTLIER_CONFIRM_COUNT = 3;
  const float FLOW_PULSES_PER_LITER = 98.0;

  // Telemetry
  const unsigned long STREAM_INTERVAL_MS = 250;

  // Global state
  volatile unsigned long flowPulseCount = 0;
  unsigned long lastFlowUpdateMs = 0;
  unsigned long lastStreamMs = 0;
  unsigned long lastHeightControlMs = 0;
  unsigned long lastHeightControlLogMs = 0;
  bool flowInterruptAttached = false;

  float flowRateLpm = 0.0;
  float totalVolumeL = 0.0;
  float waterLevel1Mm = 0.0;
  float waterLevel2Mm = 0.0;
  float distance1Mm = 0.0;
  float distance2Mm = 0.0;
  float distance1Buffer[DISTANCE_FILTER_SIZE] = {0};
  float distance2Buffer[DISTANCE_FILTER_SIZE] = {0};
  int distance1BufferIndex = 0;
  int distance2BufferIndex = 0;
  bool distance1FilterPrimed = false;
  bool distance2FilterPrimed = false;
  int distance1OutlierCount = 0;
  int distance2OutlierCount = 0;
  float distance1CandidateMm = 0.0;
  float distance2CandidateMm = 0.0;

  int currentPWM = 0;
  bool pumpEnabled = false;
  bool pumpForward = true;
  bool streamEnabled = true; // Enabled by default to see telemetry on UART loop
  bool safetyEnabled = false;
  bool isolateFlowInterruptDuringUltrasonic = true;
  bool tank1HeightControlEnabled = false;
  float tank1TargetHeightMm = TANK1_TARGET_DEFAULT_MM;
  int lastTank1ControlAngle = -1;

  int tank1ValveAngle = VALVE_CLOSED_ANGLE;
  int tank2ValveAngle = VALVE_CLOSED_ANGLE;
  bool tank1ServoOutputEnabled = false;
  bool tank2ServoOutputEnabled = false;

  String inputCommand = "";
  bool commandComplete = false;

  void IRAM_ATTR flowPulseCounter() {
    flowPulseCount++;
  }

  void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);
    pinMode(LED_STATUS_PIN, OUTPUT);
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(TRIG_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);
    pinMode(TRIG_PIN_2, OUTPUT);
    pinMode(ECHO_PIN_2, INPUT);

    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    digitalWrite(LED_STATUS_PIN, LOW);
    digitalWrite(TRIG_PIN_1, LOW);
    digitalWrite(TRIG_PIN_2, LOW);

    // 1. Explicitly setup native LEDC channel for Pump (Channel 0, 10kHz, 8-bit)
    ledcAttachChannel(MOTOR_PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION, PUMP_LEDC_CH);
    ledcWrite(MOTOR_PWM_PIN, 0);

    // 2. Explicitly setup native LEDC channels for Servos (Channels 1 & 2, 50Hz, 14-bit)
    ledcAttachChannel(SERVO_TANK1_PIN, 50, 14, SERVO1_LEDC_CH);
    ledcAttachChannel(SERVO_TANK2_PIN, 50, 14, SERVO2_LEDC_CH);

    // Initial position!
    closeAllValves();

    attachFlowInterrupt();

    currentPWM = 0;
    pumpEnabled = false;
    applyPumpOutput();

    primeDistanceFilters();

    Serial.println();
    Serial.println("============================================================");
    Serial.println("  LAB 2 - SPRINT 10 TANK 1 HEIGHT CONTROL");
    Serial.println("============================================================");
    Serial.println("[OK] Hardware initialized");
    Serial.println("[OK] Startup state: PWM=0, pump OFF, valves CLOSED, Tank1 control OFF, Stream ON");
    printPinout();
    printStatus();
    printHelp();
  }

  void loop() {
    processSerialCommands();

    if (millis() - lastFlowUpdateMs >= 1000) {
      updateFlowRate();
      lastFlowUpdateMs = millis();
    }

    if (streamEnabled && millis() - lastStreamMs >= STREAM_INTERVAL_MS) {
      readAllSensors();
      printData();
      lastStreamMs = millis();
    }

    if (tank1HeightControlEnabled &&
        millis() - lastHeightControlMs >= HEIGHT_CONTROL_INTERVAL_MS) {
      readAllSensors();
      updateTank1HeightControl();
      lastHeightControlMs = millis();
    }

    if (safetyEnabled) {
      checkSafety();
    }

    updateStatusLED();
    delay(2);
  }

  // ---------------------------------------------------------------------------
  // Serial command handling
  // ---------------------------------------------------------------------------

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

    if (!commandComplete) {
      return;
    }

    inputCommand.trim();
    inputCommand.toUpperCase();

    Serial.print("[CMD] ");
    Serial.println(inputCommand);

    parseCommand(inputCommand);

    inputCommand = "";
    commandComplete = false;
  }

  void parseCommand(String cmd) {
    if (cmd.startsWith("PWM,") || cmd.startsWith("SETPWM,")) {
      int comma = cmd.indexOf(',');
      setPumpPWM(cmd.substring(comma + 1).toInt(), true);
    }
    else if (cmd == "PUMP,ON") {
      pumpOn();
    }
    else if (cmd == "PUMP,OFF") {
      pumpOff();
    }
    else if (cmd == "DIR,FWD" || cmd == "DIR,FORWARD") {
      setPumpDirection(true);
    }
    else if (cmd == "DIR,REV" || cmd == "DIR,REVERSE") {
      setPumpDirection(false);
    }
    else if (cmd.startsWith("S1,")) {
      tank1HeightControlEnabled = false;
      setServoAngle(1, cmd.substring(3).toInt());
    }
    else if (cmd.startsWith("S2,")) {
      setServoAngle(2, cmd.substring(3).toInt());
    }
    else if (cmd.startsWith("BOTH,")) {
      int comma1 = cmd.indexOf(',');
      int comma2 = cmd.indexOf(',', comma1 + 1);
      if (comma2 < 0) {
        Serial.println("[ERROR] Use BOTH,<angle1>,<angle2>");
        return;
      }
      tank1HeightControlEnabled = false;
      setServoAngle(1, cmd.substring(comma1 + 1, comma2).toInt());
      setServoAngle(2, cmd.substring(comma2 + 1).toInt());
    }
    else if (cmd == "V1,OPEN") {
      tank1HeightControlEnabled = false;
      openValve(1);
    }
    else if (cmd == "V1,CLOSE") {
      tank1HeightControlEnabled = false;
      closeValve(1);
    }
    else if (cmd == "V2,OPEN") {
      openValve(2);
    }
    else if (cmd == "V2,CLOSE") {
      closeValve(2);
    }
    else if (cmd == "VALVES,OPEN") {
      tank1HeightControlEnabled = false;
      openAllValves();
    }
    else if (cmd == "VALVES,CLOSE") {
      tank1HeightControlEnabled = false;
      closeAllValves();
    }
    else if (cmd.startsWith("SETHEIGHT1,") ||
             cmd.startsWith("SETLEVEL1,") ||
             cmd.startsWith("TARGET1,") ||
             cmd.startsWith("H1,")) {
      int comma = cmd.indexOf(',');
      setTank1HeightTarget(cmd.substring(comma + 1).toFloat());
    }
    else if (cmd == "LEVEL1,ON" || cmd == "HEIGHT1,ON" || cmd == "LEVELCTRL1,ON") {
      enableTank1HeightControl();
    }
    else if (cmd == "LEVEL1,OFF" || cmd == "HEIGHT1,OFF" || cmd == "LEVELCTRL1,OFF") {
      disableTank1HeightControl(true);
    }
    else if (cmd == "LEVEL1,STATUS" || cmd == "HEIGHT1,STATUS") {
      readAllSensors();
      printTank1HeightControlStatus();
    }
    else if (cmd.startsWith("DIR,")) {
      handleServoDirectionCommand(cmd);
    }
    else if (cmd.startsWith("STEP,")) {
      handleServoStepCommand(cmd);
    }
    else if (cmd.startsWith("CR,")) {
      handleContinuousServoCommand(cmd);
    }
    else if (cmd.startsWith("STOP,")) {
      handleServoStopCommand(cmd);
    }
    else if (cmd.startsWith("DISABLE,")) {
      int servo = cmd.substring(8).toInt();
      if (servo == 1) {
        tank1HeightControlEnabled = false;
      }
      disableServo(servo);
    }
    else if (cmd == "SERVOTEST,") {
      // This expects parameter but logic fell through above if exact match failed.
      // Handled in startsWith("SERVOTEST,")
    }
    else if (cmd == "DISTANCE" || cmd == "DIST") {
      readAllSensors();
      Serial.println();
      Serial.println("========== DISTANCE (FILTERED) ==========");
      Serial.print("Sensor 1 (Tank 1): ");
      Serial.print(distance1Mm, 1);
      Serial.println(" mm");
      Serial.print("Sensor 2 (Tank 2): ");
      Serial.print(distance2Mm, 1);
      Serial.println(" mm");
      Serial.println("====================================");
    }
    else if (cmd == "READ") {
      readAllSensors();
      printData();
    }
    else if (cmd == "STATUS") {
      readAllSensors();
      printStatus();
    }
    else if (cmd == "VALVESTATUS") {
      printValveStatus();
    }
    else if (cmd.startsWith("SERVOTEST,")) {
      servoTest(cmd.substring(10).toInt());
    }
    else if (cmd == "STREAM,ON") {
      streamEnabled = true;
      Serial.println("[OK] Stream ON");
    }
    else if (cmd == "STREAM,OFF" || cmd == "DATALOG") {
      streamEnabled = false;
      Serial.println("[OK] Stream OFF");
    }
    else if (cmd == "SAFETY,ON") {
      safetyEnabled = true;
      Serial.println("[OK] Safety ON");
    }
    else if (cmd == "SAFETY,OFF") {
      safetyEnabled = false;
      Serial.println("[OK] Safety OFF");
    }
    else if (cmd == "FLOWISO,ON") {
      isolateFlowInterruptDuringUltrasonic = true;
      Serial.println("[OK] Flow interrupt isolation ON");
    }
    else if (cmd == "FLOWISO,OFF") {
      isolateFlowInterruptDuringUltrasonic = false;
      attachFlowInterrupt();
      Serial.println("[OK] Flow interrupt isolation OFF");
    }
    else if (cmd == "PINOUT") {
      printPinout();
    }
    else if (cmd == "STOP") {
      emergencyStop();
    }
    else if (cmd == "HELP") {
      printHelp();
    }
    else {
      Serial.println("[ERROR] Unknown command. Use HELP.");
    }
  }

  // ---------------------------------------------------------------------------
  // Pump
  // ---------------------------------------------------------------------------

  void setPumpPWM(int pwmValue, bool enableOutput) {
    currentPWM = constrain(pwmValue, PWM_MIN, PWM_MAX);
    pumpEnabled = enableOutput && currentPWM > 0;
    applyPumpOutput();

    Serial.print("[OK] PWM=");
    Serial.print(currentPWM);
    Serial.print(" pump=");
    Serial.println(pumpEnabled ? "ON" : "OFF");
  }

  void pumpOn() {
    pumpEnabled = currentPWM > 0;
    applyPumpOutput();

    if (currentPWM == 0) {
      Serial.println("[WARN] Pump ON requested but PWM is 0. Use PWM,<1-255>.");
    } else {
      Serial.println("[OK] Pump ON");
    }
  }

  void pumpOff() {
    pumpEnabled = false;
    applyPumpOutput();
    Serial.println("[OK] Pump OFF");
  }

  void setPumpDirection(bool forward) {
    pumpForward = forward;
    applyPumpOutput();

    Serial.print("[OK] Pump direction=");
    Serial.println(pumpForward ? "FWD" : "REV");
  }

  void applyPumpOutput() {
    if (pumpEnabled && currentPWM > 0) {
      digitalWrite(MOTOR_IN1_PIN, pumpForward ? HIGH : LOW);
      digitalWrite(MOTOR_IN2_PIN, pumpForward ? LOW : HIGH);
      ledcWrite(MOTOR_PWM_PIN, currentPWM);
    } else {
      digitalWrite(MOTOR_IN1_PIN, LOW);
      digitalWrite(MOTOR_IN2_PIN, LOW);
      ledcWrite(MOTOR_PWM_PIN, 0);
    }
  }

  // ---------------------------------------------------------------------------
  // Servos and valves
  // ---------------------------------------------------------------------------

  void disableServo(int servo) {
    if (servo == 1) {
      ledcWrite(SERVO_TANK1_PIN, 0); // 0 duty cycle removes the pulse
      tank1ServoOutputEnabled = false;
      Serial.println("[OK] Servo1 disabled");
    } else if (servo == 2) {
      ledcWrite(SERVO_TANK2_PIN, 0);
      tank2ServoOutputEnabled = false;
      Serial.println("[OK] Servo2 disabled");
    } else {
      Serial.println("[ERROR] Servo must be 1 or 2");
    }
  }

  void setServoAngle(int servo, int angle) {
    applyServoAngle(servo, angle, true);
  }

  void applyServoAngle(int servo, int angle, bool verbose) {
    angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    int duty = map(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_DUTY_MIN, SERVO_DUTY_MAX);

    if (servo == 1) {
      tank1ValveAngle = angle;
      ledcWrite(SERVO_TANK1_PIN, duty);
      tank1ServoOutputEnabled = true;
      if (verbose) {
        Serial.print("[OK] Servo1 angle=");
        Serial.print(tank1ValveAngle);
        Serial.print(" duty=");
        Serial.println(duty);
      }
    } else if (servo == 2) {
      tank2ValveAngle = angle;
      ledcWrite(SERVO_TANK2_PIN, duty);
      tank2ServoOutputEnabled = true;
      if (verbose) {
        Serial.print("[OK] Servo2 angle=");
        Serial.print(tank2ValveAngle);
        Serial.print(" duty=");
        Serial.println(duty);
      }
    } else {
      Serial.println("[ERROR] Servo must be 1 or 2");
    }
  }

  void writeServoPulse(int servo, int pulseUs) {
    pulseUs = constrain(pulseUs, SERVO_MIN_US, SERVO_MAX_US);
    int duty = map(pulseUs, SERVO_MIN_US, SERVO_MAX_US, SERVO_DUTY_MIN, SERVO_DUTY_MAX);

    if (servo == 1) {
      ledcWrite(SERVO_TANK1_PIN, duty);
      tank1ServoOutputEnabled = true;
      Serial.print("[OK] Servo1 pulse_us=");
      Serial.println(pulseUs);
    } else if (servo == 2) {
      ledcWrite(SERVO_TANK2_PIN, duty);
      tank2ServoOutputEnabled = true;
      Serial.print("[OK] Servo2 pulse_us=");
      Serial.println(pulseUs);
    } else {
      Serial.println("[ERROR] Servo must be 1 or 2");
    }
  }

  void openValve(int tank) {
    setServoAngle(tank, VALVE_FULL_OPEN_ANGLE);
  }

  void closeValve(int tank) {
    setServoAngle(tank, VALVE_CLOSED_ANGLE);
  }

  void openAllValves() {
    openValve(1);
    openValve(2);
  }

  void closeAllValves() {
    closeValve(1);
    closeValve(2);
  }

  void handleServoDirectionCommand(String cmd) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);

    if (comma2 < 0) {
      Serial.println("[ERROR] Use DIR,<servo>,LEFT|CENTER|RIGHT or DIR,FWD|REV");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1);

    if (servo == 1) {
      tank1HeightControlEnabled = false;
    }

    if (direction == "LEFT") {
      setServoAngle(servo, VALVE_FULL_OPEN_ANGLE);
    } else if (direction == "CENTER") {
      setServoAngle(servo, (VALVE_FULL_OPEN_ANGLE + VALVE_CLOSED_ANGLE) / 2);
    } else if (direction == "RIGHT") {
      setServoAngle(servo, VALVE_CLOSED_ANGLE);
    } else {
      Serial.println("[ERROR] Direction must be LEFT, CENTER, or RIGHT");
    }
  }

  void handleServoStepCommand(String cmd) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);

    if (comma2 < 0) {
      Serial.println("[ERROR] Use STEP,<servo>,LEFT|RIGHT");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1);
    int currentAngle = (servo == 1) ? tank1ValveAngle : tank2ValveAngle;

    if (servo == 1) {
      tank1HeightControlEnabled = false;
    }

    if (direction == "LEFT") {
      setServoAngle(servo, currentAngle - VALVE_STEP_DEGREES);
    } else if (direction == "RIGHT") {
      setServoAngle(servo, currentAngle + VALVE_STEP_DEGREES);
    } else {
      Serial.println("[ERROR] Step direction must be LEFT or RIGHT");
    }
  }

  void handleContinuousServoCommand(String cmd) {
    int comma1 = cmd.indexOf(',');
    int comma2 = cmd.indexOf(',', comma1 + 1);
    int comma3 = cmd.indexOf(',', comma2 + 1);

    if (comma2 < 0 || comma3 < 0) {
      Serial.println("[ERROR] Use CR,<servo>,CW|CCW,<speed>");
      return;
    }

    int servo = cmd.substring(comma1 + 1, comma2).toInt();
    String direction = cmd.substring(comma2 + 1, comma3);
    int speed = constrain(cmd.substring(comma3 + 1).toInt(), 0, 100);
    int spanUs = SERVO_MAX_US - SERVO_NEUTRAL_US;
    int pulseUs = SERVO_NEUTRAL_US;

    if (servo == 1) {
      tank1HeightControlEnabled = false;
    }

    if (direction == "CW") {
      pulseUs = SERVO_NEUTRAL_US + ((spanUs * speed) / 100);
    } else if (direction == "CCW") {
      pulseUs = SERVO_NEUTRAL_US - ((spanUs * speed) / 100);
    } else {
      Serial.println("[ERROR] Direction must be CW or CCW");
      return;
    }

    writeServoPulse(servo, pulseUs);
  }

  void handleServoStopCommand(String cmd) {
    int comma = cmd.indexOf(',');
    int servo = cmd.substring(comma + 1).toInt();
    if (servo == 1) {
      tank1HeightControlEnabled = false;
    }
    writeServoPulse(servo, SERVO_NEUTRAL_US);
  }

  void servoTest(int servo) {
    if (servo != 1 && servo != 2) {
      Serial.println("[ERROR] Servo must be 1 or 2");
      return;
    }

    if (servo == 1) {
      tank1HeightControlEnabled = false;
    }

    Serial.print("[OK] Servo");
    Serial.print(servo);
    Serial.println(" test start");

    setServoAngle(servo, VALVE_CLOSED_ANGLE);
    delay(600);
    setServoAngle(servo, VALVE_FULL_OPEN_ANGLE);
    delay(600);
    setServoAngle(servo, VALVE_CLOSED_ANGLE);

    Serial.print("[OK] Servo");
    Serial.print(servo);
    Serial.println(" test done");
  }

  // ---------------------------------------------------------------------------
  // Tank 1 height control
  // ---------------------------------------------------------------------------

  void setTank1HeightTarget(float targetMm) {
    tank1TargetHeightMm = constrain(targetMm, 0.0, MAX_LEVEL_ALLOW);

    Serial.print("[OK] Tank1 target_mm=");
    Serial.print(tank1TargetHeightMm, 1);
    Serial.print(" range=0.0-");
    Serial.println(MAX_LEVEL_ALLOW, 1);
  }

  void enableTank1HeightControl() {
    readAllSensors();
    tank1HeightControlEnabled = true;
    lastHeightControlMs = 0;
    lastHeightControlLogMs = 0;
    lastTank1ControlAngle = -1;

    Serial.println("[OK] Tank1 height control ON");
    printTank1HeightControlStatus();
  }

  void disableTank1HeightControl(bool closeValveAfterStop) {
    tank1HeightControlEnabled = false;
    lastTank1ControlAngle = -1;

    if (closeValveAfterStop) {
      closeValve(1);
    }

    Serial.println("[OK] Tank1 height control OFF");
  }

  void updateTank1HeightControl() {
    float errorMm = waterLevel1Mm - tank1TargetHeightMm;
    int desiredAngle = VALVE_CLOSED_ANGLE;

    if (errorMm > TANK1_HEIGHT_DEADBAND_MM) {
      float activeErrorMm = errorMm - TANK1_HEIGHT_DEADBAND_MM;
      float ratio = constrain(activeErrorMm /
                              (TANK1_FULL_OPEN_ERROR_MM - TANK1_HEIGHT_DEADBAND_MM),
                              0.0,
                              1.0);
      desiredAngle = VALVE_CLOSED_ANGLE -
                     (int)((VALVE_CLOSED_ANGLE - VALVE_FULL_OPEN_ANGLE) * ratio + 0.5);
    }

    desiredAngle = constrain(desiredAngle, VALVE_FULL_OPEN_ANGLE, VALVE_CLOSED_ANGLE);
    applyServoAngle(1, desiredAngle, false);

    unsigned long now = millis();
    if (desiredAngle != lastTank1ControlAngle ||
        now - lastHeightControlLogMs >= HEIGHT_CONTROL_LOG_INTERVAL_MS) {
      Serial.print("[CTRL1] target_mm=");
      Serial.print(tank1TargetHeightMm, 1);
      Serial.print(",level_mm=");
      Serial.print(waterLevel1Mm, 1);
      Serial.print(",error_mm=");
      Serial.print(errorMm, 1);
      Serial.print(",valve_angle=");
      Serial.println(desiredAngle);

      lastTank1ControlAngle = desiredAngle;
      lastHeightControlLogMs = now;
    }
  }

  void printTank1HeightControlStatus() {
    Serial.println();
    Serial.println("========== TANK 1 HEIGHT CONTROL ==========");
    Serial.print("Control: ");
    Serial.println(tank1HeightControlEnabled ? "ON" : "OFF");
    Serial.print("Target: ");
    Serial.print(tank1TargetHeightMm, 1);
    Serial.println(" mm");
    Serial.print("Level: ");
    Serial.print(waterLevel1Mm, 1);
    Serial.print(" mm | Distance: ");
    Serial.print(distance1Mm, 1);
    Serial.println(" mm");
    Serial.print("Valve angle: ");
    Serial.print(tank1ValveAngle);
    Serial.println(" degrees");
    Serial.print("Deadband: +/-");
    Serial.print(TANK1_HEIGHT_DEADBAND_MM, 1);
    Serial.println(" mm");
    Serial.println("170=open | 180=closed");
    Serial.println("===========================================");
  }

  // ---------------------------------------------------------------------------
  // Sensors
  // ---------------------------------------------------------------------------

  void updateFlowRate() {
    static unsigned long previousMs = 0;
    unsigned long now = millis();
    float dtSeconds = (now - previousMs) / 1000.0;

    if (previousMs == 0 || dtSeconds <= 0) {
      previousMs = now;
      return;
    }

    noInterrupts();
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    interrupts();

    float frequencyHz = pulses / dtSeconds;
    flowRateLpm = frequencyHz / (FLOW_PULSES_PER_LITER / 60.0);
    totalVolumeL += pulses / FLOW_PULSES_PER_LITER;
    previousMs = now;
  }

  void attachFlowInterrupt() {
    if (!flowInterruptAttached) {
      attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, RISING);
      flowInterruptAttached = true;
    }
  }

  void detachFlowInterrupt() {
    if (flowInterruptAttached) {
      detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
      flowInterruptAttached = false;
    }
  }

  float measureDistanceMm(int trigPin, int echoPin, float maxDistanceMm) {
    unsigned long waitStartedUs = micros();
    while (digitalRead(echoPin) == HIGH) {
      if (micros() - waitStartedUs >= ECHO_TIMEOUT_US) {
        return -1.0;
      }
    }

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long durationUs = pulseIn(echoPin, HIGH, ECHO_TIMEOUT_US);
    if (durationUs == 0) {
      return -1.0;
    }

    float distanceMm = (durationUs * SOUND_MM_PER_US) / 2.0;
    if (distanceMm < MIN_VALID_DISTANCE_MM ||
        distanceMm > maxDistanceMm + MAX_DISTANCE_MARGIN_MM) {
      return -1.0;
    }

    return distanceMm;
  }

  float medianDistance(float buffer[]) {
    float sorted[DISTANCE_FILTER_SIZE];
    for (int i = 0; i < DISTANCE_FILTER_SIZE; i++) {
      sorted[i] = buffer[i];
    }

    for (int i = 1; i < DISTANCE_FILTER_SIZE; i++) {
      float value = sorted[i];
      int j = i - 1;
      while (j >= 0 && sorted[j] > value) {
        sorted[j + 1] = sorted[j];
        j--;
      }
      sorted[j + 1] = value;
    }

    return sorted[DISTANCE_FILTER_SIZE / 2];
  }

  float filterDistanceMm(float rawMm,
                         float currentMm,
                         float buffer[],
                         int &bufferIndex,
                         bool &filterPrimed,
                         int &outlierCount,
                         float &candidateMm) {
    if (!filterPrimed) {
      for (int i = 0; i < DISTANCE_FILTER_SIZE; i++) {
        buffer[i] = rawMm;
      }
      bufferIndex = 0;
      filterPrimed = true;
      outlierCount = 0;
      candidateMm = rawMm;
      return rawMm;
    }

    float referenceMm = medianDistance(buffer);
    if (fabs(rawMm - referenceMm) > MAX_DISTANCE_JUMP_MM) {
      if (fabs(rawMm - candidateMm) <= MAX_DISTANCE_JUMP_MM) {
        outlierCount++;
      } else {
        candidateMm = rawMm;
        outlierCount = 1;
      }

      if (outlierCount < OUTLIER_CONFIRM_COUNT) {
        return currentMm;
      }
    }

    buffer[bufferIndex] = rawMm;
    bufferIndex = (bufferIndex + 1) % DISTANCE_FILTER_SIZE;
    outlierCount = 0;
    candidateMm = rawMm;
    return medianDistance(buffer);
  }

  void readAllSensors() {
    if (isolateFlowInterruptDuringUltrasonic) {
      detachFlowInterrupt();
    }

    float d1 = measureDistanceMm(TRIG_PIN_1, ECHO_PIN_1, TANK1_HEIGHT_MM);
    delay(ULTRASONIC_PING_GAP_MS);
    float d2 = measureDistanceMm(TRIG_PIN_2, ECHO_PIN_2, TANK2_HEIGHT_MM);

    if (d1 >= 0) {
      distance1Mm = filterDistanceMm(d1,
                                      distance1Mm,
                                      distance1Buffer,
                                      distance1BufferIndex,
                                      distance1FilterPrimed,
                                      distance1OutlierCount,
                                      distance1CandidateMm);
      waterLevel1Mm = constrain(TANK1_HEIGHT_MM - distance1Mm, 0.0, TANK1_HEIGHT_MM);
    }
    if (d2 >= 0) {
      distance2Mm = filterDistanceMm(d2,
                                      distance2Mm,
                                      distance2Buffer,
                                      distance2BufferIndex,
                                      distance2FilterPrimed,
                                      distance2OutlierCount,
                                      distance2CandidateMm);
      waterLevel2Mm = constrain(TANK2_HEIGHT_MM - distance2Mm, 0.0, TANK2_HEIGHT_MM);
    }

    if (isolateFlowInterruptDuringUltrasonic) {
      attachFlowInterrupt();
    }
  }

  void primeDistanceFilters() {
    for (int i = 0; i < DISTANCE_FILTER_SIZE; i++) {
      readAllSensors();
      delay(ULTRASONIC_PING_GAP_MS);
    }
  }

  // ---------------------------------------------------------------------------
  // Output and diagnostics
  // ---------------------------------------------------------------------------

  void printData() {
    Serial.print("[DATA] flow_lpm=");
    Serial.print(flowRateLpm, 3);
    Serial.print(",t1_mm=");
    Serial.print(waterLevel1Mm, 1);
    Serial.print(",t2_mm=");
    Serial.print(waterLevel2Mm, 1);
    Serial.print(",d1_mm=");
    Serial.print(distance1Mm, 1);
    Serial.print(",d2_mm=");
    Serial.print(distance2Mm, 1);
    Serial.print(",pwm=");
    Serial.print(currentPWM);
    Serial.print(",pump=");
    Serial.print(pumpEnabled ? "ON" : "OFF");
    Serial.print(",dir=");
    Serial.print(pumpForward ? "FWD" : "REV");
    Serial.print(",s1=");
    Serial.print(tank1ValveAngle);
    Serial.print(",s2=");
    Serial.print(tank2ValveAngle);
    Serial.print(",volume_l=");
    Serial.print(totalVolumeL, 3);
    Serial.print(",ctrl1=");
    Serial.print(tank1HeightControlEnabled ? "ON" : "OFF");
    Serial.print(",target1_mm=");
    Serial.println(tank1TargetHeightMm, 1);
  }

  void printStatus() {
    Serial.println();
    Serial.println("========== SPRINT 10 TANK 1 STATUS ==========");
    Serial.print("Pump: ");
    Serial.print(pumpEnabled ? "ON" : "OFF");
    Serial.print(" | PWM=");
    Serial.print(currentPWM);
    Serial.print(" | Dir=");
    Serial.println(pumpForward ? "FWD" : "REV");

    Serial.print("Servo1/Tank1: angle=");
    Serial.print(tank1ValveAngle);
    Serial.print(" pin=GPIO");
    Serial.print(SERVO_TANK1_PIN);
    Serial.print(" output=");
    Serial.println(tank1ServoOutputEnabled ? "ON" : "OFF");
    Serial.print("Tank1 control: ");
    Serial.print(tank1HeightControlEnabled ? "ON" : "OFF");
    Serial.print(" | target=");
    Serial.print(tank1TargetHeightMm, 1);
    Serial.println(" mm");

    Serial.print("Servo2/Tank2: angle=");
    Serial.print(tank2ValveAngle);
    Serial.print(" pin=GPIO");
    Serial.print(SERVO_TANK2_PIN);
    Serial.print(" output=");
    Serial.println(tank2ServoOutputEnabled ? "ON" : "OFF");

    Serial.print("Levels: T1=");
    Serial.print(waterLevel1Mm, 1);
    Serial.print(" mm | T2=");
    Serial.print(waterLevel2Mm, 1);
    Serial.println(" mm");

    Serial.print("Distances (Filtered): D1=");
    Serial.print(distance1Mm, 1);
    Serial.print(" mm | D2=");
    Serial.print(distance2Mm, 1);
    Serial.println(" mm");

    Serial.print("Flow: ");
    Serial.print(flowRateLpm, 3);
    Serial.print(" L/min | Volume=");
    Serial.print(totalVolumeL, 3);
    Serial.println(" L");

    Serial.print("Stream: ");
    Serial.print(streamEnabled ? "ON" : "OFF");
    Serial.print(" | Safety: ");
    Serial.print(safetyEnabled ? "ON" : "OFF");
    Serial.print(" | FlowISO: ");
    Serial.println(isolateFlowInterruptDuringUltrasonic ? "ON" : "OFF");
    Serial.println("Valve calibration: 170=open, 180=closed");
    Serial.println("=============================================");
  }

  void printValveStatus() {
    Serial.println();
    Serial.println("========== VALVE STATUS ==========");
    Serial.print("T1 servo pin GPIO");
    Serial.print(SERVO_TANK1_PIN);
    Serial.print(" angle=");
    Serial.print(tank1ValveAngle);
    Serial.print(" output=");
    Serial.println(tank1ServoOutputEnabled ? "ON" : "OFF");

    Serial.print("T2 servo pin GPIO");
    Serial.print(SERVO_TANK2_PIN);
    Serial.print(" angle=");
    Serial.print(tank2ValveAngle);
    Serial.print(" output=");
    Serial.println(tank2ServoOutputEnabled ? "ON" : "OFF");
    Serial.println("170=open | 180=closed");
    Serial.println("==================================");
  }

  void printPinout() {
    Serial.println();
    Serial.println("========== PINOUT ==========");
    Serial.println("Pump PWM / ENA : GPIO17");
    Serial.println("Pump IN1       : GPIO15");
    Serial.println("Pump IN2       : GPIO16");
    Serial.println("YF-S401        : GPIO4");
    Serial.println("T1 HC-SR04     : TRIG GPIO5, ECHO GPIO6");
    Serial.println("T2 HC-SR04     : TRIG GPIO8, ECHO GPIO9");
    Serial.println("Status LED     : GPIO7");
    Serial.println("Servo valve T1 : GPIO18");
    Serial.println("Servo valve T2 : GPIO19");
    Serial.println("============================");
  }

  void printHelp() {
    Serial.println();
    Serial.println("Commands - Sprint 10 Tank 1 height control");
    Serial.println("  PWM,<0-255>          Set pump PWM and apply output immediately");
    Serial.println("  SETPWM,<0-255>       Alias for PWM");
    Serial.println("  PUMP,ON              Enable pump using current PWM");
    Serial.println("  PUMP,OFF             Disable pump");
    Serial.println("  DIR,FWD / DIR,REV    Set pump direction");
    Serial.println("  S1,<0-180>           Set Servo 1 raw angle");
    Serial.println("  S2,<0-180>           Set Servo 2 raw angle");
    Serial.println("  BOTH,<a1>,<a2>       Set both servo raw angles");
    Serial.println("  V1,OPEN / V1,CLOSE   Open/close Tank 1 valve");
    Serial.println("  V2,OPEN / V2,CLOSE   Open/close Tank 2 valve");
    Serial.println("  VALVES,OPEN/CLOSE    Open/close both valves");
    Serial.println("  SETHEIGHT1,<mm>      Set Tank 1 target height");
    Serial.println("  SETLEVEL1,<mm>       Alias for SETHEIGHT1");
    Serial.println("  TARGET1,<mm> / H1,<mm>  Short target aliases");
    Serial.println("  LEVEL1,ON/OFF        Enable/disable Tank 1 height control");
    Serial.println("  LEVEL1,STATUS        Print Tank 1 control status");
    Serial.println("  DIR,<s>,LEFT|CENTER|RIGHT  Valve positions: open/mid/closed");
    Serial.println("  STEP,<s>,LEFT|RIGHT  Move valve angle by 1 degree");
    Serial.println("  CR,<s>,CW|CCW,<0-100> Continuous-rotation servo pulse test");
    Serial.println("  STOP,<s>             Neutral pulse for continuous-rotation servo");
    Serial.println("  DISABLE,<s>          Detach servo PWM");
    Serial.println("  SERVOTEST,<s>        Sweep valve closed-open-closed");
    Serial.println("  READ                 Read sensors once");
    Serial.println("  DIST / DISTANCE      Read and Print raw ultrasonic distance");
    Serial.println("  STATUS               Print system status");
    Serial.println("  VALVESTATUS          Print servo valve status");
    Serial.println("  STREAM,ON/OFF        Toggle periodic [DATA] output");
    Serial.println("  DATALOG              Alias for STREAM,OFF");
    Serial.println("  SAFETY,ON/OFF        Optional pump safety by max level");
    Serial.println("  FLOWISO,ON/OFF       Isolate flow ISR during ultrasonic timing");
    Serial.println("  PINOUT               Print canonical Sprint 09 pinout");
    Serial.println("  STOP                 Emergency stop: pump off, valves closed");
    Serial.println("  HELP                 Show this help");
    Serial.println();
  }

  // ---------------------------------------------------------------------------
  // Safety and LED
  // ---------------------------------------------------------------------------

  void emergencyStop() {
    tank1HeightControlEnabled = false;
    pumpEnabled = false;
    applyPumpOutput();
    closeAllValves();
    Serial.println("[OK] Emergency stop: pump OFF, valves CLOSED, Tank1 control OFF");
  }

  void checkSafety() {
    if (waterLevel1Mm >= MAX_LEVEL_ALLOW || waterLevel2Mm >= MAX_LEVEL_ALLOW) {
      pumpEnabled = false;
      applyPumpOutput();
      Serial.println("[ERROR] Safety max level reached. Pump OFF.");
      delay(250);
    }
  }

  void updateStatusLED() {
    if (pumpEnabled) {
      digitalWrite(LED_STATUS_PIN, (millis() / 250) % 2);
    } else if (streamEnabled) {
      digitalWrite(LED_STATUS_PIN, (millis() / 1000) % 2);
    } else {
      digitalWrite(LED_STATUS_PIN, LOW);
    }
  }
