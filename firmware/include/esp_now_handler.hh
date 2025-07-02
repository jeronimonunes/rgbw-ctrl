#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <algorithm>
#include <Preferences.h>
#include <NimBLEServer.h>
#include "AsyncJson.h"

#pragma pack(push, 1)
struct EspNowMessage
{
    enum class Type : uint8_t
    {
        ToggleRed,
        ToggleGreen,
        ToggleBlue,
        ToggleWhite,
        ToggleAll,
        TurnOffAll,
        TurnOnAll,
        IncreaseBrightness,
        DecreaseBrightness,
    };

    Type type;
};

struct EspNowDevice
{
    static constexpr auto NAME_MAX_LENGTH = 23;
    static constexpr auto NAME_TOTAL_LENGTH = 24;
    static constexpr uint8_t MAC_SIZE = 6;

    std::array<char, NAME_TOTAL_LENGTH> name;
    std::array<uint8_t, MAC_SIZE> address;

    bool operator==(const EspNowDevice& other) const
    {
        return name == other.name && address == other.address;
    }

    bool operator!=(const EspNowDevice& other) const
    {
        return name != other.name && address != other.address;
    }
};

struct EspNowDeviceData
{
    static constexpr uint8_t MAX_DEVICES_PER_MESSAGE = 10;

    uint8_t deviceCount = 0;
    std::array<EspNowDevice, MAX_DEVICES_PER_MESSAGE> devices = {};

    bool operator==(const EspNowDeviceData& other) const
    {
        return deviceCount == other.deviceCount && devices == other.devices;
    }

    bool operator!=(const EspNowDeviceData& other) const
    {
        return deviceCount != other.deviceCount || devices != other.devices;
    }
};

static_assert(sizeof(EspNowDevice) == EspNowDevice::NAME_TOTAL_LENGTH + EspNowDevice::MAC_SIZE,
              "Unexpected EspNowDevice size");
#pragma pack(pop)

class EspNowHandler final : public BleInterfaceable, public StateJsonFiller
{
    static constexpr auto BLE_ESP_NOW_SERVICE = "12345678-1234-1234-1234-1234567890af";
    static constexpr auto BLE_ESP_NOW_DEVICES_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";

    static constexpr auto LOG_TAG = "EspNowHandler";

    static constexpr auto PREFERENCES_NAME = "esp-now";
    static constexpr auto PREFERENCES_COUNT_KEY = "devCount";
    static constexpr auto PREFERENCES_DATA_KEY = "devData";

    EspNowDeviceData deviceData = {};

public:
    void begin()
    {
        restoreDevices();
    }

    [[nodiscard]] EspNowDeviceData getDeviceData() const
    {
        std::lock_guard lock(getMutex());
        return deviceData;
    }

    void setDeviceData(const EspNowDeviceData& data)
    {
        std::lock_guard lock(getMutex());
        deviceData = data;
        persistDevices();
    }

    bool isMacAllowed(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        for (const auto& [name, address] : deviceData.devices)
        {
            if (std::equal(address.begin(), address.end(), mac))
                return true;
        }
        return false;
    }

    std::optional<EspNowDevice> findDeviceByMac(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        for (const auto& device : deviceData.devices)
        {
            if (std::equal(device.address.begin(), device.address.end(), mac))
                return device;
        }
        return std::nullopt;
    }

    std::optional<EspNowDevice> findDeviceByName(const std::string_view name) const
    {
        std::lock_guard lock(getMutex());
        for (const auto& device : deviceData.devices)
        {
            const auto& devName = device.name;
            if (std::string_view(devName.data(), strnlen(devName.data(), devName.size())) == name)
                return device;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<uint8_t> getDevicesBuffer()
    {
        std::lock_guard lock(getMutex());
        std::vector<uint8_t> buffer;
        buffer.reserve(1 + deviceData.deviceCount * sizeof(EspNowDevice));
        buffer.push_back(deviceData.deviceCount);
        for (const auto& [name, address] : deviceData.devices)
        {
            buffer.insert(buffer.end(), name.begin(), name.end());
            buffer.insert(buffer.end(), address.begin(), address.end());
        }
        return buffer;
    }

    void setDevicesBuffer(const uint8_t* data, const size_t length)
    {
        if (!data || length < 1) return;

        const uint8_t count = data[0];
        if (const size_t expectedSize = 1 + count * sizeof(EspNowDevice);
            length < expectedSize)
            return;

        EspNowDeviceData newData = {};
        newData.deviceCount = std::min(count, EspNowDeviceData::MAX_DEVICES_PER_MESSAGE);

        for (uint8_t i = 0; i < newData.deviceCount; ++i)
        {
            const size_t offset = 1 + i * sizeof(EspNowDevice);
            std::copy_n(&data[offset], EspNowDevice::NAME_TOTAL_LENGTH, newData.devices[i].name.begin());
            std::copy_n(&data[offset + EspNowDevice::NAME_TOTAL_LENGTH], EspNowDevice::MAC_SIZE,
                        newData.devices[i].address.begin());
        }

        setDeviceData(newData);
    }

    void createServiceAndCharacteristics(NimBLEServer* server) override
    {
        const auto service = server->createService(BLE_ESP_NOW_SERVICE);
        service->createCharacteristic(
            BLE_ESP_NOW_DEVICES_CHARACTERISTIC,
            READ | WRITE
        )->setCallbacks(new EspNowDevicesCallback(this));
        service->start();
    }

    void fillState(const JsonObject& root) const override
    {
        const auto& espNow = root["espNow"].to<JsonObject>();
        const auto arr = espNow["devices"].to<JsonArray>();
        std::lock_guard lock(getMutex());
        for (const auto& [name, mac] : deviceData.devices)
        {
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            macStr[sizeof(macStr) - 1] = '\0';
            const auto& obj = arr.add<JsonObject>();
            obj["name"] = name.data();
            obj["address"] = macStr;
        }
    }

private:
    static std::mutex& getMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    void persistDevices() const
    {
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, false))
        {
            const auto dataSize = deviceData.deviceCount * sizeof(EspNowDevice);
            prefs.putUInt(PREFERENCES_COUNT_KEY, deviceData.deviceCount);
            prefs.putBytes(PREFERENCES_DATA_KEY, deviceData.devices.data(), dataSize);
            prefs.end();
            ESP_LOGI(LOG_TAG, "Devices saved to Preferences");
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to open Preferences for saving");
        }
    }

    void restoreDevices()
    {
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, true))
        {
            deviceData.deviceCount = prefs.getUInt(PREFERENCES_COUNT_KEY, 0);
            if (const auto dataSize = deviceData.deviceCount * sizeof(EspNowDevice);
                prefs.getBytesLength(PREFERENCES_DATA_KEY) == dataSize)
            {
                deviceData.devices = {};
                prefs.getBytes(PREFERENCES_DATA_KEY, deviceData.devices.data(), dataSize);
                ESP_LOGI(LOG_TAG, "Devices restored from Preferences");
            }
            prefs.end();
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to open Preferences for reading");
        }
    }

    class EspNowDevicesCallback final : public NimBLECharacteristicCallbacks
    {
        EspNowHandler* espNowHandler;

    public:
        explicit EspNowDevicesCallback(EspNowHandler* espNowHandler)
            : espNowHandler(espNowHandler)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto value = pCharacteristic->getValue();
            espNowHandler->setDevicesBuffer(value.data(), value.size());
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto value = espNowHandler->getDevicesBuffer();
            pCharacteristic->setValue(value.data(), value.size());
        }
    };
};
