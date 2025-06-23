# RGBW Controller

ESP32 RGBW LED controller with web-based configuration and Alexa support. The firmware exposes Bluetooth Low Energy for initial setup, Wi‑Fi connectivity, a REST API, real‑time WebSocket control and OTA updates.

## Project structure

| Path | Purpose |
| --- | --- |
| [/firmware](firmware) | ESP32 PlatformIO code |
| [/filesystem](filesystem) | Device web UI assets |
| [/app](app) | Angular setup app |
| [/doc](doc) | Additional documentation |

## BOOT button behaviour

- **Short press (<2.5&nbsp;s):** toggle light output.
- **Long press (>2.5&nbsp;s):** enable Bluetooth or reset its timer.
- **Hold BOOT while pressing RESET:** enter UART flashing mode.

Bluetooth advertising stops automatically after 15&nbsp;s if unused.

## Board LED status

| State | Color | Behavior | Description |
| --- | --- | --- | --- |
| OTA update running | Purple | Fading | Firmware or filesystem upload in progress |
| BLE client connected | Yellow | Steady | Active BLE connection |
| BLE advertising | Blue | Fading | Waiting for a BLE connection |
| Wi‑Fi scan running | Yellow | Fading | Searching for networks |
| Wi‑Fi connected | Green | Steady | Connected to a Wi‑Fi network |
| Wi‑Fi disconnected | Red | Steady | No Wi‑Fi connection |

## WebSocket messages

| Type | Description |
| --- | --- |
| `ON_COLOR` | Update RGBA values |
| `ON_BLE_STATUS` | Toggle Bluetooth |
| `ON_DEVICE_NAME` | Set the device name |
| `ON_HTTP_CREDENTIALS` | Update HTTP basic auth |
| `ON_WIFI_STATUS` | Connect to Wi‑Fi |
| `ON_WIFI_SCAN_STATUS` | Trigger Wi‑Fi scan |
| `ON_WIFI_DETAILS` | Reserved |
| `ON_OTA_PROGRESS` | Reserved |
| `ON_ALEXA_INTEGRATION_SETTINGS` | Update Alexa options |

## REST API

* `GET /rest/state` – full device state
* `GET /rest/system/restart` – reboot device
* `GET /rest/system/reset` – factory reset
* `GET /rest/bluetooth?state=on|off` – enable/disable BLE

All endpoints use Basic Authentication. See [doc/ASYNC_CALL.md](doc/ASYNC_CALL.md) and related documents for implementation details.

## OTA update

Uploads are performed via `POST /update`. Pass `name=filesystem` to update the web UI instead of the firmware. Optional `md5` ensures file integrity. Detailed behaviour is documented in [doc/OTA.md](doc/OTA.md).

## Building & flashing

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
