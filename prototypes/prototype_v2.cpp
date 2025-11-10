#include <Arduino.h>
// This code runs a program where an object detected within 50cm triggers motor rotation
// for a set duration. When object is lost, motor stops for 2 seconds, then rotates
// in the opposite direction for the same duration.

// Motor Pins
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;

// Ultra-Sonic Pins
const int trigPin = 5;  // Ultrasonic trigger
const int echoPin = 18; // Ultrasonic echo

// Ultra-sonic Variables
float distance = 0;
bool objectDetected = false;

// Motor control variables
const unsigned long ROTATION_DURATION = 3000; // 3 seconds rotation time (adjust as needed)
const unsigned long STOP_DURATION = 2000;      // 2 seconds stop time
unsigned long rotationStartTime = 0;
unsigned long stopStartTime = 0;

enum MotorState {
  IDLE,
  ROTATING_FORWARD,
  STOPPED_AFTER_DETECTION,
  ROTATING_BACKWARD
};

MotorState currentState = IDLE;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

void setup() {
  // sets the pins as outputs for DC Motor
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  // sets the pins for ultra-sonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // configure LEDC PWM
  ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel);
  
  Serial.begin(115200);
  Serial.println("Setup DC Motor with Ultra-Sonic Sensors");
}

void loop() {
  // Measure distance
  distance = getDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | State: ");
  
  unsigned long currentTime = millis();
  
  switch (currentState) {
    case IDLE:
      Serial.println("IDLE");
      // Check if object is detected
      if (distance <= 50) {
        Serial.println("Object detected → Starting forward rotation");
        rotateForward();
        rotationStartTime = currentTime;
        currentState = ROTATING_FORWARD;
        objectDetected = true;
      }
      break;
      
    case ROTATING_FORWARD:
      Serial.println("ROTATING_FORWARD");
      // Check if rotation duration has elapsed
      if (currentTime - rotationStartTime >= ROTATION_DURATION) {
        Serial.println("Forward rotation complete → Checking for object");
        stopMotor();
        
        // Check if object is still there
        if (distance > 50) {
          Serial.println("Object lost → Starting 2-second stop");
          stopStartTime = currentTime;
          currentState = STOPPED_AFTER_DETECTION;
          objectDetected = false;
        } else {
          // Object still detected, continue rotating forward
          Serial.println("Object still present → Continue forward rotation");
          rotationStartTime = currentTime;
        }
      }
      break;
      
    case STOPPED_AFTER_DETECTION:
      Serial.println("STOPPED (2s wait)");
      // Wait for stop duration
      if (currentTime - stopStartTime >= STOP_DURATION) {
        Serial.println("Stop complete → Starting backward rotation");
        rotateBackward();
        rotationStartTime = currentTime;
        currentState = ROTATING_BACKWARD;
      }
      break;
      
    case ROTATING_BACKWARD:
      Serial.println("ROTATING_BACKWARD");
      // Check if rotation duration has elapsed
      if (currentTime - rotationStartTime >= ROTATION_DURATION) {
        Serial.println("Backward rotation complete → Returning to IDLE");
        stopMotor();
        currentState = IDLE;
      }
      break;
  }
  
  delay(100); // Small delay for stability
}

// Measure distance in cm
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  float cm = duration * 0.034 / 2;
  return (cm == 0 || cm > 400) ? 400 : cm; // cap invalid readings
}

// Rotate motor forward
void rotateForward() {
  ledcWrite(enable1Pin, dutyCycle);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
}

// Rotate motor backward
void rotateBackward() {
  ledcWrite(enable1Pin, dutyCycle);
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
}

// Stop motor
void stopMotor() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
}

