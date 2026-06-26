#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "output_manager.hh"
#include "state_json_filler.hh"

class SafetyShutdown : public BLE::Service, public StateJsonFiller
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

    NimBLECharacteristic* bleSafetyShutdownCharacteristic = nullptr;

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

    Sensor& sensor;
    Output::Manager& outputManager;
    Data config{DEFAULT_SAFETY_SHUTDOWN_MILLI_VOLTS, DEFAULT_SAFETY_SHUTDOWN_MODE};

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

    explicit SafetyShutdown(
        Sensor& sensor,
        Output::Manager& manager):
            sensor(sensor),
            outputManager(manager)
    {
    }

    void begin()
    {
        this->config = loadConfigurations();
    }

    void handle(const unsigned long now) const
    {
        static unsigned long lastRun = now;
        if (now - lastRun < 3000) return;
        lastRun = now;

        switch (this->shutdownMode(sensor.getVoltage()))
        {
        case Mode::OFF:
            break;
        case Mode::FULL:
            this->outputManager.turnOffAll();
            break;
        case Mode::PHASED:
            this->phasedShutdown();
            break;
        }
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

    void fillState(const JsonObject& root) const override
    {
        const auto obj = root["safetyShutdown"].to<JsonObject>();
        obj["shutdownMilliVolts"] = this->config.shutdownMilliVolts;
        switch (this->config.mode)
        {
        case Mode::OFF: obj["mode"] = "OFF";
            break;
        case Mode::FULL: obj["mode"] = "FULL";
            break;
        case Mode::PHASED: obj["mode"] = "PHASED";
            break;
        default: obj["mode"] = "UNKNOWN";
        }
    }

    NimBLEService* createServiceAndCharacteristics(NimBLEServer* server) override
    {
        ESP_LOGI(LOG_TAG, "Creating BLE services and characteristics");
        const auto service = server->getServiceByUUID(BLE::UUID::DEVICE_DETAILS_SERVICE);

        bleSafetyShutdownCharacteristic = service->createCharacteristic(
            BLE::UUID::SAFETY_SHUTDOWN_CHARACTERISTIC,
            READ | WRITE | NOTIFY
        );
        bleSafetyShutdownCharacteristic->setCallbacks(new SafetyShutdownCallback(*this));
        ESP_LOGI(LOG_TAG, "DONE creating BLE services and characteristics");
        return service;
    }

    void clearServiceAndCharacteristics() override
    {
        ESP_LOGI(LOG_TAG, "Clearing all BLE saved pointers");
        bleSafetyShutdownCharacteristic = nullptr;
        ESP_LOGI(LOG_TAG, "DONE clearing all BLE saved pointers");
    }

private:
    void phasedShutdown() const
    {
        if (this->outputManager.isOn(Color::Red))
        {
            this->outputManager.turnOff(Color::Red);
            return;
        }
        if (this->outputManager.isOn(Color::Green))
        {
            this->outputManager.turnOff(Color::Green);
            return;
        }
        if (this->outputManager.isOn(Color::Blue))
        {
            this->outputManager.turnOff(Color::Blue);
            return;
        }
        if (this->outputManager.isOn(Color::White))
        {
            this->outputManager.turnOff(Color::White);
        }
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

    class SafetyShutdownCallback final : public NimBLECharacteristicCallbacks
    {
        SafetyShutdown& safetyShutdown;

    public:
        explicit SafetyShutdownCallback(SafetyShutdown& safetyShutdown)
            : safetyShutdown(safetyShutdown)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto data = safetyShutdown.getData();
            pCharacteristic->setValue(reinterpret_cast<const uint8_t*>(&data), sizeof(data));
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            if (pCharacteristic->getValue().size() != sizeof(Data))
            {
                ESP_LOGE(LOG_TAG, "Invalid safety shutdown data");
                return;
            }
            Data safetyShutdownData = {};
            memcpy(&safetyShutdownData, pCharacteristic->getValue().data(), sizeof(SafetyShutdown::Data));
            safetyShutdown.setConfig(safetyShutdownData);
            pCharacteristic->notify(); // NOLINT
            ESP_LOGI(LOG_TAG, "Safety shutdown changed by BLE");
        }
    };
};
