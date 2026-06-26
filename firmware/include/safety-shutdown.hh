#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "state_json_filler.hh"

class SafetyShutdown : public StateJsonFiller
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

    static constexpr uint16_t DEFAULT_SAFETY_SHUTDOWN_MILLI_VOLTS = 22000;
    static constexpr auto DEFAULT_SAFETY_SHUTDOWN_MODE = Mode::OFF;

public:
#pragma pack(push, 1)
    struct Data
    {
        uint16_t shutdownMilliVolts;
        Mode mode;

        bool operator==(const Data& other) const
        {
            return shutdownMilliVolts == other.shutdownMilliVolts && mode == other.mode;
        }

        bool operator!=(const Data& other) const
        {
            return shutdownMilliVolts != other.shutdownMilliVolts || mode != other.mode;
        }
    };
#pragma pack(pop)

private:

    Data config {DEFAULT_SAFETY_SHUTDOWN_MILLI_VOLTS, DEFAULT_SAFETY_SHUTDOWN_MODE };

    [[nodiscard]] static Data loadConfigurations()
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, true);
        const auto shutdownVoltage = prefs.getUShort(PREFERENCES_VOLTAGE_KEY, DEFAULT_SAFETY_SHUTDOWN_MILLI_VOLTS);
        const auto shutdownMode = prefs.getUChar(PREFERENCES_MODE_KEY,
                                                 static_cast<uint8_t>(DEFAULT_SAFETY_SHUTDOWN_MODE));
        prefs.end();
        return Data{shutdownVoltage, static_cast<Mode>(shutdownMode)};
    }

    static void saveConfigurations(const Data& data)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME);
        prefs.putUShort(PREFERENCES_VOLTAGE_KEY, data.shutdownMilliVolts);
        prefs.putUChar(PREFERENCES_MODE_KEY, static_cast<uint8_t>(data.mode));
        prefs.end();
    }

public:
    explicit SafetyShutdown() = default;

    void begin()
    {
        this->config = loadConfigurations();
    }

    [[nodiscard]] Mode shutdownMode(const float voltage) const
    {
        if (this->config.mode == Mode::OFF)
            return Mode::OFF;
        if (voltage < static_cast<float>(this->config.shutdownMilliVolts) / 1000.0f)
        {
            return this->config.mode;
        }
        return Mode::OFF;
    }

    [[nodiscard]] Data getData() const
    {
        return this->config;
    }

    void setConfig(const Data& data)
    {
        this->config = data;
        saveConfigurations(data);
    }

    void fillState(const JsonObject& obj) const override
    {
        obj["shutdownMilliVolts"] = this->config.shutdownMilliVolts;
        switch (this->config.mode)
        {
            case Mode::OFF: obj["mode"] = "OFF"; break;
            case Mode::FULL: obj["mode"] = "FULL"; break;
            case Mode::PHASED: obj["mode"] = "PHASED"; break;
            default : obj["mode"] = "UNKNOWN";
        }
    }
};
