#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Wifi credentials
static const char *WIFI_SSID = "Group64";
static const char *WIFI_PASS = "64GroupProject";

// Motor pins
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 25; // PWM (LEDC) pin

// Road Traffic Lights Pins
int redLEDPin_R = 32;
int yellowLEDPin_R = 35;
int greenLEDPin_R = 34;

// Boat Traffic Lights Pins
int redLEDPin_B = 16;
int yellowLEDPin_B = 17;
int greenLEDPin_B = 18;

// Light Sensor LED
int whiteLEDPin = 19;

// Ultrasonic Sensor Pins
const int trigPin_A = 2;
const int echoPin_A = 15;
const int trigPin_B = 4;
const int echoPin_B = 5;

// Distance Variables
float distanceA = 0;
float distanceB = 0;
bool boatDetected = false;

// timings variables (ms)
const unsigned long ROTATION_DURATION = 4000; // 4s motor move (open/close)
const unsigned long ROAD_WARNING_MS = 3000;   // 3s road yellow
const unsigned long BOAT_WARNING_MS = 3000;   // 3s boat yellow flashing
const unsigned long CLEAR_WINDOW_MS = 6000;   // 6s "no boat" before closing
const unsigned long STOP_DURATION = 2000;     // (not used in this flow)

unsigned long rotationStartTime = 0;
unsigned long yellowStartTime = 0;
unsigned long boatClearTime = 0;

enum MotorState
{
  IDLE,           // Bridge down, road open
  ROAD_WARNING,   // 3s road yellow
  BOAT_WARNING,   // 3s boat yellow flashing (with boat red)
  BRIDGE_OPENING, // motor up
  BRIDGE_OPEN,    // boat green
  BRIDGE_CLOSING  // 3s boat yellow flashing + motor down
};
MotorState currentState = IDLE;

// Manual override (UI)
bool manualMode = false; // Auto by default

// PWM (LEDC) Variables 
const int pwmChannel = 0;
const int pwmFreq = 30000;
const int pwmResBits = 8;
int dutyCycle = 200; // 0..255

// Web server
AsyncWebServer server(80);

// Light state for UI mirroring
bool uiRoadRed = false;
bool uiRoadYellow = false;
bool uiRoadGreen = true;
bool uiBoatRed = true;
bool uiBoatYellow = false;
bool uiBoatGreen = false;

void applyRoad(bool R, bool Y, bool G)
{
  uiRoadRed = R;
  uiRoadYellow = Y;
  uiRoadGreen = G;
  digitalWrite(redLEDPin_R, R);
  digitalWrite(yellowLEDPin_R, Y);
  digitalWrite(greenLEDPin_R, G);
}

void applyBoat(bool R, bool Y, bool G)
{
  uiBoatRed = R;
  uiBoatYellow = Y;
  uiBoatGreen = G;
  digitalWrite(redLEDPin_B, R);
  digitalWrite(yellowLEDPin_B, Y);
  digitalWrite(greenLEDPin_B, G);
}

// Helper signature preserved (mapping into applyRoad/applyBoat)
void setLights(bool roadGreen, bool roadRed, bool roadYellow,
               bool boatRed, bool boatYellow, bool boatGreen)
{
  applyRoad(roadRed, roadYellow, roadGreen);
  applyBoat(boatRed, boatYellow, boatGreen);
}

// Motor PWM control
void motorPWM(int duty)
{
  ledcWrite(pwmChannel, constrain(duty, 0, 255));
}

void stopMotor()
{
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  motorPWM(0);
}

// Rotate motor forward (open bridge)
void rotateForward()
{
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  motorPWM(dutyCycle);
}

// Rotate motor backward (close bridge)
void rotateBackward()
{
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  motorPWM(dutyCycle);
}

// Generalized distance
float getDistance(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  float cm = duration * 0.034f / 2.0f;
  return (cm == 0 || cm > 400) ? 400.0f : cm;
}

// Boat yellow flasher; also mirrors state for UI
void flashBoatYellow()
{
  static unsigned long lastFlashTime = 0;
  static bool yellowState = false;
  if (millis() - lastFlashTime >= 500)
  {
    yellowState = !yellowState;
    digitalWrite(yellowLEDPin_B, yellowState);
    uiBoatYellow = yellowState; // mirror to UI
    lastFlashTime = millis();
  }
}

// State -> string for UI
const char *stateString(MotorState s)
{
  switch (s)
  {
  case IDLE:
    return "IDLE";
  case ROAD_WARNING:
    return "ROAD_WARNING";
  case BOAT_WARNING:
    return "BOAT_WARNING";
  case BRIDGE_OPENING:
    return "OPENING";
  case BRIDGE_OPEN:
    return "OPEN";
  case BRIDGE_CLOSING:
    return "CLOSING";
  }
  return "UNKNOWN";
}

