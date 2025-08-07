#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include "DHT.h"
#include "webpage.h"

#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
WiFiMulti wifiMulti;

unsigned long lastRead = 0;
float temperature = 0.0;
float humidity = 0.0;

// Add your WiFi credentials here
const char* ssid_wokwi = "Wokwi-GUEST";
const char* pass_wokwi = "";
const char* ssid_home = "YourHomeWiFi";
const char* pass_home = "YourHomePassword";
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
    <title>ESP32 DHT22 Server</title>
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
      <h1>ESP32 DHT22 Sensor Readings</h1>
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

  // Add all networks to WiFiMulti
  wifiMulti.addAP(ssid_wokwi, pass_wokwi);
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