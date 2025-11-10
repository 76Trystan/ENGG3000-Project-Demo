#include <Arduino.h>
#include <ESP32Servo.h>  // Library for servo on ESP32

// --- Pin Definitions ---
const int trigPin = 5;     // Ultrasonic trigger
const int echoPin = 18;    // Ultrasonic echo
const int servoPin = 19;   // Servo signal

// --- Servo Configuration ---
Servo myServo;
const int servoStop = 94;       // Stop, 94 is stop point for our servo
const int servoForward = 180;   // Full-speed clockwise
const int servoReverse = 0;     // Full-speed counterclockwise
const int rotationTime = 1000;  // Duration (ms) for one full 360° rotation — tune this

// --- Variables ---
float distance = 0;
bool objectDetected = false;  // Track whether servo is in "object detected" state

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Continuous Servo + Ultrasonic Sensor starting...");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  myServo.attach(servoPin);

  myServo.write(servoStop);  // Ensure servo is stopped at startup
  Serial.println("Setup complete.\n");
}

void loop() {
  distance = getDistance();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // --- Object detected (≤ 50 cm) ---
  if (distance <= 50 && !objectDetected) {
    Serial.println("Object detected → Rotating +360° (CW)");
    rotateServoOnce(servoForward);
    objectDetected = true;
  }

  // --- Object lost (> 50 cm) ---
  if (distance > 50 && objectDetected) {
    Serial.println("Object lost → Rotating -360° (CCW)");
    rotateServoOnce(servoReverse);
    objectDetected = false;
  }

  delay(300);
}

// --- Measure distance in cm ---
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30ms
  float cm = duration * 0.034 / 2;
  return (cm == 0 || cm > 400) ? 400 : cm; // cap invalid readings
}

// --- Rotate continuous servo for one full turn ---
void rotateServoOnce(int direction) {
  myServo.write(direction);
  Serial.print("Servo rotating ");
  Serial.println(direction == servoForward ? "CW..." : "CCW...");
  delay(rotationTime);     // time for one full rotation (adjust to match your servo)
  myServo.write(servoStop);
  Serial.println("Servo stopped.\n");
}