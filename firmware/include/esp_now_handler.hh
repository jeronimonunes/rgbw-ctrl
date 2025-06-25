#pragma once

#include <array>
#include <mutex>
#include <vector>
#include <algorithm>
#include <esp_now.h>
#include <Preferences.h>

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
#pragma pack(pop)

class EspNowHandler
{
    static constexpr auto LOG_TAG = "EspNowHandler";
    static constexpr auto PREFERENCES_NAME = "esp-now";
    static constexpr auto PREFERENCES_KEY = "allowedMacs";
    static constexpr uint8_t MAC_SIZE = 6;
    static constexpr uint8_t MAX_MACS_PER_MESSAGE = 85;

    static std::function<void(const uint8_t* mac, EspNowMessage* message)> callback;
    static std::vector<std::array<uint8_t, MAC_SIZE>> allowedMacs;

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

        if (callback)
            callback(mac, message);
    }

public:
    static void begin(std::function<void(const uint8_t* mac, EspNowMessage* message)> callback)
    {
        if (esp_now_init() != ESP_OK)
        {
            ESP_LOGE(LOG_TAG, "Failed to initialize ESP-NOW");
            return;
        }

        restoreAllowedMacs();
        esp_now_register_recv_cb(onDataReceived);
        EspNowHandler::callback = std::move(callback);
        ESP_LOGI(LOG_TAG, "ESP-NOW initialized and callback registered");
    }

    static std::vector<std::array<uint8_t, MAC_SIZE>> getAllowedMacs()
    {
        std::lock_guard lock(getMutex());
        return allowedMacs;
    }

    static void setAllowedMacs(std::vector<std::array<uint8_t, MAC_SIZE>>&& newAllowedMacs)
    {
        std::lock_guard lock(getMutex());
        allowedMacs = std::move(newAllowedMacs);
        persistAllowedMacs();
    }

    [[nodiscard]] static std::vector<uint8_t> getAllowedMacsMessage()
    {
        std::lock_guard lock(getMutex());
        std::vector<uint8_t> buffer;
        const uint8_t count = allowedMacs.size() > MAX_MACS_PER_MESSAGE
                                  ? MAX_MACS_PER_MESSAGE
                                  : static_cast<uint8_t>(allowedMacs.size());
        buffer.push_back(count);
        for (size_t i = 0; i < count; ++i)
        {
            buffer.insert(buffer.end(), allowedMacs[i].begin(), allowedMacs[i].end());
        }
        return buffer;
    }

    static void setAllowedMacsMessage(const uint8_t* data, const size_t length)
    {
        if (length < 1 || data == nullptr) return;

        const uint8_t count = data[0];
        std::vector<std::array<uint8_t, MAC_SIZE>> result;
        result.reserve(count);

        if (const size_t expectedSize = 1 + count * MAC_SIZE;
            length < expectedSize)
            return;

        for (uint8_t i = 0; i < count; ++i)
        {
            std::array<uint8_t, MAC_SIZE> mac; // NOLINT
            std::copy_n(&data[1 + i * MAC_SIZE], MAC_SIZE, mac.begin());
            result.push_back(mac);
        }

        setAllowedMacs(std::move(result));
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
        return std::any_of(allowedMacs.begin(), allowedMacs.end(), [mac](const auto& allowed)
        {
            return std::equal(allowed.begin(), allowed.end(), mac);
        });
    }

    static void persistAllowedMacs()
    {
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, false))
        {
            prefs.putBytes(PREFERENCES_KEY, allowedMacs.data(), allowedMacs.size() * MAC_SIZE);
            prefs.end();
            ESP_LOGI(LOG_TAG, "Allowed MACs saved to Preferences");
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to open Preferences for saving");
        }
    }

    static void restoreAllowedMacs()
    {
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, true))
        {
            if (const size_t len = prefs.getBytesLength(PREFERENCES_KEY);
                len > 0 && len % MAC_SIZE == 0)
            {
                std::lock_guard lock(getMutex());
                allowedMacs.resize(len / MAC_SIZE);
                prefs.getBytes(PREFERENCES_KEY, allowedMacs.data(), len);
                ESP_LOGI(LOG_TAG, "Allowed MACs restored from Preferences");
            }
            prefs.end();
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Failed to open Preferences for reading");
        }
    }
};
