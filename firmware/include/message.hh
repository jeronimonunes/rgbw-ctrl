#pragma once

#include <array>

enum class MessageType : uint8_t
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

#pragma pack(push, 1)
struct Message
{
    MessageType type;

    explicit Message(const MessageType type) : type(type)
    {
    }
};

struct ColorMessage : Message
{
    Output::State state;

    explicit ColorMessage(const Output::State& state)
        : Message(MessageType::ON_COLOR), state(state)
    {
    }
};

struct BleStatusMessage : Message
{
    BleStatus status;

    explicit BleStatusMessage(const BleStatus& status)
        : Message(MessageType::ON_BLE_STATUS), status(status)
    {
    }
};

struct DeviceNameMessage : Message
{
    std::array<char, DEVICE_NAME_TOTAL_LENGTH> deviceName = {};

    explicit DeviceNameMessage(const std::array<char, DEVICE_NAME_TOTAL_LENGTH>& deviceName)
        : Message(MessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName.data(), DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
    }

    explicit DeviceNameMessage(const char* deviceName)
        : Message(MessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName.data(), deviceName, DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
    }
};

struct HttpCredentialsMessage : Message
{
    HttpCredentials credentials;

    explicit HttpCredentialsMessage(const HttpCredentials& credentials)
        : Message(MessageType::ON_HTTP_CREDENTIALS), credentials(credentials)
    {
    }
};

struct WiFiConnectionDetailsMessage : Message
{
    WiFiConnectionDetails details;

    explicit WiFiConnectionDetailsMessage(const WiFiConnectionDetails& details)
        : Message(MessageType::ON_WIFI_CONNECTION_DETAILS), details(details)
    {
    }
};

struct WiFiDetailsMessage : Message
{
    WiFiDetails details;

    explicit WiFiDetailsMessage(const WiFiDetails& details)
        : Message(MessageType::ON_WIFI_DETAILS), details(details)
    {
    }
};

struct WiFiStatusMessage : Message
{
    WiFiStatus status;

    explicit WiFiStatusMessage(const WiFiStatus& status)
        : Message(MessageType::ON_WIFI_STATUS), status(status)
    {
    }
};

struct AlexaSettingsMessage : Message
{
    AlexaIntegrationSettings settings;

    explicit AlexaSettingsMessage(const AlexaIntegrationSettings& settings)
        : Message(MessageType::ON_ALEXA_INTEGRATION_SETTINGS), settings(settings)
    {
    }
};

struct AlexaIntegrationSettingsMessage : Message
{
    AlexaIntegrationSettings settings;

    explicit AlexaIntegrationSettingsMessage(const AlexaIntegrationSettings& settings)
        : Message(MessageType::ON_ALEXA_INTEGRATION_SETTINGS), settings(settings)
    {
    }
};

struct OtaProgressMessage : Message
{
    OtaState otaState;

    explicit OtaProgressMessage(const OtaState& otaState)
        : Message(MessageType::ON_OTA_PROGRESS),
          otaState(otaState)
    {
    }
};

struct HeapMessage : Message
{
    uint32_t freeHeap;

    explicit HeapMessage(const uint32_t freeHeap)
        : Message(MessageType::ON_HEAP), freeHeap(freeHeap)
    {
    }
};

struct EspNowDevicesMessage : Message
{
    EspNowDeviceData data;

    explicit EspNowDevicesMessage(const EspNowDeviceData& data)
        : Message(MessageType::ON_ESP_NOW_DEVICES), data(data)
    {
    }
};

struct FirmwareVersionMessage : Message
{
    std::array<char, 6> version;

    explicit FirmwareVersionMessage(const std::array<char, 6>& version)
        : Message(MessageType::ON_FIRMWARE_VERSION), version(version)
    {
    }
};

#pragma pack(pop)
