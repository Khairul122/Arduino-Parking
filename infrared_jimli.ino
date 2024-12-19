#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// Nama SSID dan password Wi-Fi
const char* ssid = "TIARA MINDA  ARIFA";
const char* password = "killualluka";

// URL server
String getServerUrl = "http://192.168.1.21/backend-parking/servo_get.php";
String postServerUrl = "http://192.168.1.21/backend-parking/servo_post.php";

// Pin untuk servo
Servo myServo1;  // Servo dengan palang 1
Servo myServo2;  // Servo dengan palang 2
const int servoPin1 = 13;  // Pin untuk servo 1
const int servoPin2 = 12;  // Pin untuk servo 2

// Pin untuk sensor infrared
const int infraredPin = 15;  // Pin D15 untuk sensor infrared 1
const int infraredPin2 = 16; // Pin D16 untuk sensor infrared 2

void setup() {
  // Memulai Serial Monitor
  Serial.begin(115200);
  delay(10);

  // Setup servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  myServo1.setPeriodHertz(50);
  myServo2.setPeriodHertz(50);

  myServo1.attach(servoPin1, 500, 2400);
  myServo2.attach(servoPin2, 500, 2400);

  // Setup sensor infrared
  pinMode(infraredPin, INPUT);
  pinMode(infraredPin2, INPUT);

  // Koneksi Wi-Fi
  Serial.println();
  Serial.println("Menghubungkan ke Wi-Fi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Terhubung ke Wi-Fi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void controlServo(const char* palang, const char* value) {
  if (strcmp(palang, "1") == 0) {
    if (strcmp(value, "1") == 0) {
      myServo1.write(90);
      Serial.println("Palang 1 bergerak ke 90 derajat");
    } else if (strcmp(value, "0") == 0) {
      myServo1.write(0);
      Serial.println("Palang 1 bergerak ke 0 derajat");
    }
  } else if (strcmp(palang, "2") == 0) {
    if (strcmp(value, "1") == 0) {
      myServo2.write(90);
      Serial.println("Palang 2 bergerak ke 90 derajat");
    } else if (strcmp(value, "0") == 0) {
      myServo2.write(0);
      Serial.println("Palang 2 bergerak ke 0 derajat");
    }
  }
}

void sendInfraredValue(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(postServerUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"palang\": \"1\", \"value\": " + String(value) + "}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respons dari server:");
      Serial.println(response);
    } else {
      Serial.print("Error dalam mengirim permintaan: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi tidak terhubung!");
  }
}

void sendInfraredValueForPalang2(int value) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(postServerUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"palang\": \"2\", \"value\": " + String(value) + "}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respons dari server untuk palang 2:");
      Serial.println(response);
    } else {
      Serial.print("Error dalam mengirim permintaan untuk palang 2: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi tidak terhubung!");
  }
}

void loop() {
  // Membaca nilai dari sensor infrared 1
  int infraredValue = digitalRead(infraredPin);
  int valueToSend = (infraredValue == LOW) ? 1 : 0; // Membalik logika infrared 1

  // Membaca nilai dari sensor infrared 2
  int infraredValue2 = digitalRead(infraredPin2);
  int valueToSend2 = (infraredValue2 == LOW) ? 1 : 0; // Membalik logika infrared 2

  // Mengirim nilai infrared 1 ke server
  sendInfraredValue(valueToSend);

  // Mengirim nilai infrared 2 ke server untuk palang 2
  sendInfraredValueForPalang2(valueToSend2);

  // Mengambil data dari server untuk kontrol servo
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(getServerUrl);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respons dari server:");
      Serial.println(response);

      // Parse JSON
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        JsonArray data = doc["data"];
        for (JsonVariant v : data) {
          const char* palang = v["palang"];
          const char* value = v["value"];

          controlServo(palang, value);
        }
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
      }
    } else {
      Serial.print("Error dalam mengirim permintaan: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Wi-Fi tidak terhubung!");
  }

  delay(1000); // Tunggu 1 detik sebelum pembacaan berikutnya
}
