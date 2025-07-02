#pragma once

#include <ESP32RotaryEncoder.h>
#include "hardware.hh"

class RotaryEncoderManager
{

    RotaryEncoder rotaryEncoder = RotaryEncoder(
        static_cast<uint8_t>(Hardware::Pin::Header::H1::P1),
        static_cast<uint8_t>(Hardware::Pin::Header::H1::P2),
        static_cast<uint8_t>(Hardware::Pin::Header::H1::P3)
    );

public:
    void begin()
    {
        pinMode(static_cast<uint8_t>(Hardware::Pin::Header::H1::P4), OUTPUT);
        digitalWrite(static_cast<uint8_t>(Hardware::Pin::Header::H1::P4), LOW);
        this->rotaryEncoder.setEncoderType(FLOATING);
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
