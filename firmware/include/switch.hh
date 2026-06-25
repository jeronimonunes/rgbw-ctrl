#pragma once

#include <Arduino.h>
#include <functional>

class Switch
{
private:
  static constexpr uint16_t DEBOUNCE = 50;
  static constexpr uint16_t TASK_DELAY_MS = 10;

  gpio_num_t pin;
  bool state = false;
  bool lastStableState = false;
  unsigned long lastChangeTime = 0;

  std::function<void(bool)> callback;

public:
  Switch(const gpio_num_t pin) : pin(pin)
  {
  }

  void begin()
  {
    pinMode(pin, INPUT_PULLUP);
    state = digitalRead(pin) == HIGH;
    lastStableState = state;
  }

  void handle(const unsigned long now)
  {
    bool current = digitalRead(pin) == HIGH;
    if (current != lastStableState)
    {
      if (now - lastChangeTime >= DEBOUNCE)
      {
        lastStableState = current;
        state = current;
        lastChangeTime = now;

        if (callback)
          callback(state);
      }
    }
    else
    {
      lastChangeTime = now; // reset debounce window if stable
    }
  }

  void onChanged(const std::function<void(bool)>& cb)
  {
    this->callback = cb;
  }

  [[nodiscard]] bool isOn() const
  {
    return state;
  }
};
