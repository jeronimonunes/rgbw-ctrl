# BLEManager

`BleManager` is a C++ class designed to manage BLE (Bluetooth Low Energy) functionality for an embedded device using the NimBLE library. It provides services and characteristics that expose control and configuration options such as WiFi credentials, device state, heap usage, Alexa integration, and ESP-NOW device synchronization.

---

## Features

* Starts and manages BLE advertising.
* Provides custom BLE services and characteristics:

    * Device name, heap size, firmware version
    * Output color state (with throttled notifications)
    * Alexa settings
    * ESP-NOW device sync
    * WiFi credentials, scan status, and connection status
* Sends periodic BLE notifications for heap and output color state.
* Auto-restarts the device via BLE command.
* Automatically shuts down BLE if no client connects within a timeout.
* Notifies changes in WiFi details dynamically.

---

## BLE Services & Characteristics

### Device Details Service (`12345678-1234-1234-1234-1234567890ac`)

| Characteristic   | UUID                                   | Properties          |
| ---------------- | -------------------------------------- | ------------------- |
| Device Restart   | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000` | Write               |
| Device Name      | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001` | Read, Write, Notify |
| Firmware Version | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002` | Read                |
| HTTP Credentials | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003` | Read, Write         |
| Device Heap      | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004` | Notify              |
| Output Color     | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005` | Read, Write, Notify |
| Alexa Settings   | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006` | Read, Write         |
| ESP-NOW Devices  | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007` | Read, Write         |

### WiFi Service (`12345678-1234-1234-1234-1234567890ab`)

| Characteristic   | UUID                                   | Properties          |
| ---------------- | -------------------------------------- | ------------------- |
| WiFi Details     | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008` | Read, Notify        |
| WiFi Status      | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009` | Read, Write, Notify |
| WiFi Scan Status | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a` | Read, Write, Notify |
| WiFi Scan Result | `aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000b` | Read, Notify        |

---

## Throttling Behavior

To avoid flooding BLE notifications:

* Output color notifications are throttled to once every 500ms.
* Free heap notifications are throttled similarly.

---

## Status Reporting

The current BLE state can be queried using `getStatus()` or `getStatusString()`:

* `OFF` — BLE server is not started.
* `ADVERTISING` — BLE advertising is active.
* `CONNECTED` — A BLE client is connected.

---

## Integration

### Dependencies:

* NimBLE library
* FreeRTOS
* Custom modules:

    * `Output`
    * `WiFiManager`
    * `AlexaIntegration`
    * `WebServerHandler`
    * `ThrottledValue`
    * `async_call`

---

## Usage

```cpp
Output output;
WiFiManager wifiManager;
AlexaIntegration alexaIntegration;
WebServerHandler webHandler;

BleManager ble(output, wifiManager, alexaIntegration, webHandler);
ble.start();

void loop() {
    ble.handle(millis());
}
```

---

## 📜 License

This class is part of the `rgbw-ctrl` system. Usage is subject to the license defined in the main repository.
