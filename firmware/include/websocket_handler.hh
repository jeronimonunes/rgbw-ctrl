#pragma once

#include <array>
#include "message.hh"
#include "ble_manager.hh"
#include "throttled_value.hh"

class WebSocketHandler
{
    static constexpr auto LOG_TAG = "WebSocketHandler";

    Output& output;
    OtaHandler& otaHandler;
    WiFiManager& wifiManager;
    WebServerHandler& webServerHandler;
    AlexaIntegration& alexaIntegration;
    BleManager& bleManager;

    AsyncWebSocket ws = AsyncWebSocket("/ws");

    ThrottledValue<Output::State> outputThrottle{100};
    ThrottledValue<BleStatus> bleStatusThrottle{100};
    ThrottledValue<std::array<char, DEVICE_NAME_TOTAL_LENGTH>> deviceNameThrottle{100};
    ThrottledValue<OtaState> otaStateThrottle{100};
    ThrottledValue<uint32_t> heapInfoThrottle{500};

public:
    WebSocketHandler(
        Output& output,
        OtaHandler& otaHandler,
        WiFiManager& wifiManager,
        WebServerHandler& webServerHandler,
        AlexaIntegration& alexaIntegration,
        BleManager& bleManager
    )
        :
        output(output),
        otaHandler(otaHandler),
        wifiManager(wifiManager),
        webServerHandler(webServerHandler),
        alexaIntegration(alexaIntegration),
        bleManager(bleManager)
    {
        ws.onEvent([this](AsyncWebSocket*, AsyncWebSocketClient* client,
                          const AwsEventType type, void* arg, const uint8_t* data,
                          const size_t len)
        {
            this->handleWebSocketEvent(client, type, arg, data, len);
        });
    }

    void handle(const unsigned long now)
    {
        ws.cleanupClients();
        sendAllMessages(now);
    }

    AsyncWebHandler* getAsyncWebHandler()
    {
        return &ws;
    }

private:
    void handleWebSocketEvent(AsyncWebSocketClient* client,
                              const AwsEventType type, void* arg, const uint8_t* data,
                              const size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            ESP_LOGD(LOG_TAG, "WebSocket client connected: %s", client->remoteIP().toString().c_str());
            sendAllMessages(millis(), client);
            break;
        case WS_EVT_DISCONNECT: // NOLINT
            ESP_LOGD(LOG_TAG, "WebSocket client disconnected: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_PONG:
            ESP_LOGD(LOG_TAG, "WebSocket pong received from client: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_ERROR:
            ESP_LOGE(LOG_TAG, "WebSocket error: %s", client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DATA:
            this->handleWebSocketMessage(client, arg, data, len);
            break;
        default:
            break;
        }
    }

    void handleWebSocketMessage(
        AsyncWebSocketClient* client,
        void* arg,
        const uint8_t* data,
        const size_t len
    )
    {
        const auto info = static_cast<AwsFrameInfo*>(arg);
        if (info->opcode != WS_BINARY)
        {
            ESP_LOGD(LOG_TAG, "Received non-binary WebSocket message, opcode: %d", info->opcode);
            return;
        }
        if (!info->final)
        {
            ESP_LOGD(LOG_TAG, "Received fragmented WebSocket message, only final messages are processed");
            return;
        }
        if (info->index != 0)
        {
            ESP_LOGD(LOG_TAG, "Received fragmented WebSocket message with index %lld, only index 0 is processed",
                     info->index);
            return;
        }
        if (info->len != len)
        {
            ESP_LOGD(LOG_TAG, "Received WebSocket message with unexpected length: expected %lld, got %d", info->len,
                     len);
            return;
        }
        if (len < 1)
        {
            ESP_LOGD(LOG_TAG, "Received empty WebSocket message");
            return;
        }

        const uint8_t messageTypeRaw = data[0];
        if (messageTypeRaw > static_cast<uint8_t>(MessageType::ON_ALEXA_INTEGRATION_SETTINGS))
        {
            ESP_LOGD(LOG_TAG, "Received unknown WebSocket message type: %d", messageTypeRaw);
            return;
        }

        const auto messageType = static_cast<MessageType>(messageTypeRaw);
        ESP_LOGD(LOG_TAG, "Received WebSocket message of type %d", static_cast<int>(messageType));

        this->handleWebSocketMessage(messageType, client, data, len);
    }

    void handleWebSocketMessage(
        const MessageType messageType,
        AsyncWebSocketClient* client,
        const uint8_t* data,
        const size_t len
    )
    {
        switch (messageType)
        {
        case MessageType::ON_COLOR:
            handleColorMessage(data, len);
            break;

        case MessageType::ON_HTTP_CREDENTIALS:
            handleHttpCredentialsMessage(data, len);
            break;

        case MessageType::ON_DEVICE_NAME:
            handleDeviceNameMessage(data, len);
            break;

        case MessageType::ON_HEAP:
            handleHeapMessage();
            break;

        case MessageType::ON_BLE_STATUS:
            handleBleStatusMessage(client, data, len);
            break;

        case MessageType::ON_WIFI_STATUS:
            handleWiFiStatusMessage(data, len);
            break;

        case MessageType::ON_WIFI_SCAN_STATUS:
            handleWiFiScanStatusMessage();
            break;

        case MessageType::ON_WIFI_DETAILS:
            handleWiFiDetailsMessage();
            break;

        case MessageType::ON_OTA_PROGRESS:
            handleOtaProgressMessage();
            break;

        case MessageType::ON_ALEXA_INTEGRATION_SETTINGS:
            handleAlexaIntegrationSettingsMessage(data, len);
            break;

        default:
            client->text("Unknown message type");
            break;
        }
    }

    void handleColorMessage(const uint8_t* data, const size_t len)
    {
        if (len < sizeof(ColorMessage)) return;
        const auto* message = reinterpret_cast<const ColorMessage*>(data);
        outputThrottle.setLastSent(millis(), message->state);
        output.setState(message->state);
        alexaIntegration.updateDevices();
    }

    void handleHttpCredentialsMessage(const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(HttpCredentialsMessage)) return;
        const auto* message = reinterpret_cast<const HttpCredentialsMessage*>(data);
        webServerHandler.updateCredentials(message->credentials);
    }

    void handleDeviceNameMessage(const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(DeviceNameMessage)) return;
        const auto* message = reinterpret_cast<const DeviceNameMessage*>(data);
        wifiManager.setDeviceName(message->deviceName.data());
    }

