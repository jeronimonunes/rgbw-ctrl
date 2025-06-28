#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <algorithm>
#include <esp_now.h>
#include <Preferences.h>

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

class EspNowHandler
{
    static constexpr auto LOG_TAG = "EspNowHandler";

    static constexpr auto PREFERENCES_NAME = "esp-now";
    static constexpr auto PREFERENCES_COUNT_KEY = "devCount";
    static constexpr auto PREFERENCES_DATA_KEY = "devData";

    static std::function<void(EspNowMessage* message)> callback;
    static EspNowDeviceData deviceData;

    static void onDataReceived(const uint8_t* mac, const uint8_t* incomingData, const int len)
    {
        ESP_LOGI(LOG_TAG, "Data received from %02X:%02X:%02X:%02X:%02X:%02X, length: %d",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);

        if (!isMacAllowed(mac))
        {
            ESP_LOGW(LOG_TAG, "MAC address not allowed, ignoring packet");
            return;
        }

        if (len != sizeof(EspNowMessage)) return;

        const auto message = reinterpret_cast<EspNowMessage*>(const_cast<uint8_t*>(incomingData));

        if (callback) callback(message);
    }

public:
    static void begin(std::function<void(EspNowMessage* message)> cb)
    {
        if (esp_now_init() != ESP_OK)
        {
            ESP_LOGE(LOG_TAG, "Failed to initialize ESP-NOW");
            return;
        }

        restoreDevices();
        esp_now_register_recv_cb(onDataReceived);
        callback = std::move(cb);
        ESP_LOGI(LOG_TAG, "ESP-NOW initialized and callback registered");
    }

    [[nodiscard]] static EspNowDeviceData getDeviceData()
    {
        std::lock_guard lock(getMutex());
        return deviceData;
    }

    static void setDeviceData(const EspNowDeviceData& data)
    {
        std::lock_guard lock(getMutex());
        deviceData = data;
        persistDevices();
    }

    static std::optional<EspNowDevice> findDeviceByMac(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        for (uint8_t i = 0; i < deviceData.deviceCount; ++i)
        {
            if (std::equal(deviceData.devices[i].address.begin(), deviceData.devices[i].address.end(), mac))
                return deviceData.devices[i];
        }
        return std::nullopt;
    }

    static std::optional<EspNowDevice> findDeviceByName(const std::string_view name)
    {
        std::lock_guard lock(getMutex());
        for (uint8_t i = 0; i < deviceData.deviceCount; ++i)
        {
            const auto& devName = deviceData.devices[i].name;
            if (std::string_view(devName.data(), strnlen(devName.data(), devName.size())) == name)
                return deviceData.devices[i];
        }
        return std::nullopt;
    }

    [[nodiscard]] static std::vector<uint8_t> getDevicesBuffer()
    {
        std::lock_guard lock(getMutex());
        std::vector<uint8_t> buffer;
        buffer.reserve(1 + deviceData.deviceCount * sizeof(EspNowDevice));
        buffer.push_back(deviceData.deviceCount);
        for (uint8_t i = 0; i < deviceData.deviceCount; ++i)
        {
            const auto& [name, address] = deviceData.devices[i];
            buffer.insert(buffer.end(), name.begin(), name.end());
            buffer.insert(buffer.end(), address.begin(), address.end());
        }
        return buffer;
    }

    static void setDevicesBuffer(const uint8_t* data, const size_t length)
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

    static void toJson(const JsonObject& espNow)
    {
        const auto arr = espNow["devices"].to<JsonArray>();
        std::lock_guard lock(getMutex());
        for (uint8_t i = 0; i < deviceData.deviceCount; ++i)
        {
            const auto& [name, mac] = deviceData.devices[i];
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

    static bool isMacAllowed(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        for (uint8_t i = 0; i < deviceData.deviceCount; ++i)
        {
            if (std::equal(deviceData.devices[i].address.begin(), deviceData.devices[i].address.end(), mac))
                return true;
        }
        return false;
    }

    static void persistDevices()
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

    static void restoreDevices()
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
};
