# ESP32 Solar PIR Light Controller

A solar-powered PIR (Passive Infrared) sensor light controller for ESP32/ESP8266 microcontrollers with web interface for monitoring environmental data.

## Features

- **Multi-platform Support**: Compatible with both ESP32 and ESP8266 microcontrollers
- **Environmental Monitoring**: Temperature and humidity sensing using DHT22 sensor
- **Web Interface**: Real-time monitoring via built-in web server
- **WiFi Connectivity**: Multi-network support for seamless connection
- **Debug Output**: Comprehensive serial output for monitoring device status

## Current Implementation

This project currently implements a **DHT22 sensor web server** with the following features:
- Temperature and humidity monitoring
- Auto-refreshing web interface (every 10 seconds)
- WiFi connectivity with multiple network support
- Serial debug output every 10 seconds
- Cross-platform compatibility (ESP32/ESP8266)

**Note**: The project includes template code in `webpage.h` for future solar PIR light controller features (solar voltage monitoring, battery level, PIR light control), but these features are not currently implemented.

## Hardware Requirements

- ESP32 or ESP8266 development board
- DHT22 temperature/humidity sensor
- Breadboard and jumper wires for connections

**For Future Solar PIR Implementation:**
- Solar panel and battery system 
- PIR motion sensor
- LED light controller circuit

## Pin Configuration

- **DHT22 Sensor**: GPIO 15

## WiFi Configuration

Update the WiFi credentials in `src/main.cpp`:

```cpp
const char* ssid_home = "YourHomeWiFi";
const char* pass_home = "YourHomePassword";
const char* ssid_office = "YourOfficeWiFi";
const char* pass_office = "YourOfficePassword";
```

## Building and Uploading

This project uses PlatformIO. To build and upload:

```bash
# For ESP8266 (NodeMCU v2)
pio run -e nodemcuv2 --target upload

# For ESP32
pio run -e esp32dev --target upload
```

## Web Interface

Once connected to WiFi, the device will start a web server on port 80. Navigate to the device's IP address to view:

- Real-time temperature readings
- Humidity levels
- Auto-refreshing data every 10 seconds

## Project Structure

```
├── src/
│   ├── main.cpp      # Main application code
│   ├── webpage.h     # Web interface template
│   └── README.md     # Project overview
├── platformio.ini    # PlatformIO configuration
└── README.md         # This file
```

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.