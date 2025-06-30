#pragma once

#include <array>
#include <cstring>
#include "device_manager.hh"

#pragma pack(push, 1)
struct WebSocketMessage
{
    enum class Type : uint8_t
    {
        ON_HEAP,
        ON_DEVICE_NAME,
        ON_FIRMWARE_VERSION,
        ON_COLOR,
        ON_HTTP_CREDENTIALS,
        ON_BLE_STATUS,
        ON_WIFI_STATUS,
        ON_WIFI_SCAN_STATUS,
        ON_WIFI_DETAILS,
        ON_WIFI_CONNECTION_DETAILS,
        ON_OTA_PROGRESS,
        ON_ALEXA_INTEGRATION_SETTINGS,
        ON_ESP_NOW_DEVICES,
    };

    Type type;

    explicit WebSocketMessage(const Type type) : type(type)
    {
    }
};

struct WebSocketColorMessage : WebSocketMessage
{
    Output::State state;

    explicit WebSocketColorMessage(const Output::State& state)
        : WebSocketMessage(Type::ON_COLOR), state(state)
    {
    }
};

struct WebSocketBleStatusMessage : WebSocketMessage
{
    BleStatus status;

    explicit WebSocketBleStatusMessage(const BleStatus& status)
        : WebSocketMessage(Type::ON_BLE_STATUS), status(status)
    {
    }
};

struct WebSocketDeviceNameMessage : WebSocketMessage
{
    std::array<char, DeviceManager::DEVICE_NAME_TOTAL_LENGTH> deviceName = {};

    explicit WebSocketDeviceNameMessage(const std::array<char, DeviceManager::DEVICE_NAME_TOTAL_LENGTH>& deviceName)
        : WebSocketMessage(Type::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName.data(), DeviceManager::DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DeviceManager::DEVICE_NAME_MAX_LENGTH] = '\0';
    }

    explicit WebSocketDeviceNameMessage(const char* deviceName)
        : WebSocketMessage(Type::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName, DeviceManager::DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DeviceManager::DEVICE_NAME_MAX_LENGTH] = '\0';
    }
};

struct WebSocketHttpCredentialsMessage : WebSocketMessage
{
    HttpCredentials credentials;

    explicit WebSocketHttpCredentialsMessage(const HttpCredentials& credentials)
        : WebSocketMessage(Type::ON_HTTP_CREDENTIALS), credentials(credentials)
    {
    }
};

struct WebSocketWiFiConnectionDetailsMessage : WebSocketMessage
{
    WiFiConnectionDetails details;

    explicit WebSocketWiFiConnectionDetailsMessage(const WiFiConnectionDetails& details)
        : WebSocketMessage(Type::ON_WIFI_CONNECTION_DETAILS), details(details)
    {
    }
};

struct WebSocketWiFiDetailsMessage : WebSocketMessage
{
    WiFiDetails details;

    explicit WebSocketWiFiDetailsMessage(const WiFiDetails& details)
        : WebSocketMessage(Type::ON_WIFI_DETAILS), details(details)
    {
    }
};

struct WebSocketWiFiStatusMessage : WebSocketMessage
{
    WiFiStatus status;

    explicit WebSocketWiFiStatusMessage(const WiFiStatus& status)
        : WebSocketMessage(Type::ON_WIFI_STATUS), status(status)
    {
    }
};

struct WebSocketAlexaIntegrationSettingsMessage : WebSocketMessage
{
    AlexaIntegration::Settings settings;

    explicit WebSocketAlexaIntegrationSettingsMessage(const AlexaIntegration::Settings& settings)
        : WebSocketMessage(Type::ON_ALEXA_INTEGRATION_SETTINGS), settings(settings)
    {
    }
};

struct WebSocketOtaProgressMessage : WebSocketMessage
{
    OtaState otaState;

    explicit WebSocketOtaProgressMessage(const OtaState& otaState)
        : WebSocketMessage(Type::ON_OTA_PROGRESS),
          otaState(otaState)
    {
    }
};

struct WebSocketHeapMessage : WebSocketMessage
{
    uint32_t freeHeap;

    explicit WebSocketHeapMessage(const uint32_t freeHeap)
        : WebSocketMessage(Type::ON_HEAP), freeHeap(freeHeap)
    {
    }
};

struct WebSocketEspNowDevicesMessage : WebSocketMessage
{
    EspNowDeviceData data;

    explicit WebSocketEspNowDevicesMessage(const EspNowDeviceData& data)
        : WebSocketMessage(Type::ON_ESP_NOW_DEVICES), data(data)
    {
    }
};

struct WebSocketFirmwareVersionMessage : WebSocketMessage
{
    std::array<char, 10> version;

    explicit WebSocketFirmwareVersionMessage(const std::array<char, 10>& version)
        : WebSocketMessage(Type::ON_FIRMWARE_VERSION), version(version)
    {
    }
};


#pragma pack(pop)
