#include "DHT.h"
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <RTClib.h>  // Include the library for DS3231 RTC

// -----------------------------
// Configuration Constants
// -----------------------------

// WiFi credentials
const char* ssid = "Intellier-KB";
const char* password = "IntellierKb@4321";

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

// Relay pin definitions
#define RELAY1_PIN 16  // First bulb relay
#define RELAY2_PIN 17  // Second bulb relay
#define RELAY3_PIN 27  // Exhaust fan relay
#define RELAY4_PIN 14  // Unused relay, you can connect additional device

// Thresholds for DHT11 sensor
const float lowTempThreshold = 26.0;  // Temperature threshold for turning on bulbs
const float highTempThreshold = 32.0; // Temperature threshold for turning on exhaust fan

// Loop delay in milliseconds
const unsigned long LOOP_DELAY = 5000; // 5 seconds

// -----------------------------
// Global Objects
// -----------------------------

// Create DHT objects
DHT dht1(DHT1_PIN, DHTTYPE);
DHT dht2(DHT2_PIN, DHTTYPE);

// File object for writing to SD card
File dataFile;

// RTC object for DS3231
RTC_DS3231 rtc;

// WiFiClient object
WiFiClient client;

// -----------------------------
// Setup Function
// -----------------------------

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("System initializing...");

  // Start the DHT sensors
  dht1.begin();
  dht2.begin();

  // Setup analog pins for MQ sensors (MQ5 & MQ135)
  pinMode(MQ5_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);

  // Initialize the microSD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed! Halting.");
    while (1); // Halt the execution if SD card initialization fails
  }
  Serial.println("SD card initialized successfully.");

  // Initialize relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Set all relays to off at the beginning
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC. Halting.");
    while (1);
  }

  // Check if the RTC lost power and if so, set the time
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    // The following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);  // Pass the WiFiClient object instead of WiFi
}

// -----------------------------
// Loop Function
// -----------------------------

void loop() {
  // Read the current time from RTC
  DateTime now = rtc.now();

  // Read temperature and humidity from the first DHT11 sensor
  float temp1 = dht1.readTemperature();
  float hum1 = dht1.readHumidity();

  // Read temperature and humidity from the second DHT11 sensor
  float temp2 = dht2.readTemperature();
  float hum2 = dht2.readHumidity();

  // Read analog values from MQ-5 and MQ-135 sensors
  int mq5Value = analogRead(MQ5_PIN);
  int mq135Value = analogRead(MQ135_PIN);

  // Validate sensor readings
  bool validReadings = true;
  if (isnan(temp1) || isnan(hum1)) {
    Serial.println("Error: Failed to read from DHT11 Sensor 1!");
    validReadings = false;
  }
  if (isnan(temp2) || isnan(hum2)) {
    Serial.println("Error: Failed to read from DHT11 Sensor 2!");
    validReadings = false;
  }

  // Optionally, validate analog sensor readings
  if (mq5Value < 0 || mq5Value > 4095) { // ESP32 ADC is 12-bit
    Serial.println("Error: Invalid MQ-5 sensor reading!");
    validReadings = false;
  }
  if (mq135Value < 0 || mq135Value > 4095) {
    Serial.println("Error: Invalid MQ-135 sensor reading!");
    validReadings = false;
  }

  if (validReadings) {
    // Print sensor values to serial monitor
    Serial.println("\n--- Sensor Readings ---");
    Serial.print("Timestamp: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);

    Serial.println("DHT11 Sensor 1:");
    Serial.print("  Temperature: ");
    Serial.print(temp1);
    Serial.println(" *C");
    Serial.print("  Humidity: ");
    Serial.print(hum1);
    Serial.println(" %");

    Serial.println("DHT11 Sensor 2:");
    Serial.print("  Temperature: ");
    Serial.print(temp2);
    Serial.println(" *C");
    Serial.print("  Humidity: ");
    Serial.print(hum2);
    Serial.println(" %");

    Serial.print("MQ-5 Gas Sensor Value: ");
    Serial.println(mq5Value);
    Serial.print("MQ-135 Gas Sensor Value: ");
    Serial.println(mq135Value);

    // Open file for writing to SD card
    dataFile = SD.open("/sensor_data.txt", FILE_WRITE);

    // If the file is available, write the sensor data along with timestamp
    if (dataFile) {
      // Write the current time from RTC
      dataFile.print("Timestamp: ");
      dataFile.print(now.year(), DEC);
      dataFile.print('/');
      dataFile.print(now.month(), DEC);
      dataFile.print('/');
      dataFile.print(now.day(), DEC);
      dataFile.print(" ");
      dataFile.print(now.hour(), DEC);
      dataFile.print(':');
      dataFile.print(now.minute(), DEC);
      dataFile.print(':');
      dataFile.println(now.second(), DEC);

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
      Serial.println("Data written to SD card.");
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

    // Control relay based on temperature thresholds
    if (temp1 < lowTempThreshold && temp2 < lowTempThreshold) {
      // Turn on both bulbs
      digitalWrite(RELAY1_PIN, HIGH);
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println("Both bulbs turned ON");
    } else {
      // Turn off both bulbs
      digitalWrite(RELAY1_PIN, LOW);
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println("Both bulbs turned OFF");
    }

    if (temp1 > highTempThreshold || temp2 > highTempThreshold) {
      // Turn on exhaust fan
      digitalWrite(RELAY3_PIN, HIGH);
      Serial.println("Exhaust fan turned ON");
    } else {
      // Turn off exhaust fan
      digitalWrite(RELAY3_PIN, LOW);
      Serial.println("Exhaust fan turned OFF");
    }
  }

  // Delay before next loop iteration
  delay(LOOP_DELAY);  // Delay for 5 seconds
}
