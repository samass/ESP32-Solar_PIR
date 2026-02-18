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
#include "secrets.h"
#include "webpage.h"

#ifdef ESP32
  #include <Wire.h>
  #include <Adafruit_AHTX0.h>
  #include <Adafruit_BMP280.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT  32
  #define OLED_RESET     -1   // no reset pin
  #define OLED_ADDR    0x3C   // try 0x3D if display not found
  #define DISPLAY_BTN_PIN 13  // pushbutton: GPIO13 → GND (INPUT_PULLUP)
#endif

#ifdef ESP32
  #define PLATFORM_NAME "ESP32"
#endif
#ifdef ESP8266
  #define PLATFORM_NAME "ESP8266"
#endif

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
#define VOLTAGE_DIVIDER_RATIO 4.0  // R1=30kΩ (3x 10kΩ), R2=10kΩ → ratio=(30+10)/10
#define NIGHT_VOLTAGE_THRESHOLD 1.5  // Below this voltage = night time
#define LOW_SOLAR_THRESHOLD 3.2     // Below this voltage = low solar output
#define CRITICAL_SOLAR_THRESHOLD 1.8 // Below this voltage = near darkness

DHT dht(DHTPIN, DHTTYPE);

#ifdef ESP32
  Adafruit_AHTX0 aht;
  Adafruit_BMP280 bmp;
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

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
float temperature = NAN;
float humidity = NAN;
float voltage = 0.0;
bool dhtAvailable = false;

#ifdef ESP32
  bool aht20Available  = false;
  bool bmp280Available = false;
  bool oledAvailable   = false;
  bool displayOn       = true;
  float ahtTemp        = NAN;
  float ahtHumidity    = NAN;
  float bmpTemp        = NAN;
  float bmpPressure    = NAN;
#endif

// PIR motion detection variables
volatile bool motionDetected = false;
unsigned long lastMotionTime = 0;
unsigned long motionCount = 0;
bool isNightTime = false;
bool lightActive = false;
bool lowSolar = false;
bool criticalSolar = false;
const unsigned long LIGHT_DURATION = 30000; // Light stays on for 30 seconds

// Add your WiFi credentials to src/secrets.h

// PIR interrupt handler
IRAM_ATTR void handleMotion() {
  motionDetected = true;
}

// Read solar panel voltage
void readVoltage() {
  int adcValue = analogRead(VOLTAGE_PIN);
  voltage = (adcValue * REF_VOLTAGE / ADC_MAX) * VOLTAGE_DIVIDER_RATIO;
  isNightTime   = (voltage < NIGHT_VOLTAGE_THRESHOLD);
  lowSolar      = (voltage < LOW_SOLAR_THRESHOLD);
  criticalSolar = (voltage < CRITICAL_SOLAR_THRESHOLD);
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
  if (!dhtAvailable) return;
  unsigned long now = millis();
  if (now - lastRead > 2000) { // Read every 2 seconds
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    lastRead = now;
  }
}

#ifdef ESP32
void readI2CSensors() {
  static unsigned long lastI2CRead = 0;
  if (millis() - lastI2CRead < 2000) return;
  lastI2CRead = millis();

  if (aht20Available) {
    sensors_event_t humidity_ev, temp_ev;
    aht.getEvent(&humidity_ev, &temp_ev);
    ahtTemp     = temp_ev.temperature;
    ahtHumidity = humidity_ev.relative_humidity;
  }
  if (bmp280Available) {
    bmpTemp     = bmp.readTemperature();
    bmpPressure = bmp.readPressure() / 100.0F; // hPa
  }
}
#endif

// ── OLED display functions (ESP32 only) ──────────────────────────────────────
#ifdef ESP32

// 4 ascending signal bars, bottom-aligned in the 8px status bar
void drawSignalBars(int rssi, bool connected) {
  const int heights[] = {2, 4, 6, 7};
  int numBars = 0;
  if (connected) {
    if      (rssi >= -50) numBars = 4;
    else if (rssi >= -60) numBars = 3;
    else if (rssi >= -70) numBars = 2;
    else                  numBars = 1;
  }
  for (int i = 0; i < 4; i++) {
    int h = heights[i];
    int x = i * 3;        // 2px bar + 1px gap
    int y = 8 - h;        // bottom-align to y=7
    if (i < numBars) {
      display.fillRect(x, y, 2, h, SSD1306_WHITE);
    } else {
      display.drawRect(x, y, 2, h, SSD1306_WHITE);
    }
  }
}

