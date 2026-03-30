# ESP32 Typer — USB HID Edition

A USB HID keyboard emulator built on the ESP32-S2 Mini. Paste text into a web interface on any device on your local network, and the ESP32 types it out via USB into whatever is focused on the target PC — no software, drivers, or Bluetooth pairing required on the target machine.

This project is a fork/adaptation of [jdcb4/KeyboardClipboard](https://github.com/jdcb4/KeyboardClipboard), reworked to use USB HID instead of Bluetooth, with added features including multi-network WiFi fallback, mDNS, and a custom HTML login page.

---

## How It Works

```
Your Phone / Laptop / Any Device         Target PC
┌─────────────────────────┐             ┌──────────────┐
│                         │   WiFi      │              │
│  Browser → Web UI       │ ──────────▶ │              │
│  (paste text, click     │   ESP32     │  Sees a USB  │
│   "Type it out")        │ ◀────────── │  keyboard    │
└─────────────────────────┘   USB HID   └──────────────┘
```

- The ESP32-S2 Mini connects to your WiFi and hosts a simple web page
- You open that page on any device on the same network and paste your text
- The ESP32 types the text via USB HID into whatever is focused on the target PC
- The target PC sees it as a standard USB keyboard — no drivers, no pairing, no software

---

## Hardware Required

| Item | Notes |
|---|---|
| WEMOS LOLIN S2 Mini (ESP32-S2) | Must be an ESP32-S2 or S3 variant — the original ESP32 does not support native USB HID |
| USB-C cable (×2) | One for flashing (UART port), one for connecting to the target PC (USB port) |

> **Important:** The S2 Mini has two USB-C ports on the board edge. The port near the reset button is the UART port (for flashing). The other port is the native USB port (for HID — plug this into the target PC).

---

## Features

- **USB HID** — no Bluetooth, no drivers, no pairing on the target PC
- **Web UI** — paste text and trigger typing from any browser on your network
- **Custom login page** — session-based authentication to protect the web interface
- **Multi-network WiFi** — define multiple SSIDs, the device tries each in order on boot
- **mDNS** — access the UI at `http://esp32typer.local` without needing to know the IP
- **Adjustable typing speed** — slider to tune delay between characters
- **Countdown timer** — gives you time to click a text field on the target PC before typing starts
- **Logout button** — cleanly ends your session from the web UI

---

## Requirements

### Software
- [PlatformIO IDE](https://platformio.org/) (VS Code extension recommended)
- No additional libraries required — USB HID is built into the ESP32-S2 Arduino core

### PlatformIO Configuration (`platformio.ini`)

```ini
[env:lolin_s2_mini]
platform = espressif32
board = lolin_s2_mini
framework = arduino

board_build.partitions = no_ota.csv

build_flags =
    -DARDUINO_USB_MODE=0
    -DARDUINO_USB_CDC_ON_BOOT=1
```

---

## Quick Start

### 1. Clone the repo

```bash
git clone https://github.com/Velicoma/esp32-typer.git
cd esp32-typer
```

### 2. Create your secrets file

Create `src/secrets.h` — this file is excluded from Git (see `.gitignore`) so your credentials are never committed.

```cpp
// src/secrets.h
#ifndef SECRETS_H
#define SECRETS_H

struct WifiNetwork {
  const char* ssid;
  const char* password;
};

WifiNetwork networks[] = {
  { "YOUR_WIFI_SSID",       "YOUR_WIFI_PASSWORD"       },
  { "YOUR_BACKUP_WIFI_SSID", "YOUR_BACKUP_WIFI_PASSWORD" },
};
const int networkCount = sizeof(networks) / sizeof(networks[0]);

const char* webUsername = "admin";
const char* webPassword = "yourpassword";

#endif
```

- Add as many WiFi networks as you need — the device will try each in order on boot
- Change `webUsername` and `webPassword` to whatever you like
- The mDNS hostname can be changed in `main.cpp` by editing the `MDNS.begin("esp32typer")` line

### 3. Flash the firmware

Connect the **UART port** (the USB-C port near the reset button) to your PC.

In PlatformIO, run:
- **Clean** → **Build** → **Upload**

If the upload hangs on `Connecting...`, hold the **BOOT** button on the board for 2–3 seconds until it proceeds.

### 4. Connect to the target PC

Once flashed, disconnect from the UART port and connect the **USB port** (the other USB-C port) to the target PC. It will appear as a USB keyboard automatically.

The UART port can remain connected to a power source or your PC for Serial Monitor access — it is not required for normal operation.

### 5. Find the web interface

On first boot, the device will print its IP address to the Serial Monitor (115200 baud). After that you can always reach it at:

```
http://esp32typer.local
```

> **Windows users:** `.local` addresses require Bonjour to be installed. iTunes installs it automatically. Windows 11 includes it natively.

### 6. Log in and use it

1. Open `http://esp32typer.local` in a browser on any device on the same network
2. Log in with the credentials you set in `secrets.h`
3. On the target PC, click into any text field
4. Paste your text into the web UI, set your speed and countdown, and click **Type it out**

---

## Project Structure

```
esp32-typer/
├── src/
│   ├── main.cpp          # Main firmware
│   └── secrets.h         # Your credentials — NOT committed to Git
├── platformio.ini        # PlatformIO build configuration
├── .gitignore
└── README.md
```

---

## .gitignore

Ensure your `.gitignore` includes the following to prevent credentials from being committed:

```
.pio/
.vscode/
src/secrets.h
```

---

## Troubleshooting

**Upload fails with `Connecting...`**
Hold the BOOT button on the ESP32 while the IDE shows `Connecting...` and release after 2–3 seconds.

**Can't reach the web interface**
- Check your router's DHCP table for a device named `espressif` or `lolin` to confirm it connected to WiFi
- Make sure your phone/laptop is on the same WiFi network as the ESP32
- Try the IP address directly if `esp32typer.local` doesn't resolve

**Characters are dropped or garbled on the target PC**
Increase the typing delay using the speed slider. Try 30–50ms for older or slower systems.

**Device not appearing as a keyboard on the target PC**
Make sure you are using the correct USB-C port (the native USB port, not the UART port near the reset button). Try a different USB cable — some cables are charge-only.

**Build fails with `sdkconfig.h` not found**
Remove any `board_build.arduino.memory_type` line from `platformio.ini` if present, then do a clean build.

---

## Credits

Based on [jdcb4/KeyboardClipboard](https://github.com/jdcb4/KeyboardClipboard).
USB HID adaptation, multi-network WiFi, mDNS, and login page added separately.
