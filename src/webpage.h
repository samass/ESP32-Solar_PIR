#pragma once

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv='refresh' content='10'>
  <title>%PLATFORM% Solar PIR Light</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      padding: 50px;
      background-color: #f2f2f2;
    }
    h1 { color: #333; }
    p { font-size: 1.2em; color: #555; }
    .card {
      background: white;
      padding: 20px;
      border-radius: 10px;
      display: inline-block;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
    }
    .status-on  { color: #28a745; font-weight: bold; }
    .status-off { color: #dc3545; font-weight: bold; }
  </style>
</head>
<body>
  <div class="card">
    <h1>%PLATFORM% Solar PIR Light</h1>
    <p>🔆 Solar Voltage: %VOLTAGE%V (%DAY_NIGHT%)</p>
    <p>💡 Light Status: <span class="%LIGHT_CLASS%">%LIGHT_STATUS%</span></p>
    <p>👁 Motion Count: %MOTION_COUNT%</p>
    <p>🌡 DHT22 &mdash; Temp: %DHT_TEMP% &nbsp;|&nbsp; Humidity: %DHT_HUM%</p>
    <p>🌡 AHT20 &mdash; Temp: %AHT_TEMP% &nbsp;|&nbsp; Humidity: %AHT_HUM%</p>
    <p>🔵 BMP280 &mdash; Pressure: %BMP_PRESS% &nbsp;|&nbsp; Temp: %BMP_TEMP%</p>
    <p><em>Auto-refreshes every 10 seconds</em></p>
  </div>
</body>
</html>
)rawliteral";
