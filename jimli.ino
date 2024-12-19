#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi credentials
const char* ssid = "TIARA MINDA  ARIFA";
const char* password = "killualluka";

// Server URLs
const char* serverName1 = "http://192.168.1.21/backend-parking/parkir1_post.php";
const char* serverName2 = "http://192.168.1.21/backend-parking/parkir2_post.php";
String getServerUrl = "http://192.168.1.21/backend-parking/servo_get.php";
String postServerUrl = "http://192.168.1.21/backend-parking/servo_post.php";

// Pin definitions for ultrasonic sensors
#define TRIGGER_PIN_1 27
#define ECHO_PIN_1 26
#define TRIGGER_PIN_2 33
#define ECHO_PIN_2 32

// Pin definitions for servo motors
const int servoPin1 = 13;
const int servoPin2 = 12;

// Pin definitions for infrared sensors
const int infraredPin = 15;
const int infraredPin2 = 16;

// Servo objects
Servo myServo1;
Servo myServo2;

void setup() {
  Serial.begin(115200);
  delay(10);

  // Configure ultrasonic sensor pins
  pinMode(TRIGGER_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIGGER_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Configure infrared sensor pins
  pinMode(infraredPin, INPUT);
  pinMode(infraredPin2, INPUT);

  // Setup servo motors
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  myServo1.setPeriodHertz(50);
  myServo2.setPeriodHertz(50);
  
  myServo1.attach(servoPin1, 500, 2400);
  myServo2.attach(servoPin2, 500, 2400);

  // Initialize WiFi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Ultrasonic sensor functions
float readDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = (duration / 2.0) * 0.0343;
  return distance;
}

void sendToAPI(const char* serverName, const char* sensorId, float distance) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> doc;
    JsonObject sensor = doc.createNestedObject("Ultrasonik");
    sensor["id"] = sensorId;
    sensor["value"] = (distance < 7.0) ? 1 : 0;

    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpResponseCode = http.POST(jsonString);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("%s: Data sent successfully. Response code: %d\n", sensorId, httpResponseCode);
      Serial.println(response);
    } else {
      Serial.printf("Error sending data for %s. Error code: %d\n", sensorId, httpResponseCode);
    }

    http.end();
  }
}

// Servo and barrier functions
void controlServo(const char* palang, const char* value) {
  if (strcmp(palang, "1") == 0) {
    myServo1.write(strcmp(value, "1") == 0 ? 90 : 0);
    Serial.printf("Palang 1 bergerak ke %d derajat\n", strcmp(value, "1") == 0 ? 90 : 0);
  } else if (strcmp(palang, "2") == 0) {
    myServo2.write(strcmp(value, "1") == 0 ? 90 : 0);
    Serial.printf("Palang 2 bergerak ke %d derajat\n", strcmp(value, "1") == 0 ? 90 : 0);
  }
}

void sendInfraredValue(const char* palang, int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(postServerUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"palang\": \"" + String(palang) + "\", \"value\": " + String(value) + "}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("Respons dari server untuk palang %s:\n%s\n", palang, response.c_str());
    } else {
      Serial.printf("Error dalam mengirim permintaan untuk palang %s: %d\n", palang, httpResponseCode);
    }

    http.end();
  }
}

void checkAndUpdateBarriers() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(getServerUrl);

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String response = http.getString();
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        JsonArray data = doc["data"];
        for (JsonVariant v : data) {
          controlServo(v["palang"], v["value"]);
        }
      } else {
        Serial.println("JSON parsing failed: " + String(error.f_str()));
      }
    }
    http.end();
  }
}

void loop() {
  // Read and process ultrasonic sensor data
  float distance1 = readDistance(TRIGGER_PIN_1, ECHO_PIN_1);
  float distance2 = readDistance(TRIGGER_PIN_2, ECHO_PIN_2);
  
  Serial.println("\n--- Parking Spot Readings ---");
  Serial.printf("Spot 1: %.2f cm\nSpot 2: %.2f cm\n", distance1, distance2);
  
  sendToAPI(serverName1, "ULTRASONIC_01", distance1);
  sendToAPI(serverName2, "ULTRASONIC_02", distance2);

  // Read and process infrared sensor data
  int infrared1 = digitalRead(infraredPin);
  int infrared2 = digitalRead(infraredPin2);
  
  sendInfraredValue("1", infrared1 == LOW ? 1 : 0);
  sendInfraredValue("2", infrared2 == LOW ? 1 : 0);

  // Check and update barrier positions
  checkAndUpdateBarriers();

  // Delay before next cycle
  delay(1000);
}