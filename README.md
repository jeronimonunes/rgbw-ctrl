# RGBW Controller

ESP32 RGBW LED controller with web-based configuration and Alexa support. The firmware exposes Bluetooth Low Energy for initial setup, Wi‑Fi connectivity, a REST API, real‑time WebSocket control and OTA updates.
## Requirements
- Node.js 20+ and npm
- PlatformIO CLI 6+


## Project structure

| Path | Purpose |
| --- | --- |
| [/firmware](firmware) | ESP32 PlatformIO code |
| [/filesystem](filesystem) | Device web UI assets |
| [/app](app) | Angular setup app |
| [/doc](doc) | Additional documentation |

## Quick start

1. Install Node.js dependencies for the UI projects:
   ```bash
   cd filesystem && npm ci && npm run build
   cd ../app && npm ci
   ```
2. Compile and upload the firmware with PlatformIO:
   ```bash
   cd ../firmware
   pio run -t upload
   ```
3. Upload the bundled web UI:
   ```bash
   pio run -t uploadfs   # via USB
   ```

## Angular Configuration App

The `app` directory houses an Angular project used to configure the controller via Web Bluetooth.

### Prerequisites
- Node.js 20+
- npm

### Getting Started
```bash
cd app
npm install
npm start
```
Use `npm run build` to create production files. The BLE connection workflow along with Alexa and Wi-Fi settings forms is implemented in `src/app`.

You can deploy the built project to GitHub Pages using `npm run deploy`.


## BOOT Button Behavior
The BOOT button on the board has multiple context-sensitive behaviors:

Short Press (< 2.5s)
🟢 Toggles the board's lights on or off, depending on the current state.

Long Press (> 2.5s)
🔵 Enables Bluetooth, if it’s not already active.
🔄 If Bluetooth is already on, it simply resets its 15s auto-shutdown timer.

⏱️ Bluetooth automatically disables after 15 seconds if no connection is established.

Firmware Update Mode
🛠️ To enter UART firmware update mode:

Hold the BOOT button,

Then press RESET.

## Board LED Status Indicator

The `BoardLED` class provides a simple, visual representation of system status using the onboard RGB LED. It conveys Bluetooth, Wi-Fi, and OTA update states using colors and effects (steady or fading).

### Features

* ✨ **Smooth fade blinking** for advertising and scanning states
* 🟢 **Static colors** for stable statuses
* 🎯 **Priority handling** to ensure the most critical state is always shown

### Color Codes & Behavior

| State                   | Color      | Behavior | Description                             |
| ----------------------- | ---------  | -------- | --------------------------------------- |
| 🔄 OTA update running   | 🟣 Purple | Fading   | Indicates firmware update in progress   |
| 🤝 BLE client connected | 🟡 Yellow | Steady   | Device is actively connected via BLE    |
| 📡 BLE advertising      | 🔵 Blue   | Fading   | BLE is active, waiting for a connection |
| 📶 Wi-Fi scan running   | 🟡 Yellow | Fading   | Scanning for available Wi-Fi networks   |
| 🌐 Wi-Fi connected      | 🟢 Green  | Steady   | Device is connected to a Wi-Fi network  |
| ❌ Wi-Fi disconnected   | 🔴 Red    | Steady   | No Wi-Fi connection available           |

## WebSocket Features

This firmware includes a WebSocket handler that supports real-time, bidirectional communication between the device and the front-end interface.

### 🔌 Supported WebSocket Message Types

| Type                            | Description                                 |
|-------------------------------- |---------------------------------------------|
| `ON_COLOR`                      | Update the LED RGBA values                  |
| `ON_BLE_STATUS`                 | Toggle Bluetooth ON/OFF                     |
| `ON_DEVICE_NAME`                | Set the device name                         |
| `ON_HTTP_CREDENTIALS`           | Update HTTP basic auth credentials          |
| `ON_WIFI_STATUS`                | Connect to a Wi-Fi network                  |
| `ON_WIFI_SCAN_STATUS`           | Trigger a Wi-Fi scan                        |
| `ON_WIFI_DETAILS`               | Reserved for future                         |
| `ON_OTA_PROGRESS`               | Reserved for future                         |
| `ON_ALEXA_INTEGRATION_SETTINGS` | Update Alexa integration preferences        |

