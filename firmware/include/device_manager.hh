#pragma once

#include "NimBLEServer.h"
#include "NimBLEService.h"
#include "NimBLECharacteristic.h"
#include "throttled_value.hh"
#include "version.hh"

class DeviceManager
{
public:
    static constexpr auto DEVICE_BASE_NAME = "rgbw-ctrl-";
    static constexpr auto DEVICE_NAME_MAX_LENGTH = 28;
    static constexpr auto DEVICE_NAME_TOTAL_LENGTH = DEVICE_NAME_MAX_LENGTH + 1;

private:
    static constexpr auto LOG_TAG = "DeviceManager";
    static constexpr auto PREFERENCES_NAME = "device-config";

    NimBLECharacteristic* deviceNameCharacteristic = nullptr;
    NimBLECharacteristic* deviceHeapCharacteristic = nullptr;

    mutable std::array<char, DEVICE_NAME_TOTAL_LENGTH> deviceName = {};
    ThrottledValue<uint32_t> heapNotificationThrottle{500};

public:
    void begin() // NOLINT
    {
        WiFi.mode(WIFI_STA); // NOLINT
    }

    void handle(const unsigned long now)
    {
        sendHeapNotification(now);
    }

    char* getDeviceName() const
    {
        std::lock_guard lock(getDeviceNameMutex());
        if (deviceName[0] == '\0')
        {
            loadDeviceName(deviceName.data());
        }
        return deviceName.data();
    }

    std::array<char, DEVICE_NAME_TOTAL_LENGTH> getDeviceNameArray() const
    {
        std::lock_guard lock(getDeviceNameMutex());
        if (deviceName[0] == '\0')
        {
            loadDeviceName(deviceName.data());
        }
        return deviceName;
    }

    void setDeviceName(const char* name)
    {
        if (!name || name[0] == '\0') return;

        std::lock_guard lock(getDeviceNameMutex());

        char safeName[DEVICE_NAME_TOTAL_LENGTH];
        std::strncpy(safeName, name, DEVICE_NAME_MAX_LENGTH);
        safeName[DEVICE_NAME_MAX_LENGTH] = '\0';

        if (std::strncmp(deviceName.data(), safeName, DEVICE_NAME_MAX_LENGTH) == 0)
            return;

        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);
        prefs.putString("deviceName", safeName);
        prefs.end();

        deviceName[0] = '\0'; // Invalidate cached name
        WiFiClass::setHostname(safeName);
        WiFi.reconnect();

        if (!deviceHeapCharacteristic) return;
        const auto len = std::min(strlen(safeName), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
        deviceNameCharacteristic->setValue(reinterpret_cast<uint8_t*>(safeName), len);
        deviceNameCharacteristic->notify(); // NOLINT
    }

    void createServiceAndCharacteristics(
        NimBLEServer* server,
        const char* deviceDetailsServiceUUID,
        const char* deviceRestartCharacteristicUUID,
        const char* deviceNameCharacteristicUUID,
        const char* firmwareVersionCharacteristicUUID,
        const char* deviceHeapCharacteristicUUID
    )
    {
        const auto service = server->createService(deviceDetailsServiceUUID);

        service->createCharacteristic(
            deviceRestartCharacteristicUUID,
            WRITE
        )->setCallbacks(new RestartCallback());

        deviceNameCharacteristic = service->createCharacteristic(
            deviceNameCharacteristicUUID,
            WRITE | READ | NOTIFY
        );
        deviceNameCharacteristic->setCallbacks(new DeviceNameCallback(this));

        service->createCharacteristic(
            firmwareVersionCharacteristicUUID,
            READ
        )->setCallbacks(new FirmwareVersionCallback());

        deviceHeapCharacteristic = service->createCharacteristic(
            deviceHeapCharacteristicUUID,
            NOTIFY
        );

        service->start();
    }

private:
    static const char* loadDeviceName(char* deviceName)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, true);
        if (prefs.isKey("deviceName"))
        {
            prefs.getString("deviceName", deviceName, DEVICE_NAME_TOTAL_LENGTH);
            prefs.end();
            return deviceName;
        }
        prefs.end();
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(deviceName, DEVICE_NAME_TOTAL_LENGTH,
                 "%s%02X%02X%02X", DEVICE_BASE_NAME, mac[3], mac[4], mac[5]);
        return deviceName;
    }

    static std::mutex& getDeviceNameMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    void sendHeapNotification(const unsigned long now)
    {
        if (!deviceHeapCharacteristic) return;

        auto state = ESP.getFreeHeap();
        if (!heapNotificationThrottle.shouldSend(now, state))
            return;
        deviceHeapCharacteristic->setValue(reinterpret_cast<uint8_t*>(&state), sizeof(state));
        if (deviceHeapCharacteristic->notify())
        {
            heapNotificationThrottle.setLastSent(now, state);
        }
    }

    class RestartCallback final : public NimBLECharacteristicCallbacks
    {
    public:
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            if (pCharacteristic->getValue() == "RESTART_NOW")
            {
                ESP_LOGW(LOG_TAG, "Device restart requested via BLE.");
                async_call([this]
                {
                    esp_restart();
                }, 1024, 50);
            }
            else
            {
                ESP_LOGW(LOG_TAG, "Device restart ignored: invalid value received.");
            }
        }
    };

    class DeviceNameCallback final : public NimBLECharacteristicCallbacks
    {
        DeviceManager* deviceManager;

    public:
        explicit DeviceNameCallback(DeviceManager* deviceManager) : deviceManager(deviceManager)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto name = deviceManager->getDeviceName();
            const auto len = std::min(strlen(name), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(const_cast<char*>(name)), len);
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto value = pCharacteristic->getValue();
            const auto data = value.data();
            const auto deviceName = reinterpret_cast<const char*>(data);

            if (const auto length = pCharacteristic->getLength(); length == 0 || length > DEVICE_NAME_MAX_LENGTH)
            {
                ESP_LOGE(LOG_TAG, "Invalid device name length: %d", static_cast<int>(length));
                return;
            }

            deviceManager->setDeviceName(deviceName);
        }
    };

    class FirmwareVersionCallback final : public NimBLECharacteristicCallbacks
    {
    public:
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            pCharacteristic->setValue(FIRMWARE_VERSION);
        }
    };
};
