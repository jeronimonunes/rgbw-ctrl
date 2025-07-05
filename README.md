# RGBW Controller

**ESP32-based RGBW LED Controller with Web UI & Alexa Integration**

**rgbw-ctrl** is a versatile, powerful controller designed for RGBW LED strips. It runs on the ESP32 platform and offers a seamless setup and control experience via Bluetooth, Wi-Fi, and a browser-based interface. Integration with Amazon Alexa, REST APIs, and WebSocket communication makes it ideal for hobbyists, smart home enthusiasts, and developers.

---

## 🚀 Features

* **Bluetooth Setup:** Fast and easy pairing for initial configuration
* **Wi-Fi Management:** Configure, scan, and connect to networks effortlessly
* **Alexa Integration:** Native support for voice control
* **Web Interface:** Sleek UI for real-time RGBW control
* **REST API:** Full-featured JSON API for remote control
* **WebSocket:** Low-latency bidirectional communication
* **OTA Updates:** Update firmware and UI over HTTP
* **Rotary Encoder Support:** Optional hardware input for manual brightness control and BLE activation

---

## 🎛️ Rotary Encoder Integration

The device supports an optional rotary encoder connected via the H1 header. This allows for:

* Turning lights on/off with a short press
* Enabling Bluetooth with a long press
* Adjusting brightness by rotating left or right

To use this feature, connect a compatible rotary encoder to the H1 pins as follows:

| Pin | Purpose          |
| --- | ---------------- |
| P1  | Encoder CLK      |
| P2  | Encoder DT       |
| P3  | Encoder SW (btn) |
| P4  | GND              |

Functionality is built-in and automatically enabled if the encoder is connected.

## 🧰 Requirements

* Node.js 20+
* npm
* PlatformIO CLI 6+

---

## 📁 Project Structure

| Path          | Description                     |
| ------------- | ------------------------------- |
| `/firmware`   | PlatformIO-based ESP32 firmware |
| `/filesystem` | Web-based device UI             |
| `/app`        | Angular configuration app       |
| `/doc`        | Additional documentation        |

---

## ⚡ Quick Start

1. Build the web UI:

   ```bash
   cd filesystem && npm ci && npm run build
   ```
2. Set up the configuration app:

   ```bash
   cd ../app && npm ci
   ```
3. Upload firmware to the board:

   ```bash
   cd ../firmware
   pio run -t upload
   ```
4. Upload web UI files:

   ```bash
   pio run -t uploadfs
   ```

---

## 🧩 Configuration App (Angular)

Provides a Web Bluetooth-based UI to configure the controller.

### Getting Started

```bash
cd app
npm install
npm start
```

Build production version:

```bash
npm run build
```

Deploy via GitHub Pages:

```bash
npm run deploy
```

---

## 🔘 BOOT Button Behavior

* **Short Press (< 2.5s):** Toggle light on/off
* **Long Press (> 2.5s):** Enable Bluetooth or reset its timeout
* **Firmware Update Mode:** Hold BOOT, press RESET

Bluetooth auto-disables after 15 seconds if unused.

---

## 💡 Board LED Status

| State              | Color     | Behavior | Description                     |
| ------------------ | --------- | -------- | ------------------------------- |
| OTA update         | 🔮 Purple | Fading   | Firmware update in progress     |
| BLE connected      | 🟡 Yellow | Steady   | Active BLE connection           |
| BLE advertising    | 🔵 Blue   | Fading   | Waiting for BLE connection      |
| Wi-Fi scanning     | 🟡 Yellow | Fading   | Searching for Wi-Fi networks    |
| Wi-Fi connected    | 🟢 Green  | Steady   | Successfully connected to Wi-Fi |
| Wi-Fi disconnected | 🔴 Red    | Steady   | No Wi-Fi connection available   |

---

## 🌐 REST API

The device offers a simple HTTP-based control interface using `GET` requests. While it resembles a REST API, all actions (even mutable ones) use `GET` methods.

### ✩ Endpoint Summary

| Method | Path                 | Description                           |
| ------ | -------------------- | ------------------------------------- |
| GET    | `/state`             | Returns the current system state      |
| GET    | `/bluetooth`         | Enables or disables Bluetooth         |
| GET    | `/output/color`      | Updates the device color              |
| GET    | `/output/brightness` | Sets uniform brightness               |
| GET    | `/system/restart`    | Restarts the device                   |
| GET    | `/system/reset`      | Resets the device to factory defaults |

