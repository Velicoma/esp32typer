#include <BleKeyboard.h>
#include <WiFi.h>
#include <WebServer.h>

// ---- EDIT THESE TWO LINES ----
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// ------------------------------

BleKeyboard bleKeyboard("ESP32 Typer", "DIY", 100);
WebServer server(80);

String pendingText = "";
bool shouldType = false;

const char* htmlPage = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Typer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; max-width: 650px; margin: 40px auto; padding: 0 20px; }
    h2 { color: #333; }
    textarea { width: 100%; height: 280px; font-size: 14px; padding: 8px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }
    .controls { display: flex; gap: 16px; margin-top: 10px; align-items: flex-end; flex-wrap: wrap; }
    .control-group { display: flex; flex-direction: column; gap: 4px; }
    label { font-size: 13px; color: #555; }
    input[type=range] { width: 150px; }
    input[type=number] { width: 65px; padding: 6px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px; }
    button { padding: 10px 28px; font-size: 16px; background: #0078d4; color: white; border: none; border-radius: 4px; cursor: pointer; }
    button:disabled { background: #999; cursor: default; }
    #status { margin-top: 14px; font-weight: bold; min-height: 24px; color: #333; }
    #cdDisplay { font-size: 72px; font-weight: bold; color: #0078d4; text-align: center; margin: 10px 0; display: none; }
    .bt-status { font-size: 13px; color: #888; margin-top: 6px; }
  </style>
</head>
<body>
  <h2>ESP32 Typer</h2>
  <textarea id="txt" placeholder="Paste your text here..."></textarea>
  <div class="controls">
    <div class="control-group">
      <label>Speed (delay: <span id="delayVal">10</span>ms per char)</label>
      <input type="range" id="delaySlider" min="5" max="100" value="10"
        oninput="document.getElementById('delayVal').innerText=this.value">
    </div>
    <div class="control-group">
      <label>Countdown (sec)</label>
      <input type="number" id="cdInput" min="0" max="30" value="3">
    </div>
    <button id="btn" onclick="startTyping()">Type it out</button>
  </div>
  <div id="cdDisplay"></div>
  <div id="status"></div>
  <p class="bt-status">Make sure the target PC has this device paired as a Bluetooth keyboard and a text field is focused before typing starts.</p>

  <script>
    function startTyping() {
      const text = document.getElementById('txt').value;
      const delay = document.getElementById('delaySlider').value;
      const countdown = parseInt(document.getElementById('cdInput').value) || 0;
      const btn = document.getElementById('btn');
      const status = document.getElementById('status');
      const cdDisplay = document.getElementById('cdDisplay');

      if (!text.trim()) { status.innerText = 'Please paste some text first.'; return; }

      btn.disabled = true;
      status.innerText = '';
      let remaining = countdown;

      function tick() {
        if (remaining > 0) {
          cdDisplay.style.display = 'block';
          cdDisplay.innerText = remaining;
          remaining--;
          setTimeout(tick, 1000);
        } else {
          cdDisplay.style.display = 'none';
          status.innerText = 'Typing ' + text.length + ' characters...';
          fetch('/type', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'text=' + encodeURIComponent(text) + '&delay=' + delay
          }).then(r => r.text()).then(msg => {
            status.innerText = msg;
            btn.disabled = false;
          }).catch(() => {
            status.innerText = 'Connection error.';
            btn.disabled = false;
          });
        }
      }
      tick();
    }
  </script>
</body>
</html>
)rawhtml";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleType() {
  if (server.hasArg("text")) {
    pendingText = server.arg("text");
    int delay_ms = 10;
    if (server.hasArg("delay")) {
      delay_ms = server.arg("delay").toInt();
    }
    // Store delay in a global for the loop
    shouldType = true;
    server.send(200, "text/plain",
      "Done! Typed " + String(pendingText.length()) + " characters.");
  } else {
    server.send(400, "text/plain", "No text received.");
  }
}

int typingDelay = 10;

void setup() {
  Serial.begin(115200);

  // Start BLE keyboard
  bleKeyboard.begin();
  Serial.println("BLE Keyboard started - pair with target PC now (name: 'ESP32 Typer')");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Web interface ready at: http://");
  Serial.println(WiFi.localIP());

  // Start web server
  server.on("/", handleRoot);
  server.on("/type", HTTP_POST, handleType);
  server.begin();
}

void loop() {
  server.handleClient();

  if (shouldType) {
    if (bleKeyboard.isConnected()) {
      shouldType = false;

      // Read delay from last request
      int delay_ms = typingDelay;

      delay(300);
      for (int i = 0; i < pendingText.length(); i++) {
        bleKeyboard.print(pendingText[i]);
        delay(delay_ms);
      }
    }
    // If BLE not connected yet, keep waiting
  }
}
