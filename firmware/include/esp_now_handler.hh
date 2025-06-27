#pragma once

#include <array>
#include <mutex>
#include <vector>
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
};

static_assert(sizeof(EspNowDevice) == EspNowDevice::NAME_TOTAL_LENGTH + EspNowDevice::MAC_SIZE,
              "Unexpected EspNowDevice size");
#pragma pack(pop)

class EspNowHandler
{
    static constexpr auto LOG_TAG = "EspNowHandler";

    static constexpr auto PREFERENCES_NAME = "esp-now";
    static constexpr auto PREFERENCES_KEY = "devices";
    static constexpr uint8_t MAX_DEVICES_PER_MESSAGE = 15;

    static std::function<void(EspNowMessage* message)> callback;
    static std::vector<EspNowDevice> devices;

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

    [[nodiscard]] static std::vector<EspNowDevice> getDevices()
    {
        std::lock_guard lock(getMutex());
        return devices;
    }

    static void setDevices(std::vector<EspNowDevice>&& newDevices)
    {
        std::lock_guard lock(getMutex());
        devices = std::move(newDevices);
        persistDevices();
    }

    static std::optional<EspNowDevice> findDeviceByMac(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        const auto it = std::find_if(devices.begin(), devices.end(), [mac](const auto& device)
        {
            return std::equal(device.address.begin(), device.address.end(), mac);
        });
        if (it != devices.end()) return *it;
        return std::nullopt;
    }

    static std::optional<EspNowDevice> findDeviceByName(std::string_view name)
    {
        std::lock_guard lock(getMutex());
        const auto it = std::find_if(devices.begin(), devices.end(), [name](const auto& device)
        {
            return std::string_view(device.name.data(), strnlen(device.name.data(), device.name.size())) == name;
        });
        if (it != devices.end()) return *it;
        return std::nullopt;
    }

    [[nodiscard]] static std::vector<uint8_t> getDevicesBuffer()
    {
        std::lock_guard lock(getMutex());
        std::vector<uint8_t> buffer;
        const uint8_t count = devices.size() > MAX_DEVICES_PER_MESSAGE
                                  ? MAX_DEVICES_PER_MESSAGE
                                  : static_cast<uint8_t>(devices.size());
        buffer.reserve(1 + count * sizeof(EspNowDevice));
        buffer.push_back(count);
        for (size_t i = 0; i < count; ++i)
        {
            const auto& [name, mac] = devices[i];
            buffer.insert(buffer.end(), name.begin(), name.end());
            buffer.insert(buffer.end(), mac.begin(), mac.end());
        }
        return buffer;
    }

    static void setDevicesBuffer(const uint8_t* data, const size_t length)
    {
        if (!data || length < 1) return;
        const uint8_t count = data[0];

        if (const size_t expectedSize = 1 + count * sizeof(EspNowDevice); length < expectedSize) return;

        std::vector<EspNowDevice> result;
        result.reserve(count);

        for (uint8_t i = 0; i < count; ++i)
        {
            EspNowDevice device; // NOLINT
            const size_t offset = 1 + i * sizeof(EspNowDevice);
            std::copy_n(&data[offset], EspNowDevice::NAME_TOTAL_LENGTH, device.name.begin());
            std::copy_n(&data[offset + EspNowDevice::NAME_TOTAL_LENGTH], EspNowDevice::MAC_SIZE, device.address.begin());
            result.push_back(device);
        }

        setDevices(std::move(result));
    }

    static void toJson(const JsonObject& espNow)
    {
        const auto arr = espNow["devices"].to<JsonArray>();
        const auto devices = getDevices(); // NOLINT
        for (const auto& [name, mac] : devices)
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

    static bool isMacAllowed(const uint8_t* mac)
    {
        std::lock_guard lock(getMutex());
        return std::any_of(devices.begin(), devices.end(), [mac](const auto& device)
        {
            return std::equal(device.address.begin(), device.address.end(), mac);
        });
    }

    static void persistDevices()
    {
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, false))
        {
            prefs.putBytes(PREFERENCES_KEY, devices.data(), devices.size() * sizeof(EspNowDevice));
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
            if (const size_t len = prefs.getBytesLength(PREFERENCES_KEY);
                len > 0 && len % sizeof(EspNowDevice) == 0)
            {
                std::lock_guard lock(getMutex());
                devices.resize(len / sizeof(EspNowDevice));
                prefs.getBytes(PREFERENCES_KEY, devices.data(), len);
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
