#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Pin Definitions
#define DHTPIN 2
#define DHTTYPE DHT22
#define GP2Y1014_LED 12
#define GP2Y1014_AOUT A0
#define MQ135_PIN A2
#define MQ2_PIN A1
#define FLAME_SENSOR_PIN 3

DHT dht(DHTPIN, DHTTYPE);
float dustDensity = 0;
int ledPulseWidth = 280;

void setup() {
  Serial.begin(9600);  // Serial to communicate with ESP32
  pinMode(GP2Y1014_LED, OUTPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  dht.begin();
  Serial.println("Starting Arduino...");
}

float readDustSensor() {
  digitalWrite(GP2Y1014_LED, LOW);
  delayMicroseconds(ledPulseWidth);
  int rawADC = analogRead(GP2Y1014_AOUT);
  digitalWrite(GP2Y1014_LED, HIGH);
  float voltage = (rawADC / 1024.0) * 5.0;
  float density = 0.17 * voltage - 0.1;
  return density < 0 ? 0 : density;
}

int readGasSensor(int pin) {
  int value = analogRead(pin);
  return value == 1023 ? -1 : value;
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  dustDensity = readDustSensor();
  int mq135Value = readGasSensor(MQ135_PIN);
  int mq2Value = readGasSensor(MQ2_PIN);
  int flameDetected = digitalRead(FLAME_SENSOR_PIN);

  if (!isnan(temperature) && !isnan(humidity)) {
    // Send data as a comma-separated string
    Serial.print(temperature); Serial.print(",");
    Serial.print(humidity); Serial.print(",");
    Serial.print(dustDensity); Serial.print(",");
    Serial.print(mq135Value); Serial.print(",");
    Serial.print(mq2Value); Serial.print(",");
    Serial.println(flameDetected);
  } else {
    Serial.println("ERROR: Invalid DHT22 readings!");
  }
  delay(5000);
}