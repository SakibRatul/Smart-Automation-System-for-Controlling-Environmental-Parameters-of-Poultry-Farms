#include "DHT.h"
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <ThingSpeak.h>

// WiFi credentials
const char* ssid = "Bap";
const char* password = "1234512345";

// ThingSpeak channel details
unsigned long myChannelNumber = 2679819;
const char * myWriteAPIKey = "PUSDW8QJGZAL6ZXT";

// Define DHT11 pin connections and sensor types
#define DHT1_PIN 15  // First DHT11 sensor connected to GPIO 15
#define DHT2_PIN 4   // Second DHT11 sensor connected to GPIO 4
#define DHTTYPE DHT11 // Defining the sensor type as DHT11

// Define MQ sensors' analog pins
#define MQ5_PIN 34  // MQ-5 sensor connected to Analog pin GPIO 34
#define MQ135_PIN 35 // MQ-135 sensor connected to Analog pin GPIO 35

// Define the microSD card CS pin
#define SD_CS_PIN 5  // Chip select for the SD card

// Relay pins for bulbs and fan
#define BULB1_PIN 26  // Relay pin for Bulb 1
#define BULB2_PIN 27  // Relay pin for Bulb 2
#define FAN_PIN 14    // Relay pin for Exhaust Fan

// Threshold values for temperature and humidity
const float tempThresholdLow = 25.0;  // Lower threshold for temperature (in degrees Celsius)
const float tempThresholdHigh = 35.0; // Higher threshold for temperature (in degrees Celsius)
const float humThresholdLow = 75.0;   // Lower threshold for humidity (in percentage)
const float humThresholdHigh = 80.0;  // Higher threshold for humidity (in percentage)

// Create DHT objects
DHT dht1(DHT1_PIN, DHTTYPE);
DHT dht2(DHT2_PIN, DHTTYPE);

// Create WiFiClient object
WiFiClient client;

// File object for writing to SD card
File dataFile;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Start the DHT sensors
  dht1.begin();
  dht2.begin();

  // Setup analog pins for MQ sensors (MQ5 & MQ135)
  pinMode(MQ5_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);

  // Initialize relay pins for bulbs and fan
  pinMode(BULB1_PIN, OUTPUT);
  pinMode(BULB2_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  // Set initial states of relays to off
  digitalWrite(BULB1_PIN, LOW);
  digitalWrite(BULB2_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);

  // Initialize the microSD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card failed, or not present.");
    while (1); // Halt the execution if SD card initialization fails
  }
  Serial.println("SD card initialized successfully.");

  // Connect to Wi-Fi with retry mechanism
  WiFi.begin(ssid, password);
  int wifiRetryCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < 30) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    wifiRetryCount++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("Failed to connect to WiFi, check credentials or router.");
    while (1); // Halt if Wi-Fi connection fails
  }

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Read temperature and humidity from the first DHT11 sensor
  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();

  // Read temperature and humidity from the second DHT11 sensor
  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  // Read analog values from MQ-5 and MQ-135 sensors
  int mq5Value = analogRead(MQ5_PIN);
  int mq135Value = analogRead(MQ135_PIN);

  // Check if any reading from DHT11 is failed and handle errors
  if (isnan(temp1) || isnan(hum1) || isnan(temp2) || isnan(hum2)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Print sensor values to serial monitor
    Serial.println("DHT11 Sensor 1:");
    Serial.print("Temperature: ");
    Serial.print(temp1);
    Serial.println(" *C");
    Serial.print("Humidity: ");
    Serial.print(hum1);
    Serial.println(" %");

    Serial.println("DHT11 Sensor 2:");
    Serial.print("Temperature: ");
    Serial.print(temp2);
    Serial.println(" *C");
    Serial.print("Humidity: ");
    Serial.print(hum2);
    Serial.println(" %");

    // Print MQ sensor values to serial monitor
    Serial.print("MQ-5 Gas Sensor Value: ");
    Serial.println(mq5Value);
    Serial.print("MQ-135 Gas Sensor Value: ");
    Serial.println(mq135Value);

    // Open file for writing to SD card
    dataFile = SD.open("/sensor_data.txt", FILE_WRITE);

    // If the file is available, write the sensor data
    if (dataFile) {
      // Log DHT11 sensor values
      dataFile.print("DHT11 Sensor 1: ");
      dataFile.print("Temp: ");
      dataFile.print(temp1);
      dataFile.print(" *C, ");
      dataFile.print("Humidity: ");
      dataFile.print(hum1);
      dataFile.println(" %");

      dataFile.print("DHT11 Sensor 2: ");
      dataFile.print("Temp: ");
      dataFile.print(temp2);
      dataFile.print(" *C, ");
      dataFile.print("Humidity: ");
      dataFile.print(hum2);
      dataFile.println(" %");

      // Log MQ sensor values
      dataFile.print("MQ-5 Gas Sensor: ");
      dataFile.println(mq5Value);
      dataFile.print("MQ-135 Gas Sensor: ");
      dataFile.println(mq135Value);

      // Close the file after writing
      dataFile.close();
    } else {
      // If the file couldn't be opened, print an error
      Serial.println("Error opening sensor_data.txt");
    }

    // Send the sensor data to ThingSpeak
    ThingSpeak.setField(1, temp1); // Field 1 for temperature from DHT11 Sensor 1
    ThingSpeak.setField(2, hum1);  // Field 2 for humidity from DHT11 Sensor 1
    ThingSpeak.setField(3, temp2); // Field 3 for temperature from DHT11 Sensor 2
    ThingSpeak.setField(4, hum2);  // Field 4 for humidity from DHT11 Sensor 2
    ThingSpeak.setField(5, mq5Value); // Field 5 for MQ-5 sensor
    ThingSpeak.setField(6, mq135Value); // Field 6 for MQ-135 sensor

    // Update ThingSpeak channel
    int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (responseCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully.");
    } else {
      Serial.println("Failed to send data to ThingSpeak. Response code: " + String(responseCode));
    }

    // Relay control logic for bulbs and exhaust fan

    // Check if both sensors are below the lower threshold for temp and humidity
    if (temp1 < tempThresholdLow && temp2 < tempThresholdLow && hum1 < humThresholdLow && hum2 < humThresholdLow) {
      digitalWrite(BULB1_PIN, HIGH);  // Turn on Bulb 1
      digitalWrite(BULB2_PIN, HIGH);  // Turn on Bulb 2
      Serial.println("Bulbs turned on.");
    } else {
      digitalWrite(BULB1_PIN, LOW);   // Turn off Bulb 1
      digitalWrite(BULB2_PIN, LOW);   // Turn off Bulb 2
      Serial.println("Bulbs turned off.");
    }

    // Check if both sensors exceed the higher threshold for temp or humidity
    if ((temp1 > tempThresholdHigh && temp2 > tempThresholdHigh) || (hum1 > humThresholdHigh && hum2 > humThresholdHigh)) {
      digitalWrite(FAN_PIN, HIGH);  // Turn on the exhaust fan
      Serial.println("Exhaust fan turned on.");
    } else {
      digitalWrite(FAN_PIN, LOW);   // Turn off the exhaust fan
      Serial.println("Exhaust fan turned off.");
    }
  }

  // Add delay for readability and cloud upload
  delay(20000); // Delay of 20 seconds before next reading
}