// Toggle display on/off with debounced button press
void handleDisplayButton() {
  static bool lastState = HIGH;
  static unsigned long lastDebounce = 0;
  bool state = digitalRead(DISPLAY_BTN_PIN);
  if (state == LOW && lastState == HIGH && millis() - lastDebounce > 200) {
    displayOn = !displayOn;
    display.ssd1306_command(displayOn ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
    lastDebounce = millis();
  }
  lastState = state;
}

void updateDisplay() {
  if (!oledAvailable || !displayOn) return;

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate < 2000) return;
  lastDisplayUpdate = millis();

  display.clearDisplay();

  // ── Status bar ────────────────────────────────────────────────────────────
  drawSignalBars(WiFi.RSSI(), WiFi.status() == WL_CONNECTED);

  // Solar voltage — right-aligned in status bar
  String voltStr = String(voltage, 1) + "V";
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(SCREEN_WIDTH - voltStr.length() * 6, 0);
  display.print(voltStr);
  display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE); // divider

  // ── Content (y=11 to 31) ─────────────────────────────────────────────────
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  bool showDHT = dhtAvailable    && !isnan(temperature);
  bool showAHT = aht20Available  && !isnan(ahtTemp);
  bool showBMP = bmp280Available && !isnan(bmpPressure);

  if (showDHT && showAHT) {
    // Dual sensor — compact comparison, two rows
    display.setCursor(0, 12);
    display.print("DHT:");
    display.print(temperature, 1);
    display.print("C ");
    display.print((int)roundf(humidity));
    display.print("%");

    display.setCursor(0, 22);
    display.print("AHT:");
    display.print(ahtTemp, 1);
    display.print("C ");
    display.print((int)roundf(ahtHumidity));
    display.print("%");

  } else {
    // Single sensor — row 1: label + temp/humidity
    int y = 12;
    if (showDHT) {
      display.setCursor(0, y);
      display.print("DHT22 ");
      display.print(temperature, 1);
      display.print("C ");
      display.print((int)roundf(humidity));
      display.print("%");
      y += 10;
    } else if (showAHT) {
      display.setCursor(0, y);
      display.print("AHT20 ");
      display.print(ahtTemp, 1);
      display.print("C ");
      display.print((int)roundf(ahtHumidity));
      display.print("%");
      y += 10;
    } else {
      display.setCursor(0, y);
      display.print("No temp sensor");
      y += 10;
    }

    // Row 2: BMP280 pressure if available
    if (showBMP) {
      display.setCursor(0, y);
      display.print((int)roundf(bmpPressure));
      display.print("hPa ");
      display.print(bmpTemp, 1);
      display.print("C");
    }
  }

  display.display();
}

#endif
// ─────────────────────────────────────────────────────────────────────────────

