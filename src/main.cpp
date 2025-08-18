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

// Add your WiFi credentials here
//const char* ssid_wokwi = "Wokwi-GUEST";
//const char* pass_wokwi = "";
const char* ssid_home = "SKYAEPBW";
const char* pass_home = "e3CVynV31eeU";
const char* ssid_office = "YourOfficeWiFi";
const char* pass_office = "YourOfficePassword";

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

  html += R"rawliteral( DHT22 Server</title>
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

  html += R"rawliteral( DHT22 Sensor Readings</h1>
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
  dht.begin();

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