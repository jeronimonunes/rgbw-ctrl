# Output Class - RGBW Light Control

This class encapsulates control logic for an RGBW lighting system using four independent light channels (Red, Green, Blue, White). It is designed to be used with Arduino-based hardware and supports a range of operations such as brightness adjustment, toggling, and JSON serialization.

## Overview

The `Output` class provides an abstraction over an array of four `Light` objects. Each `Light` corresponds to a color channel and is associated with a specific GPIO pin.

## Key Components

### State Struct

* Holds the on/off status and intensity value for each color.
* Offers helper methods to check if any light is on and to get per-color status.

### Core Methods

* `begin()`: Initializes hardware.
* `handle(now)`: Updates light states based on the current time.
* `setValue(value, color)`: Sets the brightness value of a specific color.
* `setOn(on, color)`: Turns a specific color on or off.
* `toggle(color)`: Toggles visibility for a specific color.
* `toggleAll()`: Toggles all lights based on current visibility.
* `increaseBrightness()`, `decreaseBrightness()`: Adjusts brightness.
* `setColor(r, g, b)` / `setColor(r, g, b, w)`: Sets RGB or RGBW colors.
* `setAll(value, on)`: Applies the same value and state to all colors.
* `setState(state)`: Loads a complete state object.
* `toJson(jsonArray)`: Serializes the current light states to JSON.

### Getters

* `anyOn()`, `anyVisible()`: State checks for lights.
* `getValue(color)`: Returns current brightness of a color.
* `isOn(color)`: Returns on/off status.
* `getValues()`: Returns all brightness values as an array.
* `getState()`: Returns a full snapshot of the current light states.

## Dependencies

* `color.hh`: Defines the `Color` enum.
* `light.hh`: Contains the `Light` class and `Light::State` struct.
* `hardware.hh`: Provides GPIO pin mapping.
* Arduino core libraries.

## Use Case

This class is ideal for embedded lighting control in RGBW LED systems where fine-grained control over each color channel is necessary. It abstracts away the lower-level GPIO handling and provides high-level operations that can be easily integrated into an application loop.

## 📜 License

This class is part of the `rgbw-ctrl` system. Usage is subject to the license defined in the main repository.
