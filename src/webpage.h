const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Solar PIR Light Controller</title>
  <style>
    body { font-family: Arial; background: #f2f2f2; text-align: center; padding:40px;}
    .card { background: #fff; padding: 22px; border-radius: 10px; display: inline-block; box-shadow:0 0 10px #ccc;}
    .status { font-size:1.2em; margin:10px 0;}
    button { font-size:1em; padding:10px 20px; border-radius:6px; border:none; background: #007bff; color: #fff;}
  </style>
</head>
<body>
  <div class="card">
    <h1>Solar PIR Light Controller</h1>
    <p class="status">🔆 Solar Voltage: <b>%VOLTAGE%</b> V</p>
    <p class="status">🔋 Battery Level: <b>%BATTERY%</b> %</p>
    <p class="status">💡 Light Status: <b>%LIGHT_STATUS%</b></p>
    <p class="status">🌡 Temp: <b>%TEMPERATURE%</b> °C</p>
    <p class="status">💧 Humidity: <b>%HUMIDITY%</b> %</p>
    <form action="/override" method="POST">
      <button type="submit">%OVERRIDE_BUTTON%</button>
    </form>
    <p><em>Page refreshes every 10 seconds</em></p>
  </div>
</body>
</html>
)rawliteral";