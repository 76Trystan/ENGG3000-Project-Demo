#include <Arduino.h>

//MOTOR CONTROL
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 25;

//ROAD TRAFFIC LIGHTS
int redLEDPin_R = 32;    
int yellowLEDPin_R = 35; 
int greenLEDPin_R = 34;

//BOAT TRAFFIC LIGHTS
int redLEDPin_B = 16;
int yellowLEDPin_B = 17;
int greenLEDPin_B = 18;

// Light Sensor LED 
int whiteLEDPin = 19;

//ULTRASONIC SENSORS
const int trigPin_A = 2;  
const int echoPin_A = 15; 
const int trigPin_B = 4;  
const int echoPin_B = 5;  

//VARIABLES
float distanceA = 0;
float distanceB = 0;
bool boatDetected = false;

//TINING CONTROL
const unsigned long ROTATION_DURATION = 4000; // adjust as needed
const unsigned long STOP_DURATION = 2000;
unsigned long rotationStartTime = 0;
unsigned long yellowStartTime = 0;
unsigned long boatClearTime = 0;

enum MotorState {
  IDLE,            // Bridge closed, road open
  ROAD_WARNING,    // Yellow light for road (new)
  BOAT_WARNING,    // Yellow for boat before opening
  BRIDGE_OPENING,  // Bridge lifting
  BRIDGE_OPEN,     // Bridge lifted
  BRIDGE_CLOSING   // Bridge lowering
};

MotorState currentState = IDLE;

// PWM setup
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

// SETUP
void setup() {
  Serial.begin(115200);
  Serial.println("Setup: Bridge + Boat Traffic Control System");

  // Motor pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  // Ultrasonic sensors
  pinMode(trigPin_A, OUTPUT);
  pinMode(echoPin_A, INPUT);
  pinMode(trigPin_B, OUTPUT);
  pinMode(echoPin_B, INPUT);

  // Traffic lights
  pinMode(redLEDPin_R, OUTPUT);
  pinMode(yellowLEDPin_R, OUTPUT);
  pinMode(greenLEDPin_R, OUTPUT);
  pinMode(redLEDPin_B, OUTPUT);
  pinMode(yellowLEDPin_B, OUTPUT);
  pinMode(greenLEDPin_B, OUTPUT);

  // Light Sesnsor Light
  pinMode(whiteLEDPin, OUTPUT);
  digitalWrite(whiteLEDPin, HIGH); // Turn on white LED for light sensor

  // PWM setup
  ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel);

  // Initial light state: road green, boat red
  setLights(true, false, false,   // Road: Green
            true, false, false);  // Boat: Red
}

// Flash the boat yellow LED (used during open/close warnings)
void flashBoatYellow() {
  static unsigned long lastFlashTime = 0;
  static bool yellowState = false;

  if (millis() - lastFlashTime >= 500) { // toggle every 0.5 seconds
    yellowState = !yellowState;
    digitalWrite(yellowLEDPin_B, yellowState);
    lastFlashTime = millis();
  }
}


