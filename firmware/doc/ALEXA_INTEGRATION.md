# AlexaIntegration

`AlexaIntegration` is a C++ class that integrates RGB, RGBW, or individual channel devices with Amazon Alexa using the `AsyncEspAlexa` library for ESP-based microcontrollers (such as the ESP32). It dynamically manages devices based on persistent settings and synchronizes their state with an `Output` controller.

## Features

* Supports four Alexa integration modes:

    * `OFF`: integration disabled
    * `RGBW_DEVICE`: unified RGB and white channel control
    * `RGB_DEVICE`: RGB control in one device and white channel in another
    * `MULTI_DEVICE`: separate devices for R, G, B, and W channels
* Dynamically creates Alexa devices with custom names
* Saves and restores settings using `Preferences`
* Keeps Alexa in sync with the `Output` state
* Handles Alexa commands through callbacks

## Usage

### Initialization

```cpp
Output output;
AlexaIntegration alexa(output);
alexa.begin();
```

### Main loop

```cpp
void loop() {
    alexa.handle(millis());
}
```

### Custom settings

```cpp
AlexaIntegration::Settings settings;
settings.integrationMode = AlexaIntegration::Settings::Mode::RGBW_DEVICE;
strncpy(settings.deviceNames[0].data(), "Living Room RGBW", settings.MAX_DEVICE_NAME_LENGTH);
alexa.applySettings(settings);
```

You might need to delete the existing devices in your Alexa app and rediscover them to apply the new settings.

## Web Handler

```cpp
server.addHandler(alexa.createAsyncWebHandler());
```

Allows Alexa to discover and control the devices via local protocol.

## Data Structure

### Settings

```cpp
enum class Mode : uint8_t { OFF, RGBW_DEVICE, RGB_DEVICE, MULTI_DEVICE };
std::array<std::array<char, MAX_DEVICE_NAME_LENGTH>, 4> deviceNames;
```

* Device names are used for labeling devices in the Alexa app.
* Indexes represent: `[0] = R or RGB(W), [1] = G, [2] = B, [3] = W`.

## Notes

* The class dynamically allocates memory for devices; pointers are cleared in the destructor.
* Device state is updated every 500 ms if `Output` state has changed.
* Color conversion is handled by `AsyncEspAlexaColorUtils`.

## Dependencies

* [AsyncEspAlexa](https://github.com/kakopappa/arduino-esp8266-alexa-wemo)
* ArduinoJson
* `Output` class (defined externally)

---

This component is ideal for projects requiring local Alexa integration with dynamic control of RGB/RGBW LEDs via voice assistant.

## 📜 License

This class is part of the `rgbw-ctrl` system. Usage is subject to the license defined in the main repository.
