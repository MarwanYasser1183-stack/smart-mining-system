#include <Wire.h>  // I2C library for accelerometer
#include <SPI.h>   // SPI library for SD card
#include <SD.h>    // SD card library
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC time, update every minute

// ADXL345 Accelerometer Variables
int ADXL345 = 0x53; // ADXL345 I2C address
float sensitivityFactor = 256.0; // Sensitivity for ±2g range
float X_out, Y_out, Z_out; // Outputs for accelerometer

// SD Card Variables
#define SD_CS 5  // SD Card Chip Select pin
File dataFile;   // File object for SD operations

// Variables to store parsed Arduino data
float temperature = 0.0;
float humidity = 0.0;
float dustDensity = 0.0;
int mq135 = 0;
int mq2 = 0;
int flameDetected = 0;
int earthquakeStatus = 0;
String receivedData = "";

// Earthquake detection variables
const float EARTHQUAKE_THRESHOLD = 0.48;
int earthquakeDetectionCounter = 0;
const int EARTHQUAKE_DETECTION_WINDOW = 5;

// Pitch tracking variables
float previousPitch = 0.0;
float pitchChangeAbsolute = 0.0;

// WiFi credentials — replace with your own
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase credentials — replace with your own
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define FIREBASE_PROJECT_ID "YOUR_FIREBASE_PROJECT_ID"

// Firebase data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Serial2 pins for Arduino communication
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (true);
  }
  Serial.println("SD card initialized successfully.");

  if (!SD.exists("/Downloads")) {
    SD.mkdir("/Downloads");
    Serial.println("Created folder: /Downloads");
  }

  dataFile = SD.open("/Downloads/sensor_data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Timestamp,Temperature,Humidity,DustDensity,MQ135,MQ2,FlameDetected,X_out,Y_out,Z_out,Pitch,Roll,PitchChangeAbsolute,EarthquakeStatus");
    dataFile.close();
  } else {
    Serial.println("Error creating file!");
  }

  Wire.begin(21, 22);
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D);
  Wire.write(8);
  Wire.endTransmission();
  delay(100);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(0);

  config.api_key = API_KEY;
  auth.user.email = "YOUR_EMAIL@gmail.com";
  auth.user.password = "YOUR_PASSWORD";
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  timeClient.update();

  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      Serial.println("Received: " + receivedData);
      parseAndSave(receivedData);
      receivedData = "";
    } else {
      receivedData += c;
    }
  }

  readAccelerometer();
  detectEarthquake();
  saveToSD();
  uploadSensorDataToFirestore();

  delay(5000);
}

void detectEarthquake() {
  float acceleration = sqrt(X_out * X_out + Y_out * Y_out + Z_out * Z_out);
  if (abs(acceleration - 0.35) > EARTHQUAKE_THRESHOLD) {
    earthquakeDetectionCounter++;
    if (earthquakeDetectionCounter >= EARTHQUAKE_DETECTION_WINDOW) {
      earthquakeStatus = 2;
      Serial.println("Earthquake detected!");
    } else {
      earthquakeStatus = 1;
    }
  } else {
    earthquakeDetectionCounter = 0;
    earthquakeStatus = 0;
  }
}

void readAccelerometer() {
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);

  int16_t X_raw = (Wire.read() | Wire.read() << 8);
  X_out = X_raw / sensitivityFactor;
  int16_t Y_raw = (Wire.read() | Wire.read() << 8);
  Y_out = Y_raw / sensitivityFactor;
  int16_t Z_raw = (Wire.read() | Wire.read() << 8);
  Z_out = Z_raw / sensitivityFactor;
}

void parseAndSave(String data) {
  int commaIndex = 0;
  int previousIndex = 0;
  int variableIndex = 0;
  while ((commaIndex = data.indexOf(',', previousIndex)) != -1) {
    storeVariable(data.substring(previousIndex, commaIndex), variableIndex);
    previousIndex = commaIndex + 1;
    variableIndex++;
  }
  storeVariable(data.substring(previousIndex), variableIndex);
}

