#pragma once

#include "color.hh"
#include "light.hh"
#include "hardware.hh"

#include <array>
#include <Arduino.h>
#include <algorithm>

class Output
{
public:
#pragma pack(push, 1)
    struct State
    {
        std::array<Light::State, 4> values = {};

        bool operator==(const State& other) const
        {
            return values == other.values;
        }

        bool operator!=(const State& other) const
        {
            return values != other.values;
        }
    };
#pragma pack(pop)

private:
    std::array<Light, 4> lights = {
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::RED))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::GREEN))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::BLUE))),
        Light(static_cast<gpio_num_t>(static_cast<uint8_t>(Hardware::Pin::Output::WHITE)))
    };

    static_assert(static_cast<size_t>(Color::White) < 4, "Color enum out of bounds");

public:
    void begin()
    {
        for (auto& light : lights)
            light.setup();
    }

    void handle(const unsigned long now)
    {
        for (auto& light : lights)
            light.handle(now);
    }

    void setValue(const uint8_t value, Color color)
    {
        return lights.at(static_cast<size_t>(color)).setValue(value);
    }

    void setOn(const bool on, Color color)
    {
        return lights.at(static_cast<size_t>(color)).setOn(on);
    }

    void toggleAll()
    {
        const bool on = anyVisible();
        for (auto& light : lights)
        {
            if (on)
            {
                light.setOn(false);
            }
            else
            {
                light.setOn(true);
                light.setValue(Light::ON_VALUE);
            }
        }
    }

    void increaseBrightness()
    {
        for (auto& light : lights)
            light.increaseBrightness();
    }

    void decreaseBrightness()
    {
        for (auto& light : lights)
            light.decreaseBrightness();
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        lights.at(static_cast<size_t>(Color::Red)).setValue(r);
        lights.at(static_cast<size_t>(Color::Green)).setValue(g);
        lights.at(static_cast<size_t>(Color::Blue)).setValue(b);
    }

    void setColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t w)
    {
        lights.at(static_cast<size_t>(Color::Red)).setValue(r);
        lights.at(static_cast<size_t>(Color::Green)).setValue(g);
        lights.at(static_cast<size_t>(Color::Blue)).setValue(b);
        lights.at(static_cast<size_t>(Color::White)).setValue(w);
    }

    void setState(const State& state)
    {
        for (size_t i = 0; i < std::min(lights.size(), state.values.size()); ++i)
            lights.at(i).setState(state.values[i]);
    }

    void toJson(const JsonArray& to) const
    {
        for (const auto& light : lights)
            light.toJson(to.add<JsonObject>());
    }

    [[nodiscard]] bool anyOn() const
    {
        return std::any_of(lights.begin(), lights.end(),
                           [](const Light& light) { return light.isOn(); });
    }

    [[nodiscard]] bool anyVisible() const
    {
        return std::any_of(lights.begin(), lights.end(),
                           [](const Light& light) { return light.isVisible(); });
    }

    [[nodiscard]] uint8_t getValue(Color color) const
    {
        return lights.at(static_cast<size_t>(color)).getValue();
    }

    [[nodiscard]] bool isOn(Color color) const
    {
        return lights.at(static_cast<size_t>(color)).isOn();
    }

    [[nodiscard]] std::array<uint8_t, 4> getValues() const
    {
        std::array<uint8_t, 4> output = {};
        std::transform(lights.begin(), lights.end(), output.begin(), [](const auto& light)
        {
            return light.getValue();
        });
        return output;
    }

    [[nodiscard]] State getState() const
    {
        std::array<Light::State, 4> state;
        std::transform(lights.begin(), lights.end(), state.begin(),
                       [](const Light& light) { return light.getState(); });
        return {state};
    }
};
