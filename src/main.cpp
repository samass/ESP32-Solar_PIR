// Platform-specific includes
#ifdef ESP32
  #include <WiFi.h>
  #include <WiFiMulti.h>
  #include <WebServer.h>
#endif

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WiFiMulti.h>
  #include <ESP8266WebServer.h>
#endif

#include "DHT.h"
#include "webpage.h"

#define DHTPIN 15
#define DHTTYPE DHT22
#define PIR_PIN 2
#define LED_CONTROL_PIN 4  // This will connect to IC pin 3

// Voltage sensing configuration
#ifdef ESP32
  #define VOLTAGE_PIN 35  // ADC1 pin for voltage sensing (ESP32)
  #define ADC_MAX 4095    // 12-bit ADC
  #define REF_VOLTAGE 3.3 // Reference voltage
#endif

#ifdef ESP8266
  #define VOLTAGE_PIN A0  // Analog pin for voltage sensing (ESP8266)  
  #define ADC_MAX 1023    // 10-bit ADC
  #define REF_VOLTAGE 3.3 // Reference voltage
#endif

// Voltage divider ratio (adjust based on your hardware)
#define VOLTAGE_DIVIDER_RATIO 4.1  // Calibrated for solar panel voltage
#define NIGHT_VOLTAGE_THRESHOLD 1.5  // Below this voltage = night time
#define LOW_BATTERY_THRESHOLD 3.2   // Below this voltage = low battery warning
#define CRITICAL_BATTERY_THRESHOLD 3.0  // Below this voltage = shutdown

DHT dht(DHTPIN, DHTTYPE);

// Platform-specific object declarations
#ifdef ESP32
  WebServer server(80);
  WiFiMulti wifiMulti;
#endif

#ifdef ESP8266
  ESP8266WebServer server(80);
  ESP8266WiFiMulti wifiMulti;
#endif

unsigned long lastRead = 0;
float temperature = 0.0;
float humidity = 0.0;
float voltage = 0.0;

// PIR motion detection variables
volatile bool motionDetected = false;
unsigned long lastMotionTime = 0;
unsigned long motionCount = 0;
bool isNightTime = false;
bool lightActive = false;
bool lowBattery = false;
bool criticalBattery = false;
const unsigned long LIGHT_DURATION = 30000; // Light stays on for 30 seconds

// Add your WiFi credentials here
//const char* ssid_wokwi = "Wokwi-GUEST";
//const char* pass_wokwi = "";
const char* ssid_home = "SKYAEPBW";
const char* pass_home = "e3CVynV31eeU";
const char* ssid_office = "YourOfficeWiFi";
const char* pass_office = "YourOfficePassword";

// PIR interrupt handler
IRAM_ATTR void handleMotion() {
  motionDetected = true;
}

// Read voltage from solar panel/battery
void readVoltage() {
  int adcValue = analogRead(VOLTAGE_PIN);
  voltage = (adcValue * REF_VOLTAGE / ADC_MAX) * VOLTAGE_DIVIDER_RATIO;
  isNightTime = (voltage < NIGHT_VOLTAGE_THRESHOLD);
  lowBattery = (voltage < LOW_BATTERY_THRESHOLD);
  criticalBattery = (voltage < CRITICAL_BATTERY_THRESHOLD);
  
  // Critical battery protection
  if (criticalBattery) {
    Serial.println("⚠️ CRITICAL BATTERY - Entering deep sleep!");
    digitalWrite(LED_CONTROL_PIN, LOW); // Ensure LEDs are off
    // In a real implementation, you'd enter deep sleep here
    // ESP.deepSleep(0); // Sleep until reset
  }
}

// Handle PIR motion detection
void handlePIRMotion() {
  if (motionDetected && isNightTime) {
    motionDetected = false; // Reset flag
    lastMotionTime = millis();
    motionCount++;
    lightActive = true;
    
    // Turn on LEDs via the original driver IC
    digitalWrite(LED_CONTROL_PIN, HIGH);
    
    Serial.println("*** MOTION DETECTED AT NIGHT ***");
    Serial.println("LEDs turned ON via driver IC");
    Serial.print("Motion count: ");
    Serial.println(motionCount);
    Serial.print("Solar voltage: ");
    Serial.print(voltage);
    Serial.println("V (Night mode)");
  }
  
  // Turn off light after duration
  if (lightActive && (millis() - lastMotionTime > LIGHT_DURATION)) {
    lightActive = false;
    digitalWrite(LED_CONTROL_PIN, LOW);
    Serial.println("LEDs turned OFF (timeout)");
  }
}

void readDHT() {
  unsigned long now = millis();
  if (now - lastRead > 2000) { // Read every 2 seconds
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    lastRead = now;
  }
}

void handleRoot() {
  readDHT();
  readVoltage(); // Update voltage reading
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta http-equiv='refresh' content='10'>
    <title>)rawliteral";
    
#ifdef ESP32
  html += "ESP32";
#endif
#ifdef ESP8266
  html += "ESP8266";
