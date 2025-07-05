# Firmware

ESP32 source code for the RGBW controller. This PlatformIO project builds two firmware images:
the main controller (`controller.cpp`) and an optional ESP-NOW remote (`remote.cpp`).
Requires the PlatformIO CLI (version 6+) or VS Code extension. Building the bundled web UI also requires **Node.js 20+**.

- Edit code in `src/` and headers in `include/`.
- Configuration lives in `platformio.ini` and `partitions.csv`.
- Build and upload using [PlatformIO](https://platformio.org/):
  ```bash
  # main controller
  pio run -e controller
  pio run -e controller -t upload
  # optional remote
  pio run -e remote
  pio run -e remote -t upload
  ```
- The compiled web UI from `../filesystem` is copied to the `data/` directory before uploading.

See the [root README](../README.md) for full build and flashing instructions.
