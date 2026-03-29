# ESP32 Typer

A Bluetooth HID keyboard emulator for the ESP32. Paste text into a web interface on your PC, and the ESP32 types it out via Bluetooth on any paired device — no software required on the target machine.

---

## How It Works

```
Your Source PC                        Target PC
┌──────────────┐                     ┌──────────────┐
│              │   WiFi (same LAN)   │              │
│   Browser    │ ─────────────────▶  │              │
│  (paste text)│       ESP32         │  Sees a BT   │
│              │ ◀──────────────── │  keyboard    │
└──────────────┘    Bluetooth HID   └──────────────┘
```

- The ESP32 connects to your local WiFi and hosts a simple web page
- You open that page on your source PC and paste text into it
- The ESP32 types the text via Bluetooth HID into whatever is focused on the target PC
- The target PC sees it as a normal Bluetooth keyboard — no drivers or software needed

---

## Hardware Required

| Item | Notes |
|------|-------|
| ESP32 DevKit (ESP-32D) | The generic 38-pin DevKit with USB-C or Micro-USB |
| USB cable | USB-C or Micro-USB depending on your board |
| Power source | USB charger or PC USB port to power the ESP32 |

No additional components required.

---

## Software Setup

### Step 1 — Install Arduino IDE 2
Download from: https://www.arduino.cc/en/software

### Step 2 — Add ESP32 Board Support
1. Open **File → Preferences**
2. Add this URL to "Additional boards manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**
4. Search `esp32` and install **esp32 by Espressif Systems**

### Step 3 — Install CH340 Driver
Your board uses the CH340C USB-serial chip. Install the driver from:
https://www.wch-ic.com/downloads/CH341SER_EXE.html

Reboot after installing.

### Step 4 — Install the BLE Keyboard Library
The T-vK library does not always appear in the Library Manager, so install it manually:

1. Download the ZIP from:
   https://github.com/T-vK/ESP32-BLE-Keyboard/archive/refs/heads/master.zip
2. In Arduino IDE go to **Sketch → Include Library → Add .ZIP Library...**
3. Select the downloaded ZIP

### Step 5 — Fix Library Compatibility
The library has a type mismatch with the newer ESP32 Arduino core. Open this file in Notepad:
```
C:\Users\[YOUR USERNAME]\OneDrive\Documents\Arduino\libraries\ESP32_BLE_Keyboard\BleKeyboard.cpp
```

Find line 106 and change:
```cpp
BLEDevice::init(deviceName);
```
to:
```cpp
BLEDevice::init(String(deviceName.c_str()));
```

Find line 117 and change:
```cpp
hid->manufacturer()->setValue(deviceManufacturer);
```
to:
```cpp
hid->manufacturer()->setValue(String(deviceManufacturer.c_str()));
```

Save the file.

---

## Configuration

Open `ESP32Typer.ino` and edit these two lines near the top:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

Replace with your actual WiFi network name and password.

---

## Uploading

1. Open `ESP32Typer.ino` in Arduino IDE
2. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
3. Select the correct port under **Tools → Port** (e.g. COM3)
4. **Important — change the partition scheme:**
   Go to **Tools → Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)**
   (BLE + WiFi together requires more flash space)
5. Click **Upload** (the → arrow)
6. If you see `Connecting......` in the output, hold the **BOOT button** on the ESP32 for 2-3 seconds to enter download mode
7. Once uploaded, open **Tools → Serial Monitor** at **115200 baud**
8. You will see the IP address printed once it connects to WiFi

---

## First Use

### On the target PC (the one you want to type into):
1. Open Bluetooth settings
2. Add a new Bluetooth device
3. Look for **"ESP32 Typer"** and pair it
4. It will appear as a Bluetooth keyboard — no PIN required

This only needs to be done once. The target PC will remember the pairing.

### On your source PC (the one you control):
1. Open a browser and go to the IP address shown in the Serial Monitor
   (e.g. `http://192.168.1.45`)
2. You will see the ESP32 Typer web interface

### Typing text:
1. On the target PC, click into any text field (document, browser, chat, etc.)
2. On your source PC, paste your text into the web interface
3. Set your preferred typing speed and countdown delay
4. Click **"Type it out"**
5. The countdown runs, then the text is typed on the target PC

---

## Web Interface Controls

| Control | Description |
|---------|-------------|
| Text area | Paste any text here — supports most standard characters |
| Speed slider | Delay between each character (5ms = fast, 100ms = slow). Increase if characters are being dropped |
| Countdown | Seconds to wait before typing starts — gives you time to click a text field on the target |
| Type it out | Sends the text to the ESP32 and begins typing |

---

## Powering the ESP32

Once flashed, the ESP32 does not need to stay connected to your PC. You can power it from:
- Any USB phone charger
- A USB power bank
- Any USB port

It will automatically connect to WiFi on boot and be ready to use.

---

## Troubleshooting

**Can't find the web interface:**
- Check the IP address in the Serial Monitor
- Make sure your source PC is on the same WiFi network as the ESP32
- Try power cycling the ESP32

**Target PC won't pair:**
- Make sure the ESP32 is powered and running (PWR LED on)
- Remove any old "ESP32 Typer" pairings from the target PC and re-pair fresh

**Characters are being dropped or garbled:**
- Increase the typing speed delay using the slider
- Try a value of 30-50ms for slower or older systems

**Upload fails with "Wrong boot mode":**
- Hold the BOOT button on the ESP32 while the IDE shows `Connecting......`
- Release after 2-3 seconds

**Sketch too large error:**
- Make sure you set the partition scheme to **Huge APP** under Tools menu

---

## Notes

- The ESP32 and your source PC must be on the **same WiFi network**
- The target PC only needs Bluetooth — no WiFi, no software, no drivers
- Special characters (emoji, non-Latin scripts) are not supported
- Very long texts (10,000+ characters) may occasionally drop characters — break into chunks if needed
