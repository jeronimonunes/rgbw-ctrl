# Firmware

ESP32 source code for the RGBW controller. This PlatformIO project builds the firmware that runs on the device.

- Edit code in `src/` and headers in `include/`.
- Configuration lives in `platformio.ini` and `partitions.csv`.
- Build and upload using [PlatformIO](https://platformio.org/):
  ```bash
  pio run
  pio run -t upload
  ```
- The compiled web UI from `../filesystem` is copied to the `data/` directory before uploading.

See the [root README](../README.md) for full build and flashing instructions.