// HTTP Routes 
void setupRoutes()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // Mode GET
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    String json = String("{\"value\":\"") + (manualMode ? "manual" : "auto") + "\"}";
    req->send(200, "application/json", json); });

  // Mode POST
  server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *req)
            {
    if (!req->hasParam("value")) {
      req->send(400, "text/plain", "missing value");
      return;
    }
    String v = req->getParam("value")->value();
    v.toLowerCase();
    if (v == "manual")      manualMode = true;
    else if (v == "auto")   manualMode = false;
    else {
      req->send(400, "text/plain", "invalid value");
      return;
    }
    stopMotor();
    currentState = IDLE;
    req->send(200, "text/plain", "OK"); });

  // Manual open/close (only if manualMode)
  server.on("/led/on", HTTP_ANY, [](AsyncWebServerRequest *req) { // OPEN
    if (!manualMode)
    {
      req->send(403, "text/plain", "Manual mode required");
      return;
    }
    rotateForward();
    rotationStartTime = millis();
    currentState = BRIDGE_OPENING;
    req->send(200, "text/plain", "OPENING");
  });

  server.on("/led/off", HTTP_ANY, [](AsyncWebServerRequest *req) { // CLOSE
    if (!manualMode)
    {
      req->send(403, "text/plain", "Manual mode required");
      return;
    }
    rotateBackward();
    rotationStartTime = millis();
    currentState = BRIDGE_CLOSING;
    req->send(200, "text/plain", "CLOSING");
  });

  server.on("/stop", HTTP_ANY, [](AsyncWebServerRequest *req)
            {
    stopMotor();
    currentState = IDLE;
    req->send(200, "text/plain", "STOPPED"); });

  // Live measurements
  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    float a = getDistance(trigPin_A, echoPin_A);
    float b = getDistance(trigPin_B, echoPin_B);
    String json = String("{\"A\":") + String(a, 1) + ",\"B\":" + String(b, 1) + "}";
    req->send(200, "application/json", json); });

  // Traffic light mirror for UI
  server.on("/lights", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    String json = String("{\"road\":{\"red\":")  + (uiRoadRed ? 1 : 0) +
                  ",\"yellow\":" + (uiRoadYellow ? 1 : 0) +
                  ",\"green\":"  + (uiRoadGreen ? 1 : 0) +
                  "},\"boat\":{\"red\":" + (uiBoatRed ? 1 : 0) +
                  ",\"yellow\":" + (uiBoatYellow ? 1 : 0) +
                  ",\"green\":"  + (uiBoatGreen ? 1 : 0) + "}}";
    req->send(200, "application/json", json); });

  // Bridge state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    String json = String("{\"state\":\"") + stateString(currentState) + "\"}";
    req->send(200, "application/json", json); });

  // Timers endpoint: remaining time for road + boat phases
  server.on("/timers", HTTP_GET, [](AsyncWebServerRequest *req)
            {
    unsigned long now = millis();

    long roadRemainMs = 0;
    long boatRemainMs = 0;

    // ROAD timer: only during ROAD_WARNING
    if (currentState == ROAD_WARNING) {
      unsigned long elapsed = now - yellowStartTime;
      if (elapsed < ROAD_WARNING_MS) {
        roadRemainMs = (long)(ROAD_WARNING_MS - elapsed);
      }
    }

    // BOAT / BRIDGE timer
    if (currentState == BOAT_WARNING) {
      // 3s boat yellow before opening
      unsigned long elapsed = now - yellowStartTime;
      if (elapsed < BOAT_WARNING_MS) {
        boatRemainMs = (long)(BOAT_WARNING_MS - elapsed);
      }
    } else if (currentState == BRIDGE_OPENING) {
      // time left to fully open
      unsigned long elapsed = now - rotationStartTime;
      if (elapsed < ROTATION_DURATION) {
        boatRemainMs = (long)(ROTATION_DURATION - elapsed);
      }
    } else if (currentState == BRIDGE_OPEN && boatClearTime != 0) {
      // clear-window countdown before closing (6s)
      unsigned long elapsed = now - boatClearTime;
      if (elapsed < CLEAR_WINDOW_MS) {
        boatRemainMs = (long)(CLEAR_WINDOW_MS - elapsed);
      }
    } else if (currentState == BRIDGE_CLOSING) {
      // 3s boat warning, then motor down for ROTATION_DURATION
      unsigned long warnElapsed = now - yellowStartTime;

      if (warnElapsed < BOAT_WARNING_MS) {
        boatRemainMs = (long)(BOAT_WARNING_MS - warnElapsed);   // still warning
      } else {
        // in the closing movement phase
        unsigned long moveElapsed = now - rotationStartTime;
        if (moveElapsed < ROTATION_DURATION) {
          boatRemainMs = (long)(ROTATION_DURATION - moveElapsed);
        }
      }
    }

    String json = "{";
    json += "\"road\":{\"remaining_ms\":" + String(roadRemainMs) + "},";
    json += "\"boat\":{\"remaining_ms\":" + String(boatRemainMs) + "}";
    json += "}";

    req->send(200, "application/json", json); });

  // Serve UI
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *req)
                    {
    if (req->method() == HTTP_OPTIONS) req->send(200);
    else req->send(404, "text/plain", "Not found"); });
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Bridge + Boat Traffic (WiFi + UI mirror)");

  // Pins
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  ledcSetup(pwmChannel, pwmFreq, pwmResBits);
  ledcAttachPin(enable1Pin, pwmChannel);
  stopMotor();

  pinMode(redLEDPin_R, OUTPUT);
  pinMode(yellowLEDPin_R, OUTPUT);
  pinMode(greenLEDPin_R, OUTPUT);
  pinMode(redLEDPin_B, OUTPUT);
  pinMode(yellowLEDPin_B, OUTPUT);
  pinMode(greenLEDPin_B, OUTPUT);

  pinMode(trigPin_A, OUTPUT);
  pinMode(echoPin_A, INPUT);
  pinMode(trigPin_B, OUTPUT);
  pinMode(echoPin_B, INPUT);

  pinMode(whiteLEDPin, OUTPUT);
  digitalWrite(whiteLEDPin, HIGH); // turn on

  // Initial: road green, boat red
  setLights(true, false, false, true, false, false);

  // SPIFFS
  if (!SPIFFS.begin(true))
    Serial.println("SPIFFS mount failed");
  else
    Serial.println("SPIFFS mounted");

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000)
  {
    Serial.print('.');
    delay(300);
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi timeout, rebooting in 3s");
    delay(3000);
    ESP.restart();
  }
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());

  setupRoutes();
  server.begin();
  Serial.println("HTTP server started");
}