    static void handleHeapMessage()
    {
        ESP_LOGD(LOG_TAG, "Received HEAP message (ignored).");
    }

    void handleBleStatusMessage(AsyncWebSocketClient* client, const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(BleStatusMessage)) return;
        switch (
            const auto* message = reinterpret_cast<const BleStatusMessage*>(data);
            message->status
        )
        {
        case BleStatus::ADVERTISING:
            async_call([this]
            {
                bleManager.start();
            }, 4096, 0);
            break;
        case BleStatus::OFF:
            async_call([client,this]
            {
                client->close();
                delay(100);
                bleManager.stop();
            }, 2048, 0);
            break;
        default:
            break;
        }
    }

    void handleWiFiStatusMessage(const uint8_t* data, const size_t len) const
    {
        if (len < sizeof(WiFiConnectionDetailsMessage)) return;
        const auto* message = reinterpret_cast<const WiFiConnectionDetailsMessage*>(data);
        wifiManager.connect(message->details);
    }

    void handleWiFiScanStatusMessage() const
    {
        wifiManager.triggerScan();
    }

    static void handleWiFiDetailsMessage()
    {
        ESP_LOGD(LOG_TAG, "Received WIFI_DETAILS message (ignored).");
    }

    static void handleOtaProgressMessage()
    {
        ESP_LOGD(LOG_TAG, "Received OTA_PROGRESS message (ignored).");
    }

    void handleAlexaIntegrationSettingsMessage(const uint8_t* data,
                                               const size_t len) const
    {
        if (len < sizeof(AlexaIntegrationSettingsMessage)) return;
        const auto* message = reinterpret_cast<const AlexaIntegrationSettingsMessage*>(data);
        alexaIntegration.applySettings(message->settings);
    }

    template <typename TState, typename TMessage, typename TThrottle>
    void sendThrottledMessage(const TState& state, TThrottle& throttle, const unsigned long now,
                              AsyncWebSocketClient* client = nullptr)
    {
        if (!throttle.shouldSend(now, state) && !client)
            return;

        const TMessage message(state);
        const auto data = reinterpret_cast<const uint8_t*>(&message);
        constexpr size_t len = sizeof(TMessage);

        if (client)
        {
            client->binary(data, len);
        }
        else if (AsyncWebSocket::SendStatus::ENQUEUED == ws.binaryAll(data, len))
        {
            throttle.setLastSent(now, state);
        }
    }

    void sendAllMessages(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        sendOutputColorMessage(now, client);
        sendBleStatusMessage(now, client);
        sendDeviceNameMessage(now, client);
        sendOtaProgressMessage(now, client);
        sendHeapInfoMessage(now, client);
    }

    void sendOutputColorMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        sendThrottledMessage<Output::State, ColorMessage>(
            output.getState(), outputThrottle, now, client);
    }

    void sendBleStatusMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        sendThrottledMessage<BleStatus, BleStatusMessage>(
            bleManager.getStatus(), bleStatusThrottle, now, client);
    }

    void sendDeviceNameMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        std::array<char, DEVICE_NAME_TOTAL_LENGTH> deviceName = {};
        strncpy(deviceName.data(), wifiManager.getDeviceName(), DEVICE_NAME_MAX_LENGTH);
        sendThrottledMessage<std::array<char, DEVICE_NAME_TOTAL_LENGTH>, DeviceNameMessage>(
            deviceName, deviceNameThrottle, now, client);
    }

    void sendOtaProgressMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        sendThrottledMessage<OtaState, OtaProgressMessage>(
            otaHandler.getState(), otaStateThrottle, now, client);
    }

    void sendHeapInfoMessage(const unsigned long now, AsyncWebSocketClient* client = nullptr)
    {
        const auto freeHeap = ESP.getFreeHeap();
        sendThrottledMessage<uint32_t, HeapMessage>(
            freeHeap, heapInfoThrottle, now, client);
    }
};