Messages are binary-encoded and processed asynchronously to prevent blocking the main execution loop. RGBW sliders and Bluetooth control UI are bound directly to these messages via a browser-based WebSocket connection.

## REST API

The device exposes a RESTful interface for status retrieval and control.

### 📍 Available Endpoints

#### `GET /rest/state`
Returns a JSON document with full system state:

```json
{
  "deviceName": "rgbw-ctrl-of-you",
  "firmwareVersion": "1.0.0",
  "heap": 117380,
  "wifi": {
    "details": {
      "ssid": "my-wifi",
      "mac": "00:00:00:00:00:00",
      "ip": "192.168.0.2",
      "gateway": "192.168.0.1",
      "subnet": "255.255.255.0",
      "dns": "1.1.1.1"
    },
    "status": "CONNECTED"
  },
  "alexa": {
    "mode": "rgbw_device",
    "names": [
      "led strip"
    ]
  },
  "output": [
    { "state": "off", "value": 255 },
    { "state": "off", "value": 255 },
    { "state": "off", "value": 255 },
    { "state": "off", "value": 255 }
  ],
  "ble": {
    "status": "OFF"
  },
  "ota": {
    "state": "Idle"
  }
}
```

#### `GET /rest/system/restart`
Restarts the device after sending a response.

#### `GET /rest/system/reset`
Performs a factory reset (clears NVS), stops BLE, and restarts.

#### `GET /rest/bluetooth?state=on|off`
Enables or disables Bluetooth based on the query parameter.

- `state=on` → enables BLE
- `state=off` → disables BLE

---

### 🔒 Authentication

REST endpoints use the same authentication as the web server and OTA.

## OTA Update via Web Server

This project includes support for OTA (Over-the-Air) firmware and filesystem updates via HTTP POST requests.

### 📡 Supported Upload Targets

* **Firmware**
* **Filesystem** (works only with LittleFS partitions)

---

### 🔒 Authentication

OTA endpoints are protected using Basic Authentication. Unauthorized requests receive `401 Unauthorized`.

---

### 🔧 Endpoints

#### `POST /update`

Uploads a new firmware or filesystem image.

##### Parameters

| Parameter | Type   | Required  | Description                          |
| --------- | ------ | --------- | ------------------------------------ |
| `name`    | string | ❌ No     | `filesystem` (default is firmware)   |
| `md5`     | string | ❌ No     | 32-char hex string to validate file  |

##### Example

**Upload firmware:**

Example (firmware):

```bash
curl -u user:pass -F "file=@firmware.bin" http://<device-ip>/update
```

Example (filesystem):

```bash
curl -u user:pass -F "name=filesystem" -F "file=@littlefs.bin" http://<device-ip>/update
```

Example with MD5:

```bash
curl -u user:pass -F "file=@firmware.bin" "http://<device-ip>/update?md5=d41d8cd98f00b204e9800998ecf8427e"
```

---

### 🔁 Auto-Restart

After a successful OTA update, the device automatically restarts.
No state is lost, and the new firmware or filesystem is immediately active.
More details available in the [OtaHandler documentation](doc/OTA.md).


## ⚙️ Building & flashing

1. Build the filesystem assets:
   ```bash
   cd filesystem
   npm install
   npm run build
   ```
2. Compile and upload the firmware with PlatformIO:
   ```bash
   cd ../firmware
   pio run -t upload
   ```
3. Upload the filesystem:
   ```bash
   pio run -t uploadfs   # via USB
   # or
   curl -u user:pass -F "name=filesystem" -F "file=@littlefs.bin" http://<device-ip>/update
   ```

## License

This project is licensed under the [MIT License](LICENSE).

See the subproject documentation for more details:

- [firmware/README.md](firmware/README.md)
- [filesystem/README.md](filesystem/README.md)
- [app/README.md](app/README.md)
- Additional docs in the [doc](doc) directory.