// MAIN LOOP
void loop() {
  distanceA = getDistance(trigPin_A, echoPin_A);
  distanceB = getDistance(trigPin_B, echoPin_B);

  Serial.print("Sensor A: ");
  Serial.print(distanceA);
  Serial.print(" cm | Sensor B: ");
  Serial.print(distanceB);
  Serial.print(" cm | State: ");

  unsigned long currentTime = millis();

  switch (currentState) {
    case IDLE:
      Serial.println("IDLE");
      // Lights: Road green, Boat red
      setLights(true, false, false,  // Road: Green
                true, false, false); // Boat: Red

      // Detect boat
      if (distanceA <= 50 || distanceB <= 50) {
        Serial.println("Boat detected: Warn road first");
        yellowStartTime = currentTime;
        currentState = ROAD_WARNING;
      }
      break;

    case ROAD_WARNING:
      Serial.println("ROAD_WARNING (10s yellow for road)");
      // Lights: Road yellow, Boat red
      setLights(false, false, true,  // Road: Yellow
                true, false, false); // Boat: Red

      if (currentTime - yellowStartTime >= 10000) {
        Serial.println("Road warned → Now warn boat");
        yellowStartTime = currentTime;
        currentState = BOAT_WARNING;
      }
      break;

    case BOAT_WARNING:
      Serial.println("BOAT_WARNING (3s flashing yellow for boat)");
      // Road: red on
      digitalWrite(redLEDPin_R, HIGH);
      digitalWrite(yellowLEDPin_R, LOW);
      digitalWrite(greenLEDPin_R, LOW);

      // Boat: red ON, yellow FLASHING
      digitalWrite(redLEDPin_B, HIGH);
      flashBoatYellow();  // flash yellow for 3s warning
      digitalWrite(greenLEDPin_B, LOW);

      if (currentTime - yellowStartTime >= 3000) {
        Serial.println("Boat warned: Opening bridge");
        rotateForward();
        rotationStartTime = currentTime;
        currentState = BRIDGE_OPENING;
        digitalWrite(yellowLEDPin_B, LOW); // turn yellow off after warning
      }
      break;

    case BRIDGE_OPENING:
      Serial.println("BRIDGE_OPENING (bridge moving up)");
      // Lights: both sides red, boat yellow flashing
      digitalWrite(redLEDPin_R, HIGH);
      digitalWrite(yellowLEDPin_R, LOW);
      digitalWrite(greenLEDPin_R, LOW);

      digitalWrite(redLEDPin_B, HIGH);
      flashBoatYellow(); // Flash while moving
      digitalWrite(greenLEDPin_B, LOW);

      if (currentTime - rotationStartTime >= ROTATION_DURATION) {
        stopMotor();
        Serial.println("Bridge fully open: Boat green LED ON");
        setLights(false, true, false,   // Road: Red
                  false, false, true);  // Boat: Green
        digitalWrite(yellowLEDPin_B, LOW); // turn off flash
        currentState = BRIDGE_OPEN;
      }
      break;

    case BRIDGE_OPEN:
        Serial.println("BRIDGE_OPEN");
        // Lights: Road red, Boat green
        setLights(false, true, false,
            false, false, true);

  // Check if boat has left
  if (distanceA > 70 && distanceB > 70) {
    if (boatClearTime == 0) {
      boatClearTime = currentTime; // Start 6s timer
      Serial.println("No boat detected: Starting 8s wait before closing");
    } else if (currentTime - boatClearTime >= 8000) {
      Serial.println("8s wait complete: Start closing sequence");
      yellowStartTime = currentTime;
      currentState = BRIDGE_CLOSING;
      boatClearTime = 0; // Reset timer
    }
  } else {
    // Boat still detected: resetting timer
    boatClearTime = 0;
  }
  break;


    case BRIDGE_CLOSING:
      Serial.println("BRIDGE_CLOSING (3s flashing yellow for boat)");
      // Road: red on
      digitalWrite(redLEDPin_R, HIGH);
      digitalWrite(yellowLEDPin_R, LOW);
      digitalWrite(greenLEDPin_R, LOW);

      // Boat: red ON, yellow FLASHING
      digitalWrite(redLEDPin_B, HIGH);
      flashBoatYellow();
      digitalWrite(greenLEDPin_B, LOW);

      if (currentTime - yellowStartTime >= 3000) {
        Serial.println("Closing bridge now");
        rotateBackward();
        rotationStartTime = currentTime;

        while (millis() - rotationStartTime < ROTATION_DURATION) {
          delay(10);
        }

        stopMotor();
        digitalWrite(yellowLEDPin_B, LOW); // stop flashing after done
        Serial.println("Bridge closed: Back to IDLE");
        currentState = IDLE;
      }
      break;
  }

  delay(100);
}

// FUNCTIONS 

// Generalized distance function
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  float cm = duration * 0.034 / 2;
  return (cm == 0 || cm > 400) ? 400 : cm;
}

// Helper to control LED states
void setLights(bool roadGreen, bool roadRed, bool roadYellow,
               bool boatRed, bool boatYellow, bool boatGreen) {
  digitalWrite(greenLEDPin_R, roadGreen ? HIGH : LOW);
  digitalWrite(redLEDPin_R, roadRed ? HIGH : LOW);
  digitalWrite(yellowLEDPin_R, roadYellow ? HIGH : LOW);

  digitalWrite(redLEDPin_B, boatRed ? HIGH : LOW);
  digitalWrite(yellowLEDPin_B, boatYellow ? HIGH : LOW);
  digitalWrite(greenLEDPin_B, boatGreen ? HIGH : LOW);
}

// Rotate motor forward (open bridge)
void rotateForward() {
  ledcWrite(enable1Pin, dutyCycle);
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
}

// Rotate motor backward (close bridge)
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
