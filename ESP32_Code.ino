#include <Arduino.h>
#include <WiFi.h>  
#include <Wire.h>
#include <Firebase_ESP_Client.h>
#include <HardwareSerial.h>
#include <DHT.h>

HardwareSerial mySerial(1);

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials (store securely)
#define WIFI_SSID "your-SSID"
#define WIFI_PASSWORD "your-PASSWORD"

// Insert Firebase project API Key (store securely)
#define API_KEY "your-API-KEY"

// Insert RTDB URL
#define DATABASE_URL "your-database-url"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

#define DHTPIN 4         // Pin where the DHT11 is connected
#define DHTTYPE DHT11    // DHT 11 sensor
#define SOIL_MOISTURE_PIN 34
#define MOTOR_PIN 33
#define ON_INDICATOR 23

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
 
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(ON_INDICATOR, OUTPUT);
 
  Serial.println("Initializing Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Sign-up OK");
    signupOK = true;
  } else {
    Serial.printf("Sign-up Error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  digitalWrite(ON_INDICATOR, HIGH);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercent = map(soilMoistureValue, 4095, 0, 0, 100);

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    if (Firebase.RTDB.setInt(&fbdo, "sensorreadings/temperature", temperature)) {
      Serial.print("Temperature: ");
      Serial.println(temperature);
    } else {
      Serial.println("Failed to send Temperature: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "sensorreadings/humidity", humidity)) {
      Serial.print("Humidity: ");
      Serial.println(humidity);
    } else {
      Serial.println("Failed to send Humidity: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "sensorreadings/soilMoisturePercent", soilMoisturePercent)) {
      Serial.print("Soil Moisture: ");
      Serial.println(soilMoisturePercent);
    } else {
      Serial.println("Failed to send Soil Moisture: " + fbdo.errorReason());
    }

    delay(1000);
  }

  if (Firebase.RTDB.getBool(&fbdo, "motorStatus")) {
    if (fbdo.dataType() == "boolean") {
      bool motorStatus = fbdo.boolData();
      digitalWrite(MOTOR_PIN, motorStatus ? HIGH : LOW);
      Serial.println(motorStatus ? "Motor ON" : "Motor OFF");
    }
  } else {
    Serial.println("Failed to get motorStatus: " + fbdo.errorReason());
  }
}
