# Light

The `Light` class provides PWM-based light control for Arduino/ESP32 environments, with support for brightness persistence, perceptual dimming, and JSON state serialization.

## ✨ Features

* On/off state control
* Perceptual brightness adjustment using gamma correction
* Persistent storage of state via `Preferences`
* Optional PWM signal inversion
* JSON-compatible output

## 📦 Dependencies

* `Arduino.h`
* `Preferences.h`
* `cmath`
* `ArduinoJson` (`JsonObject`)
* `hardware.hh` (must provide `Hardware::getPwmChannel(gpio_num_t)`)

## 🧩 Constructor

```cpp
Light(gpio_num_t pin, bool invert = false);
```

* `pin`: GPIO pin used for PWM output
* `invert`: whether to invert the PWM signal (default: `false`)

## 🔧 Main Methods

| Method                  | Description                                            |
|-------------------------|--------------------------------------------------------|
| `setup()`               | Initializes PWM and restores the last persisted state  |
| `handle(unsigned long)` | Should be called periodically to persist state changes |
| `toggle()`              | Toggles the on/off state                               |
| `setValue(uint8_t)`     | Sets brightness (0–255)                                |
| `setOn(bool)`           | Sets the on/off state                                  |
| `increaseBrightness()`  | Increases brightness perceptually                      |
| `decreaseBrightness()`  | Decreases brightness perceptually                      |
| `makeVisible()`         | Ensures light is visible (on with non-zero brightness) |
| `toJson(JsonObject&)`   | Serializes state to JSON                               |

## 📥 Usage Example

```cpp
#include <Arduino.h>
#include "light.hh"

Light light(GPIO_NUM_4);

void setup() {
    Serial.begin(115200);
    light.setup();
}

void loop() {
    light.handle(millis());

    // Toggle light every 5 seconds
    static unsigned long last = 0;
    if (millis() - last > 5000) {
        light.toggle();
        last = millis();
    }
}
```

## 📤 JSON Output

```json
{
  "on": true,
  "value": 128
}
```

## 🧠 Notes

* Persistence keys are derived from the pin number (e.g., `"04o"`, `"04v"`)
* Brightness steps are computed using gamma 2.2 correction for perceptual uniformity
* `handle()` must be called frequently to ensure state is saved reliably

## 📜 License

This class is part of the `rgbw-ctrl` system. Usage is subject to the license defined in the main repository.
