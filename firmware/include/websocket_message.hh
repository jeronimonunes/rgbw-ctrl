#pragma once

#include <array>
#include <cstring>

#pragma pack(push, 1)
enum class WebSocketMessageType : uint8_t
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

struct WebSocketMessage
{
    WebSocketMessageType type;

    explicit WebSocketMessage(const WebSocketMessageType type) : type(type)
    {
    }
};

struct WebSocketColorMessage : WebSocketMessage
{
    Output::State state;

    explicit WebSocketColorMessage(const Output::State& state)
        : WebSocketMessage(WebSocketMessageType::ON_COLOR), state(state)
    {
    }
};

struct WebSocketBleStatusMessage : WebSocketMessage
{
    BleStatus status;

    explicit WebSocketBleStatusMessage(const BleStatus& status)
        : WebSocketMessage(WebSocketMessageType::ON_BLE_STATUS), status(status)
    {
    }
};

struct WebSocketDeviceNameMessage : WebSocketMessage
{
    std::array<char, DEVICE_NAME_TOTAL_LENGTH> deviceName = {};

    explicit WebSocketDeviceNameMessage(const std::array<char, DEVICE_NAME_TOTAL_LENGTH>& deviceName)
        : WebSocketMessage(WebSocketMessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName.data(), DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
    }

    explicit WebSocketDeviceNameMessage(const char* deviceName)
        : WebSocketMessage(WebSocketMessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName, DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
    }
};

struct WebSocketHttpCredentialsMessage : WebSocketMessage
{
    HttpCredentials credentials;

    explicit WebSocketHttpCredentialsMessage(const HttpCredentials& credentials)
        : WebSocketMessage(WebSocketMessageType::ON_HTTP_CREDENTIALS), credentials(credentials)
    {
    }
};

struct WebSocketWiFiConnectionDetailsMessage : WebSocketMessage
{
    WiFiConnectionDetails details;

    explicit WebSocketWiFiConnectionDetailsMessage(const WiFiConnectionDetails& details)
        : WebSocketMessage(WebSocketMessageType::ON_WIFI_CONNECTION_DETAILS), details(details)
    {
    }
};

struct WebSocketWiFiDetailsMessage : WebSocketMessage
{
    WiFiDetails details;

    explicit WebSocketWiFiDetailsMessage(const WiFiDetails& details)
        : WebSocketMessage(WebSocketMessageType::ON_WIFI_DETAILS), details(details)
    {
    }
};

struct WebSocketWiFiStatusMessage : WebSocketMessage
{
    WiFiStatus status;

    explicit WebSocketWiFiStatusMessage(const WiFiStatus& status)
        : WebSocketMessage(WebSocketMessageType::ON_WIFI_STATUS), status(status)
    {
    }
};

struct WebSocketAlexaIntegrationSettingsMessage : WebSocketMessage
{
    AlexaIntegrationSettings settings;

    explicit WebSocketAlexaIntegrationSettingsMessage(const AlexaIntegrationSettings& settings)
        : WebSocketMessage(WebSocketMessageType::ON_ALEXA_INTEGRATION_SETTINGS), settings(settings)
    {
    }
};

struct WebSocketOtaProgressMessage : WebSocketMessage
{
    OtaState otaState;

    explicit WebSocketOtaProgressMessage(const OtaState& otaState)
        : WebSocketMessage(WebSocketMessageType::ON_OTA_PROGRESS),
          otaState(otaState)
    {
    }
};

struct WebSocketHeapMessage : WebSocketMessage
{
    uint32_t freeHeap;

    explicit WebSocketHeapMessage(const uint32_t freeHeap)
        : WebSocketMessage(WebSocketMessageType::ON_HEAP), freeHeap(freeHeap)
    {
    }
};

struct WebSocketEspNowDevicesMessage : WebSocketMessage
{
    EspNowDeviceData data;

    explicit WebSocketEspNowDevicesMessage(const EspNowDeviceData& data)
        : WebSocketMessage(WebSocketMessageType::ON_ESP_NOW_DEVICES), data(data)
    {
    }
};

struct WebSocketFirmwareVersionMessage : WebSocketMessage
{
    std::array<char, 10> version = {};

    explicit WebSocketFirmwareVersionMessage(const char* ver)
        : WebSocketMessage(WebSocketMessageType::ON_FIRMWARE_VERSION)
    {
        std::strncpy(version.data(), ver, version.size() - 1);
        version[version.size() - 1] = '\0';
    }
};


#pragma pack(pop)