void loop() // Automatic Mode (state machine)
{
  // Sample distances
  static unsigned long tSense = 0;
  unsigned long now = millis();
  if (now - tSense >= 120)
  {
    distanceA = getDistance(trigPin_A, echoPin_A);
    distanceB = getDistance(trigPin_B, echoPin_B);
    tSense = now;
  }

  // AUTO state machine
  if (!manualMode)
  {
    switch (currentState)
    {
    case IDLE:
      // Road GREEN, Boat RED
      setLights(true, false, false, true, false, false);
      if (distanceA <= 50 || distanceB <= 50)
      {
        yellowStartTime = now;
        currentState = ROAD_WARNING;
        Serial.println("Boat detected → ROAD_WARNING");
      }
      break;

    case ROAD_WARNING:
      // Road YELLOW, Boat RED
      setLights(false, false, true, true, false, false);
      if (now - yellowStartTime >= ROAD_WARNING_MS)
      {
        yellowStartTime = now;
        currentState = BOAT_WARNING;
        Serial.println("Road warned → BOAT_WARNING");
      }
      break;

    case BOAT_WARNING:
      // Road RED; Boat RED + YELLOW flashing
      applyRoad(true, false, false); // road red on
      applyBoat(true, false, false); // boat red on
      flashBoatYellow();             // flash boat yellow
      if (now - yellowStartTime >= BOAT_WARNING_MS)
      {
        rotateForward();
        rotationStartTime = now;
        digitalWrite(yellowLEDPin_B, LOW);
        uiBoatYellow = false;
        currentState = BRIDGE_OPENING;
        Serial.println("Opening bridge");
      }
      break;

    case BRIDGE_OPENING:
      // Both RED; boat yellow flashing while moving
      applyRoad(true, false, false);
      applyBoat(true, false, false);
      flashBoatYellow();
      if (now - rotationStartTime >= ROTATION_DURATION)
      {
        stopMotor();
        // Road RED, Boat GREEN
        setLights(false, true, false, false, false, true);
        uiBoatYellow = false;
        digitalWrite(yellowLEDPin_B, LOW);
        currentState = BRIDGE_OPEN;
        Serial.println("Bridge open → Boat GREEN");
      }
      break;

    case BRIDGE_OPEN:
      // Road RED, Boat GREEN
      setLights(false, true, false, false, false, true);
      // If no boat for 6s → close
      if (distanceA > 70 && distanceB > 70)
      {
        if (boatClearTime == 0)
        {
          boatClearTime = now;
          Serial.println("No boat → 6s wait before closing");
        }
        else if (now - boatClearTime >= CLEAR_WINDOW_MS)
        {
          yellowStartTime = now;
          currentState = BRIDGE_CLOSING;
          boatClearTime = 0;
          Serial.println("Closing sequence start");
        }
      }
      else
      {
        boatClearTime = 0;
      }
      break;

    case BRIDGE_CLOSING:
      // Road RED; Boat RED + YELLOW flashing 3s then motor down
      applyRoad(true, false, false);
      applyBoat(true, false, false);
      flashBoatYellow();
      if (now - yellowStartTime >= BOAT_WARNING_MS)
      {
        rotateBackward();
        rotationStartTime = now;
        while (millis() - rotationStartTime < ROTATION_DURATION)
        {
          flashBoatYellow(); // keep flashing while moving
          delay(10);
        }
        stopMotor();
        uiBoatYellow = false;
        digitalWrite(yellowLEDPin_B, LOW);
        currentState = IDLE;
        Serial.println("Bridge closed → IDLE");
      }
      break;
    }
  }

  delay(50);
}