void handleRoot() {
  String html = MAIN_page;

  html.replace("%PLATFORM%",     PLATFORM_NAME);
  html.replace("%VOLTAGE%",      String(voltage, 2));
  html.replace("%DAY_NIGHT%",    isNightTime ? "NIGHT" : "DAY");
  html.replace("%LIGHT_CLASS%",  lightActive ? "status-on" : "status-off");
  html.replace("%LIGHT_STATUS%", lightActive ? "ON" : "OFF");
  html.replace("%MOTION_COUNT%", String(motionCount));
  html.replace("%DHT_TEMP%",     isnan(temperature) ? "N/A" : String(temperature, 1) + " &deg;C");
  html.replace("%DHT_HUM%",      isnan(humidity)    ? "N/A" : String(humidity, 1)    + " %");

  #ifdef ESP32
    html.replace("%AHT_TEMP%",   aht20Available  ? String(ahtTemp, 1)     + " &deg;C" : "N/A");
    html.replace("%AHT_HUM%",    aht20Available  ? String(ahtHumidity, 1) + " %"      : "N/A");
    html.replace("%BMP_PRESS%",  bmp280Available ? String(bmpPressure, 1) + " hPa"    : "N/A");
    html.replace("%BMP_TEMP%",   bmp280Available ? String(bmpTemp, 1)     + " &deg;C" : "N/A");
  #else
    html.replace("%AHT_TEMP%",  "N/A");
    html.replace("%AHT_HUM%",   "N/A");
    html.replace("%BMP_PRESS%", "N/A");
    html.replace("%BMP_TEMP%",  "N/A");
  #endif

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

  // DHT22 detection: attempt 3 reads after warmup; any valid read = connected
  Serial.println("Detecting DHT22...");
  delay(2500);
  for (int i = 0; i < 3; i++) {
    float t = dht.readTemperature();
    if (!isnan(t)) { dhtAvailable = true; break; }
    delay(2100);
  }
  Serial.println(dhtAvailable ? "DHT22 detected" : "DHT22 not found");

  // I2C sensor detection (ESP32 only)
  #ifdef ESP32
    Wire.begin(); // SDA=GPIO21, SCL=GPIO22 (ESP32 defaults)
    aht20Available = aht.begin();
    Serial.println(aht20Available ? "AHT20 detected" : "AHT20 not found");
    // Try 0x76 first (common on combo boards), then 0x77 (Adafruit default)
    bmp280Available = bmp.begin(0x76) || bmp.begin(0x77);
    Serial.println(bmp280Available ? "BMP280 detected" : "BMP280 not found");

    // OLED detection (try 0x3C, fallback 0x3D)
    oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
    if (!oledAvailable) {
      oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    }
    if (oledAvailable) {
      Serial.println("OLED detected");
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(20, 12);
      display.print("Solar PIR Light");
      display.display();
    } else {
      Serial.println("OLED not found");
    }
  #endif
  
  // Initialize PIR sensor
  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), handleMotion, RISING);
  
  // Initialize LED control pin
  pinMode(LED_CONTROL_PIN, OUTPUT);
  digitalWrite(LED_CONTROL_PIN, LOW); // Start with LEDs off

  // Initialize voltage monitoring
  pinMode(VOLTAGE_PIN, INPUT);

  // Initialize display toggle button (ESP32 only)
  #ifdef ESP32
    pinMode(DISPLAY_BTN_PIN, INPUT_PULLUP);
  #endif
  
  Serial.println("PIR Motion Sensor initialized on GPIO " + String(PIR_PIN));
  Serial.println("LED Control initialized on GPIO " + String(LED_CONTROL_PIN));
  Serial.println("Voltage monitoring initialized on GPIO " + String(VOLTAGE_PIN));
  
  // Initial power status check
  readVoltage();
  Serial.print("Initial solar voltage: ");
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
  wifiMulti.addAP(WIFI_SSID_HOME, WIFI_PASS_HOME);
  wifiMulti.addAP(WIFI_SSID_OFFICE, WIFI_PASS_OFFICE);

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

  // Read DHT sensor periodically (readDHT() rate-limits internally)
  readDHT();

  // Read I2C sensors periodically (rate-limits internally)
  #ifdef ESP32
    handleDisplayButton();
    readI2CSensors();
    updateDisplay();
  #endif

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
    Serial.print("DHT22: ");
    if (!dhtAvailable) {
      Serial.println("not connected");
    } else {
      Serial.print(isnan(temperature) ? "N/A" : String(temperature, 1) + " C");
      Serial.print(" | ");
      Serial.println(isnan(humidity)    ? "N/A" : String(humidity, 1)    + " %");
    }
#ifdef ESP32
    Serial.print("AHT20: ");
    if (!aht20Available) {
      Serial.println("not connected");
    } else {
      Serial.print(isnan(ahtTemp)     ? "N/A" : String(ahtTemp, 1)     + " C");
      Serial.print(" | ");
      Serial.println(isnan(ahtHumidity) ? "N/A" : String(ahtHumidity, 1) + " %");
    }
    Serial.print("BMP280: ");
    if (!bmp280Available) {
      Serial.println("not connected");
    } else {
      Serial.print(isnan(bmpPressure) ? "N/A" : String(bmpPressure, 1) + " hPa");
      Serial.print(" | ");
      Serial.println(isnan(bmpTemp)   ? "N/A" : String(bmpTemp, 1)     + " C");
    }
#endif
    Serial.print("Uptime: ");
    Serial.print(now / 1000);
    Serial.println(" seconds");
    Serial.println("HTTP Server: Running on port 80");
    Serial.print("Free Heap Memory: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println("==============================================\n");
    lastDebug = now;
  }
  static unsigned long lastWifiAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiAttempt > 5000) {
    lastWifiAttempt = millis();
    Serial.println("WiFi lost. Attempting reconnect...");
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("WiFi reconnected.");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  server.handleClient();
}