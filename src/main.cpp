// ESP32Typer - USB HID version for ESP32-S2 Mini V1
//
// PLATFORMIO SETTINGS (platformio.ini):
//   board:                 lolin_s2_mini
//   framework:             arduino
//   board_build.partitions = no_ota.csv
//   build_flags:           -DARDUINO_USB_MODE=0
//                          -DARDUINO_USB_CDC_ON_BOOT=1
//
// CONNECTIONS:
//   FLASH/DEBUG -> connect the UART port (the one near the reset button) to your PC
//   TARGET PC   -> connect the USB port (the one labelled USB) to the target PC
//
// No extra libraries needed - USB HID is built into the ESP32-S2 Arduino core.

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"
#include <ESPmDNS.h>

USBHIDKeyboard Keyboard;
WebServer server(80);

String pendingText  = "";
bool   shouldType   = false;
int    typingDelay  = 10;

// Session token — set on successful login, cleared on logout
String sessionToken = "";

String generateToken() {
  String token = "";
  for (int i = 0; i < 32; i++) {
    token += String(random(0, 16), HEX);
  }
  return token;
}

bool isLoggedIn() {
  if (sessionToken == "") return false;
  if (!server.hasHeader("Cookie")) return false;
  String cookie = server.header("Cookie");
  return cookie.indexOf("session=" + sessionToken) >= 0;
}

const char* loginPage = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Typer — Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; max-width: 360px; margin: 100px auto; padding: 0 20px; }
    h2 { color: #333; margin-bottom: 24px; }
    input[type=text], input[type=password] {
      width: 100%; padding: 10px; font-size: 15px; margin-bottom: 14px;
      box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px;
    }
    button {
      width: 100%; padding: 11px; font-size: 16px;
      background: #0078d4; color: white; border: none;
      border-radius: 4px; cursor: pointer;
    }
    .error { color: #c00; font-size: 14px; margin-bottom: 12px; }
  </style>
</head>
<body>
  <h2>ESP32 Typer</h2>
  %ERROR%
  <input type="text"     id="u" placeholder="Username" />
  <input type="password" id="p" placeholder="Password" />
  <button onclick="doLogin()">Login</button>
  <script>
    function doLogin() {
      fetch('/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'u=' + encodeURIComponent(document.getElementById('u').value) +
              '&p=' + encodeURIComponent(document.getElementById('p').value)
      }).then(r => {
        if (r.ok) window.location.href = '/';
        else document.querySelector('.error') 
          ? document.querySelector('.error').innerText = 'Invalid username or password.'
          : location.reload();
      });
    }
    // Allow pressing Enter to submit
    document.addEventListener('keydown', e => { if (e.key === 'Enter') doLogin(); });
  </script>
</body>
</html>
)rawhtml";

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
    #logoutBtn { background: #888; margin-left: auto; }
    #status { margin-top: 14px; font-weight: bold; min-height: 24px; color: #333; }
    #cdDisplay { font-size: 72px; font-weight: bold; color: #0078d4; text-align: center; margin: 10px 0; display: none; }
    .usb-status { font-size: 13px; color: #888; margin-top: 6px; }
    .header { display: flex; align-items: center; margin-bottom: 10px; }
    .header h2 { margin: 0; flex: 1; }
  </style>
</head>
<body>
  <div class="header">
    <h2>ESP32 Typer</h2>
    <button id="logoutBtn" onclick="logout()">Logout</button>
  </div>
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
  <p class="usb-status">Make sure the target PC has the ESP32 connected via USB and a text field is focused before typing starts.</p>

  <script>
    function startTyping() {
      const text      = document.getElementById('txt').value;
      const delay     = document.getElementById('delaySlider').value;
      const countdown = parseInt(document.getElementById('cdInput').value) || 0;
      const btn       = document.getElementById('btn');
      const status    = document.getElementById('status');
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
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'text=' + encodeURIComponent(text) + '&delay=' + delay
          }).then(r => {
            if (r.status === 401) { window.location.href = '/login'; return; }
            return r.text();
          }).then(msg => {
            if (msg) status.innerText = msg;
            btn.disabled = false;
          }).catch(() => {
            status.innerText = 'Connection error.';
            btn.disabled = false;
          });
        }
      }
      tick();
    }

    function logout() {
      fetch('/logout', { method: 'POST' }).then(() => {
        window.location.href = '/login';
      });
    }
  </script>
</body>
</html>
)rawhtml";

void redirectToLogin() {
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "");
}

void handleLoginPage() {
  String page = String(loginPage);
  page.replace("%ERROR%", "");
  server.send(200, "text/html", page);
}

void handleLoginPost() {
  String u = server.hasArg("u") ? server.arg("u") : "";
  String p = server.hasArg("p") ? server.arg("p") : "";

  if (u == String(webUsername) && p == String(webPassword)) {
    sessionToken = generateToken();
    server.sendHeader("Set-Cookie", "session=" + sessionToken + "; Path=/");
    server.send(200, "text/plain", "OK");
  } else {
    server.send(401, "text/plain", "Unauthorized");
  }
}

void handleLogout() {
  sessionToken = "";
  server.sendHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  if (!isLoggedIn()) { redirectToLogin(); return; }
  server.send(200, "text/html", htmlPage);
}

void handleType() {
  if (!isLoggedIn()) { server.send(401, "text/plain", "Unauthorized"); return; }
  if (server.hasArg("text")) {
    pendingText = server.arg("text");
    typingDelay = 10;
    if (server.hasArg("delay")) {
      typingDelay = server.arg("delay").toInt();
    }
    shouldType = true;
    server.send(200, "text/plain",
      "Done! Typed " + String(pendingText.length()) + " characters.");
  } else {
    server.send(400, "text/plain", "No text received.");
  }
}

void setup() {
  // USB HID keyboard — must be started before USB.begin()
  Keyboard.begin();
  USB.begin();

  // Serial over USB CDC (appears on the same native USB port)
  // Give the USB stack a moment to enumerate before printing
  delay(1500);
  Serial.begin(115200);
  Serial.println("USB HID Keyboard started.");

  randomSeed(esp_random());

  // Connect to WiFi
  bool connected = false;
  for (int i = 0; i < networkCount && !connected; i++) {
    Serial.printf("Trying %s...\n", networks[i].ssid);
    WiFi.begin(networks[i].ssid, networks[i].password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.printf("\nConnected to %s\n", networks[i].ssid);
    } else {
      Serial.println("\nFailed, trying next...");
      WiFi.disconnect();
      delay(500);
    }
  }

  if (!connected) {
    Serial.println("Could not connect to any network. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.print("Web interface ready at: http://");
  Serial.println(WiFi.localIP());

  // Start mDNS so device is reachable at http://esp32typer.local
  if (MDNS.begin("esp32typer")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS started - connect at http://esp32typer.local");
  } else {
    Serial.println("mDNS failed to start.");
  }

  // Required to read Cookie headers for session auth
  const char* headerKeys[] = { "Cookie" };
  server.collectHeaders(headerKeys, 1);

  // Start web server
  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/login",  HTTP_GET,  handleLoginPage);
  server.on("/login",  HTTP_POST, handleLoginPost);
  server.on("/logout", HTTP_POST, handleLogout);
  server.on("/type",   HTTP_POST, handleType);
  server.begin();
}

void loop() {
  server.handleClient();

  if (shouldType) {
    shouldType = false;
    delay(300); // small pause before starting to type
    for (int i = 0; i < (int)pendingText.length(); i++) {
      Keyboard.print(pendingText[i]);
      delay(typingDelay);
    }
  }
}