void storeVariable(String value, int index) {
  switch (index) {
    case 0: temperature = value.toFloat(); break;
    case 1: humidity = value.toFloat(); break;
    case 2: dustDensity = value.toFloat(); break;
    case 3: mq135 = value.toInt(); break;
    case 4: mq2 = value.toInt(); break;
    case 5: flameDetected = value.toInt(); break;
    default: Serial.println("Error: Unexpected data format.");
  }
}

void saveToSD() {
  float pitch = atan2(Y_out, sqrt(X_out * X_out + Z_out * Z_out)) * 180 / PI;
  float roll = atan2(X_out, sqrt(Y_out * Y_out + Z_out * Z_out)) * 180 / PI;
  pitchChangeAbsolute = abs(pitch - previousPitch);
  previousPitch = pitch;

  unsigned long epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t *)&epochTime);
  char timestamp[30];
  snprintf(timestamp, sizeof(timestamp), "%d-%02d-%02d-%02d-%02d-%02d",
    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
    ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  dataFile = SD.open("/Downloads/sensor_data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(timestamp); dataFile.print(",");
    dataFile.print(temperature); dataFile.print(",");
    dataFile.print(humidity); dataFile.print(",");
    dataFile.print(dustDensity); dataFile.print(",");
    dataFile.print(mq135); dataFile.print(",");
    dataFile.print(mq2); dataFile.print(",");
    dataFile.print(flameDetected); dataFile.print(",");
    dataFile.print(X_out, 2); dataFile.print(",");
    dataFile.print(Y_out, 2); dataFile.print(",");
    dataFile.print(Z_out, 2); dataFile.print(",");
    dataFile.print(pitch, 2); dataFile.print(",");
    dataFile.print(roll, 2); dataFile.print(",");
    dataFile.print(pitchChangeAbsolute, 2); dataFile.print(",");
    dataFile.println(earthquakeStatus);
    dataFile.close();
    Serial.println("Data saved to /Downloads/sensor_data.csv.");
  } else {
    Serial.println("Error writing to file!");
  }
}

void uploadSensorDataToFirestore() {
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t *)&epochTime);
  char timestamp[30];
  snprintf(timestamp, sizeof(timestamp), "%d-%02d-%02d-%02d-%02d-%02d",
    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
    ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  FirebaseJson content;
  content.set("fields/timestamp/stringValue", timestamp);
  content.set("fields/temperature/doubleValue", temperature);
  content.set("fields/humidity/doubleValue", humidity);
  content.set("fields/dustDensity/doubleValue", dustDensity);
  content.set("fields/mq135/integerValue", mq135);
  content.set("fields/mq2/integerValue", mq2);
  content.set("fields/flameDetected/integerValue", flameDetected);
  content.set("fields/earthquakeStatus/integerValue", earthquakeStatus);
  content.set("fields/accelerometer/mapValue/fields/x/doubleValue", X_out);
  content.set("fields/accelerometer/mapValue/fields/y/doubleValue", Y_out);
  content.set("fields/accelerometer/mapValue/fields/z/doubleValue", Z_out);

  float pitch = atan2(Y_out, sqrt(X_out * X_out + Z_out * Z_out)) * 180 / PI;
  float roll = atan2(X_out, sqrt(Y_out * Y_out + Z_out * Z_out)) * 180 / PI;
  content.set("fields/orientation/mapValue/fields/pitch/doubleValue", pitch);
  content.set("fields/orientation/mapValue/fields/roll/doubleValue", roll);
  content.set("fields/orientation/mapValue/fields/pitchChangeAbsolute/doubleValue", pitchChangeAbsolute);

  String documentPath = "sensor_data/" + String(timestamp);
  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath, content.raw())) {
    Serial.printf("Uploaded to Firestore: %s\n", timestamp);
  } else {
    Serial.printf("Firestore upload failed: %s\n", fbdo.errorReason().c_str());
  }
}
