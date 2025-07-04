#pragma once

#include <ESP32RotaryEncoder.h>

class RotaryEncoderManager
{
    RotaryEncoder rotaryEncoder;

public:
    explicit RotaryEncoderManager(const gpio_num_t pinA,
                                  const gpio_num_t pinB,
                                  const gpio_num_t pinButton,
                                  const gpio_num_t groundPin = GPIO_NUM_NC,
                                  const gpio_num_t vccPin = GPIO_NUM_NC
    ): rotaryEncoder(pinA, pinB, pinButton)
    {
        if (groundPin != GPIO_NUM_NC)
        {
            pinMode(groundPin, OUTPUT);
            digitalWrite(groundPin, LOW);
            this->rotaryEncoder.setEncoderType(FLOATING);
        }
        if (vccPin != GPIO_NUM_NC)
        {
            pinMode(vccPin, OUTPUT);
            digitalWrite(vccPin, HIGH);
            this->rotaryEncoder.setEncoderType(HAS_PULLUP);
        }
    }


    void begin()
    {
        this->rotaryEncoder.begin(true);
    }

    void onChanged(const std::function<void(long)>& callback)
    {
        this->rotaryEncoder.onTurned(callback);
    }

    void onPressed(const std::function<void(unsigned long)>& callback)
    {
        this->rotaryEncoder.onPressed(callback);
    }

    void setEncoderValue(const long i)
    {
        this->rotaryEncoder.setEncoderValue(i);
    }
};
