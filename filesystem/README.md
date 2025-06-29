# Filesystem

Web UI assets served directly from the ESP32. Built with Vite and TypeScript.
Requires **Node.js 20+**.

- Source code lives in `src/`.
- Run the following to install dependencies and bundle the assets:
  ```bash
  npm ci
  npm run dev   # start a local preview server
  npm run build
  ```
- The build output is placed in `../firmware/data/` for flashing with PlatformIO or OTA.

Refer to the [root README](../README.md) for details on flashing the filesystem.
