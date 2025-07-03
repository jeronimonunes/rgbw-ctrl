# RestHandler – REST API for Device Control

This feature exposes a set of HTTP endpoints using `ESPAsyncWebServer` for controlling and monitoring the device.

---

## ✩ Endpoint Summary

| Method | Path                 | Description                           |
| ------ | -------------------- | ------------------------------------- |
| GET    | `/state`             | Returns the current system state      |
| GET    | `/bluetooth`         | Enables or disables Bluetooth         |
| GET    | `/output/color`      | Updates the device color              |
| GET    | `/output/brightness` | Sets uniform brightness               |
| GET    | `/system/restart`    | Restarts the device                   |
| GET    | `/system/reset`      | Resets the device to factory defaults |

---

## 📘 Detailed Endpoints

### 🔹 `GET /state`

Returns the current device state as JSON.

#### Example response:

```json
{
  "deviceName": "rgbw-ctrl-of-you",
  "firmwareVersion": "3.0.0",
  "heap": 117616,
  "wifi": {
    "details": {
      "ssid": "your_wifi",
      "mac": "00:00:00:00:00:00",
      "ip": "192.168.0.2",
      "gateway": "192.168.0.1",
      "subnet": "255.255.255.0",
      "dns": "192.168.0.1"
    },
    "status": "CONNECTED"
  },
  "alexa": {
    "mode": "rgb_device",
    "names": [
      "led strip",
      "bedroom"
    ]
  },
  "output": [
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    }
  ],
  "ble": {
    "status": "OFF"
  },
  "ota": {
    "status": "Idle",
    "totalBytesExpected": 0,
    "totalBytesReceived": 0
  },
  "espNow": {
    "devices": [
      {
        "name": "bedroom light switch",
        "address": "00:00:00:00:00:00"
      }
    ]
  }
}
``` 

---

### 🎨 `GET /output/color`

Sets the RGBW colors individually.

#### Parameters:

* `r`: Red value (0–255)
* `g`: Green value (0–255)
* `b`: Blue value (0–255)
* `w`: White value (0–255)

#### Example request:

```
GET /output/color?r=255&g=128&b=0&w=0
```

#### Response:

```json
{ "message": "Color updated" }
```

> **Note:** Channels with value > 0 are automatically turned on.

---

### 💡 `GET /output/brightness`

Applies the same brightness value to all channels.

#### Parameters:

* `value`: Intensity (0–255)

#### Example:

```
GET /rest/brightness?value=150
```

#### Response:

```json
{ "message": "OK" }
```

> **Note:** The channel will be turned off if `value == 0`.

---

### 📶 `GET /bluetooth`

Enables or disables Bluetooth.

#### Parameters:

* `state`: `"on"` to enable; any other value disables.

#### Example:

```
GET /bluetooth?state=on
```

#### Responses:

```json
{ "message": "Bluetooth enabled" }
```

```json
{ "message": "Bluetooth disabled" }
```

> Restarts the device after disabling Bluetooth.

---

### ↺ `GET /system/restart`

Restarts the device.

#### Response:

```json
{ "message": "Restarting..." }
```

---

### ⚠️ `GET /system/reset`

Clears the persistent settings and restarts the device.

#### Response:

```json
{ "message": "Resetting to factory defaults..." }
```

---

## 📌 Notes

* All endpoints return JSON and include the header:

  ```
  Cache-Control: no-store
  ```

* The HTTP methods used are `GET`, but for better REST compliance, mutable endpoints should be migrated to `POST`.
