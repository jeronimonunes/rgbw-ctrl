# Setup App

Angular application used during initial device configuration. It communicates with the ESP32 over Web Bluetooth to set Wi-Fi credentials and other options.
Requires **Node.js 20+** and npm. Use `npm run build` for production builds.

Common tasks:

```bash
npm ci
npm start
npm run build
```

The compiled app can be deployed to GitHub Pages or any static host via `npm run deploy`.