#endif

  html += R"rawliteral( Solar PIR Light</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        padding: 50px;
        background-color: #f2f2f2;
      }
      h1 {
        color: #333;
      }
      p {
        font-size: 1.2em;
        color: #555;
      }
      .card {
        background: white;
        padding: 20px;
        border-radius: 10px;
        display: inline-block;
        box-shadow: 0 0 10px rgba(0,0,0,0.1);
      }
      .status-on { color: #28a745; font-weight: bold; }
      .status-off { color: #dc3545; font-weight: bold; }
    </style>
  </head>
  <body>
    <div class="card">
      <h1>)rawliteral";

#ifdef ESP32
  html += "ESP32";
#endif
#ifdef ESP8266
  html += "ESP8266";
#endif

  html += R"rawliteral( Solar PIR Light</h1>
      <p>🔆 Solar Voltage: )rawliteral";
  html += String(voltage, 2) + "V (" + (isNightTime ? "NIGHT" : "DAY") + ")";
  html += R"rawliteral(</p>
      <p>💡 Light Status: <span class=")rawliteral";
  html += lightActive ? "status-on\">ON" : "status-off\">OFF";
  html += R"rawliteral(</span></p>
      <p>👁 Motion Count: )rawliteral";
  html += String(motionCount);
  html += R"rawliteral(</p>
      <p>🌡 Temperature: )rawliteral";

  html += isnan(temperature) ? "N/A" : String(temperature) + " °C";
  html += R"rawliteral(</p>
      <p>💧 Humidity: )rawliteral";
  html += isnan(humidity) ? "N/A" : String(humidity) + " %";
  html += R"rawliteral(</p>
      <p><em>Auto-refreshes every 10 seconds</em></p>
    </div>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // Add power-on delay for stable startup
  delay(1000);
  Serial.println("Solar PIR Light Controller Starting...");
  Serial.print("Running on: ");
#ifdef ESP32
  Serial.println("ESP32");
#endif
#ifdef ESP8266
  Serial.println("ESP8266");
#endif
  
  dht.begin();
  
  // Initialize PIR sensor
  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handleMotion, RISING);
  
  // Initialize LED control pin
  pinMode(LED_CONTROL_PIN, OUTPUT);
  digitalWrite(LED_CONTROL_PIN, LOW); // Start with LEDs off
  
  // Initialize voltage monitoring
  pinMode(VOLTAGE_PIN, INPUT);
  
  Serial.println("PIR Motion Sensor initialized on GPIO " + String(PIR_PIN));
  Serial.println("LED Control initialized on GPIO " + String(LED_CONTROL_PIN));
  Serial.println("Voltage monitoring initialized on GPIO " + String(VOLTAGE_PIN));
  
  // Initial power status check
  readVoltage();
  Serial.print("Initial battery voltage: ");
  Serial.print(voltage, 2);
  Serial.println("V");
  Serial.print("System mode: ");
  Serial.println(isNightTime ? "NIGHT" : "DAY");

  // Set hostname (platform-specific method)
#ifdef ESP32
  WiFi.setHostname("SolarPIR-ESP32");
#endif
#ifdef ESP8266
  WiFi.hostname("SolarPIR-ESP8266");
#endif

  // Add all networks to WiFiMulti
  //wifiMulti.addAP(ssid_wokwi, pass_wokwi);
  wifiMulti.addAP(ssid_home, pass_home);
  wifiMulti.addAP(ssid_office, pass_office);

  Serial.println("Connecting to WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Handle PIR motion detection
  handlePIRMotion();
  
  // Update voltage reading periodically
  static unsigned long lastVoltageRead = 0;
  if (millis() - lastVoltageRead > 5000) { // Read voltage every 5 seconds
    readVoltage();
    lastVoltageRead = millis();
  }
  
  // Debug block: print device status every 10 seconds
  static unsigned long lastDebug = 0;
  unsigned long now = millis();
  if (now - lastDebug > 10000) { // 10 seconds
    Serial.println("\n================ DEVICE STATUS ================");
    
#ifdef ESP32
    Serial.println("Platform: ESP32");
#endif
#ifdef ESP8266
    Serial.println("Platform: ESP8266");
#endif

    Serial.print("WiFi SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Hostname: ");
#ifdef ESP32
    Serial.println(WiFi.getHostname());
#endif
#ifdef ESP8266
    Serial.println(WiFi.hostname());
#endif
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Solar Voltage: ");
    Serial.print(voltage, 2);
    Serial.print("V (");
    Serial.print(isNightTime ? "NIGHT" : "DAY");
    Serial.println(")");
    Serial.print("Light Status: ");
    Serial.println(lightActive ? "ON" : "OFF");
    Serial.print("Motion Count: ");
    Serial.println(motionCount);
    Serial.print("Temperature: ");
    Serial.print(isnan(temperature) ? "N/A" : String(temperature) + " °C");
    Serial.print(" | Humidity: ");
    Serial.println(isnan(humidity) ? "N/A" : String(humidity) + " %");
    Serial.print("Uptime: ");
    Serial.print(now / 1000);
    Serial.println(" seconds");
    Serial.println("HTTP Server: Running on port 80");
    Serial.print("Free Heap Memory: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println("==============================================\n");
    lastDebug = now;
    delay(500); // Short delay to allow user to read output
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Reconnecting...");
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  server.handleClient();
}