/*
 * Laboratorio 2 - Nuevo Sensor Ultrasónico
 * Reemplazo de sensores análogos SE045 por HC-SR04.
 * 
 * Este código es una prueba aislada para verificar la lectura de ambos tanques
 * antes de integrarlo al sistema maestro.
 */

// --- Definición de Pines ---

// Tanque 1 (Configurado por el usuario)
const int TRIG_PIN_1 = 5;
const int ECHO_PIN_1 = 6;

// Tanque 2 (pines vigentes segun referencia Sprint 9)
const int TRIG_PIN_2 = 8;
const int ECHO_PIN_2 = 9;

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines Tanque 1
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  
  // Configuración de pines Tanque 2
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Asegurar que los Triggers comiencen en LOW
  digitalWrite(TRIG_PIN_1, LOW);
  digitalWrite(TRIG_PIN_2, LOW);

  Serial.println("========================================");
  Serial.println("Prueba de Sensores HC-SR04 iniciada");
  Serial.println("T1: Trig=5, Echo=6 | T2: Trig=8, Echo=9");
  Serial.println("========================================");
}

float measureDistanceMm(int trigPin, int echoPin) {
  // Limpiar el trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Enviar pulso de 10 microsegundos
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Leer la duración del pulso de eco (timeout de 30ms para evitar bloqueos)
  long duration = pulseIn(echoPin, HIGH, 30000); 
  
  if (duration == 0) {
    return -1.0; // Retorna -1 si no hay lectura (fuera de rango o desconectado)
  }
  
  // Cálculo de la distancia en milímetros:
  // Velocidad del sonido = 343 m/s = 0.343 mm/us
  // Como el sonido va y vuelve, dividimos entre 2
  return (duration * 0.343) / 2.0;
}

void loop() {
  // 1. Medir Tanque 1
  float dist1 = measureDistanceMm(TRIG_PIN_1, ECHO_PIN_1);
  
  // Pausa de 50ms entre lecturas para evitar que el eco del Tanque 1 interfiera con el Tanque 2
  delay(50); 
  
  // 2. Medir Tanque 2
  float dist2 = measureDistanceMm(TRIG_PIN_2, ECHO_PIN_2);
  
  // 3. Imprimir por UART para el Serial Plotter / Monitor
  Serial.print("Distancia_T1_mm:");
  Serial.print(dist1);
  Serial.print(",");
  Serial.print("Distancia_T2_mm:");
  Serial.println(dist2);
  
  // Tasa de muestreo (100ms adicionales + 50ms de pausa = ~6 lecturas por segundo)
  delay(100); 
}
