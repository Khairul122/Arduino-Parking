#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin definitions for both sensors
// Sensor 1 pins - First parking spot
#define TRIGGER_PIN_1 27
#define ECHO_PIN_1 26
// Sensor 2 pins - Second parking spot
#define TRIGGER_PIN_2 33
#define ECHO_PIN_2 32

// WiFi credentials
const char* ssid = "TIARA MINDA  ARIFA";
const char* password = "killualluka";  

// Separate API endpoints for each parking spot
const char* serverName1 = "http://192.168.1.21/backend-parking/parkir1_post.php";
const char* serverName2 = "http://192.168.1.21/backend-parking/parkir2_post.php";

void setup() {
  // Configure the ultrasonic sensor pins
  pinMode(TRIGGER_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(TRIGGER_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  // Start serial communication for debugging
  Serial.begin(115200);

  // Initialize WiFi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Unified function to read distance from any ultrasonic sensor
float readDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = (duration / 2.0) * 0.0343; // Convert time to distance in cm
  return distance;
}

// Function to send data to API for a specific sensor
void sendToAPI(const char* serverName, const char* sensorId, float distance) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Create JSON document for single sensor
    StaticJsonDocument<200> doc;
    JsonObject sensor = doc.createNestedObject("Ultrasonik");
    
    // Determine parking status based on distance
    sensor["id"] = sensorId;
    sensor["value"] = (distance < 7.0) ? 1 : 0;

    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send data and log the attempt
    Serial.print("Sending data to API (");
    Serial.print(sensorId);
    Serial.println("):");
    Serial.println("JSON data: " + jsonString);
    
    int httpResponseCode = http.POST(jsonString);

    // Process and display the response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print(sensorId);
      Serial.println(": Data sent successfully.");
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.print("Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error sending data for ");
      Serial.print(sensorId);
      Serial.print(". HTTP error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected. Unable to send data.");
  }
}

void loop() {
  // Read distances from both parking spots
  float distance1 = readDistance(TRIGGER_PIN_1, ECHO_PIN_1);
  float distance2 = readDistance(TRIGGER_PIN_2, ECHO_PIN_2);
  
  // Display current readings for monitoring
  Serial.println("\n--- Current Readings ---");
  Serial.print("Parking Spot 1 Distance: ");
  Serial.print(distance1);
  Serial.println(" cm");
  Serial.print("Parking Spot 2 Distance: ");
  Serial.print(distance2);
  Serial.println(" cm");
  
  // Send data to respective endpoints
  sendToAPI(serverName1, "ULTRASONIC_01", distance1);
  delay(1000); // Small delay between requests to prevent network congestion
  sendToAPI(serverName2, "ULTRASONIC_02", distance2);
  
  // Wait before next reading cycle
  delay(4000); // Total cycle time will be 5 seconds (1s + 4s)
}