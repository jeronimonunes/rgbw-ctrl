# ESP-NOW Remote Firmware

This optional firmware turns an ESP32 board into a dedicated remote for `rgbw-ctrl`. It sends simple commands to the controller using ESP-NOW and shares much of the same infrastructure (BLE setup, Wi-Fi, OTA updates).

## Capabilities

- Toggle the RGBW controller output via the on-board button
- Adjust brightness with an optional rotary encoder
- Pair with a controller over BLE to store its MAC address
- OTA update support using the same `/update` endpoint

## Building and Flashing

From the `firmware` directory run:

```bash
pio run -e remote
pio run -e remote -t upload
```

See the [main README](../README.md) for general build prerequisites.