### 📘 Detailed Endpoints

#### `GET /state`

Returns the complete system state as a JSON object.

#### `GET /output/color`

Sets the RGBW LED color channels individually.

* Parameters: `r`, `g`, `b`, `w` (0–255)
* Example: `/output/color?r=255&g=100&b=50&w=0`
* Turns channels ON automatically if value > 0.

#### `GET /output/brightness`

Applies a uniform brightness level to all channels.

* Parameter: `value` (0–255)
* Value of 0 turns channels OFF.

#### `GET /bluetooth`

Toggles Bluetooth functionality.

* Parameter: `state=on` enables; any other value disables
* Example: `/bluetooth?state=on`
* Disabling Bluetooth causes the device to restart.

#### `GET /system/restart`

Restarts the device gracefully.

#### `GET /system/reset`

Performs a full factory reset (clears persistent storage) and restarts the device.

### Notes

* All endpoints return JSON and use the header:

  ```
  Cache-Control: no-store
  ```
* For true REST semantics, POST/PUT should replace some GETs.

## 🔄 WebSocket Protocol

Efficient binary-encoded control channel. Below are the supported message types:

| Type                            | Description                                                           |
| ------------------------------- | --------------------------------------------------------------------- |
| `ON_HEAP`                       | Sends free heap memory info (sent periodically, not client-triggered) |
| `ON_DEVICE_NAME`                | Updates the device name                                               |
| `ON_FIRMWARE_VERSION`           | Sends current firmware version                                        |
| `ON_COLOR`                      | Sets the RGBW LED output state                                        |
| `ON_HTTP_CREDENTIALS`           | Updates HTTP basic authentication credentials                         |
| `ON_BLE_STATUS`                 | Enables or disables Bluetooth advertising                             |
| `ON_WIFI_STATUS`                | Sends current Wi-Fi connection status                                 |
| `ON_WIFI_SCAN_STATUS`           | Triggers a scan for nearby Wi-Fi networks                             |
| `ON_WIFI_DETAILS`               | Sends detailed Wi-Fi configuration information                        |
| `ON_WIFI_CONNECTION_DETAILS`    | Connects to a Wi-Fi network using given credentials                   |
| `ON_OTA_PROGRESS`               | Sends OTA firmware update progress status                             |
| `ON_ALEXA_INTEGRATION_SETTINGS` | Updates Alexa integration preferences                                 |
| `ON_ESP_NOW_DEVICES`            | Sends a list of ESP-NOW connected devices                             |
| `ON_ESP_NOW_CONTROLLER`         | Sends the MAC address of the paired ESP-NOW controller                |

---

## 🔧 OTA Updates

Firmware and filesystem updates are supported over-the-air using HTTP POST.

### Endpoint

* `POST /update`

  * Parameters:

    * `file`: Required firmware or filesystem binary
    * `name`: Optional; use `filesystem` to indicate a filesystem image
    * `md5`: Optional; 32-character hash for file integrity verification

### Examples

```bash
curl -u user:pass -F "file=@firmware.bin" http://<device-ip>/update
curl -u user:pass -F "name=filesystem" -F "file=@littlefs.bin" http://<device-ip>/update
curl -u user:pass -F "file=@firmware.bin" "http://<device-ip>/update?md5=<hash>"
```

### Notes

* OTA updates are protected by Basic Auth
* Only one upload is accepted at a time; others are rejected
* If `md5` is provided and doesn't match, update is aborted
* The device restarts automatically after a successful upload

See full details in the [OtaHandler documentation](doc/OTA.md).

---

## 🛠️ Build & Flash

```bash
cd filesystem && npm install && npm run build
cd ../firmware && pio run -t upload
pio run -t uploadfs  # or use curl + /update endpoint
```

---

## 📄 License

MIT License. See [LICENSE](LICENSE).

More info:

* [firmware/README.md](firmware/README.md)
* [filesystem/README.md](filesystem/README.md)
* [app/README.md](app/README.md)
* Docs in [`/doc`](doc) folder
