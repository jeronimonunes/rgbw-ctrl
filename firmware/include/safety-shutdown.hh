#pragma once

#include <Arduino.h>
#include <Preferences.h>

class SafetyShutdown
{
public:
    enum class Mode: uint8_t
    {
        OFF,
        FULL,
        PHASED
    };

private:
    static constexpr auto PREFERENCES_NAME = "safety-shutdown";
    static constexpr auto PREFERENCES_VOLTAGE_KEY = "v";
    static constexpr auto PREFERENCES_MODE_KEY = "m";
    static constexpr auto LOG_TAG = "Safety Shutdown";

    static constexpr uint16_t DEFAULT_SAFETY_SHUTDOWN_VOLTAGE = 22000;
    static constexpr auto DEFAULT_SAFETY_SHUTDOWN_MODE = Mode::OFF;

public:
#pragma pack(push, 1)
    struct Data
    {
        uint16_t milliVolts;
        Mode mode;
    };
#pragma pack(pop)

private:

    uint16_t milliVolts = DEFAULT_SAFETY_SHUTDOWN_VOLTAGE;
    Mode mode = DEFAULT_SAFETY_SHUTDOWN_MODE;

    [[nodiscard]] static Data loadConfigurations()
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, true);
        const auto shutdownVoltage = prefs.getUShort(PREFERENCES_VOLTAGE_KEY, DEFAULT_SAFETY_SHUTDOWN_VOLTAGE);
        const auto shutdownMode = prefs.getUChar(PREFERENCES_MODE_KEY,
                                                 static_cast<uint8_t>(DEFAULT_SAFETY_SHUTDOWN_MODE));
        prefs.end();
        return Data{shutdownVoltage, static_cast<Mode>(shutdownMode)};
    }

    static void saveConfigurations(const Data& data)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME);
        prefs.putUShort(PREFERENCES_VOLTAGE_KEY, data.milliVolts);
        prefs.putUChar(PREFERENCES_MODE_KEY, static_cast<uint8_t>(data.mode));
        prefs.end();
    }

public:
    explicit SafetyShutdown() = default;

    void begin()
    {
        auto [mv, m] = loadConfigurations();
        this->milliVolts = mv;
        this->mode = m;
    }

    [[nodiscard]] Mode shutdownMode(float voltage) const
    {
        if (this->mode == Mode::OFF)
            return Mode::OFF;
        if (voltage < static_cast<float>(this->milliVolts) / 1000.0f)
        {
            return this->mode;
        }
        return Mode::OFF;
    }

    [[nodiscard]] Data getData() const
    {
        return Data{this->milliVolts, this->mode};
    }

    void setData(const Data& data)
    {
        this->milliVolts = data.milliVolts;
        this->mode = data.mode;
        saveConfigurations(data);
    }
};
