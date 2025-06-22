#pragma once

enum class MessageType : uint8_t
{
    ON_COLOR,
    ON_HTTP_CREDENTIALS,
    ON_DEVICE_NAME,
    ON_HEAP,
    ON_BLE_STATUS,
    ON_WIFI_STATUS,
    ON_WIFI_SCAN_STATUS,
    ON_WIFI_DETAILS,
    ON_OTA_PROGRESS,
    ON_ALEXA_INTEGRATION_SETTINGS,
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
    char deviceName[DEVICE_NAME_TOTAL_LENGTH] = {};

    explicit DeviceNameMessage(const std::array<char, DEVICE_NAME_TOTAL_LENGTH>& deviceName)
        : Message(MessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName, deviceName.data(), DEVICE_NAME_MAX_LENGTH);
        this->deviceName[DEVICE_NAME_MAX_LENGTH] = '\0';
    }

    explicit DeviceNameMessage(const char* deviceName)
        : Message(MessageType::ON_DEVICE_NAME)
    {
        std::strncpy(this->deviceName, deviceName, DEVICE_NAME_MAX_LENGTH);
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
        : Message(MessageType::ON_WIFI_DETAILS), details(details)
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

#pragma pack(pop)
