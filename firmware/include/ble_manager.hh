#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <freertos/FreeRTOS.h>
#include <string>

#include "alexa_integration.hh"
#include "async_call.hh"
#include "throttled_value.hh"
#include "version.hh"
#include "wifi_manager.hh"
#include "webserver_handler.hh"

enum class BleStatus :uint8_t
{
    OFF,
    ADVERTISING,
    CONNECTED
};

class BleManager
{
    struct BLE_UUID
    {
        static constexpr auto DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ac";
        static constexpr auto DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
        static constexpr auto DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
        static constexpr auto FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
        static constexpr auto HTTP_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";
        static constexpr auto DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";

        static constexpr auto WIFI_SERVICE = "12345678-1234-1234-1234-1234567890ab";
        static constexpr auto WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";
        static constexpr auto WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";
        static constexpr auto WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";
        static constexpr auto WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";

        static constexpr auto ALEXA_SERVICE = "12345678-1234-1234-1234-1234567890ba";
        static constexpr auto ALEXA_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
        static constexpr auto ALEXA_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a";
    };

    static constexpr auto LOG_TAG = "BleManager";
    static constexpr auto BLE_TIMEOUT_MS = 30000;

    unsigned long bluetoothAdvertisementTimeout = 0;
    ThrottledValue<Output::State> colorNotificationThrottle{500};
    ThrottledValue<uint32_t> heapNotificationThrottle{500};

    Output& output;
    WiFiManager& wifiManager;
    AlexaIntegration& alexaIntegration;
    WebServerHandler& webServerHandler;

    NimBLEServer* server = nullptr;

    NimBLECharacteristic* deviceNameCharacteristic = nullptr;
    NimBLECharacteristic* deviceHeapCharacteristic = nullptr;

    NimBLECharacteristic* wifiDetailsCharacteristic = nullptr;
    NimBLECharacteristic* wifiStatusCharacteristic = nullptr;
    NimBLECharacteristic* wifiScanStatusCharacteristic = nullptr;
    NimBLECharacteristic* wifiScanResultCharacteristic = nullptr;

    NimBLECharacteristic* alexaColorCharacteristic = nullptr;

public:
    explicit BleManager(Output& output, WiFiManager& wifiManager, AlexaIntegration& alexaIntegration,
                        WebServerHandler& webServerHandler)
        : output(output), wifiManager(wifiManager), alexaIntegration(alexaIntegration),
          webServerHandler(webServerHandler)
    {
    }

    void start()
    {
        bluetoothAdvertisementTimeout = millis() + BLE_TIMEOUT_MS;
        if (server != nullptr) return;

        initiateServicesAndCharacteristics();
        attachWiFiManagerCallbacks();
        startAdvertising();
    }

    void handle(const unsigned long now)
    {
        handleAdvertisementTimeout(now);
        if (server == nullptr || server->getConnectedCount() == 0)
            return;
        sendColorNotification(now);
        sendHeapNotification(now);
    }

    void stop() const
    {
        if (server == nullptr) return;
        server->getAdvertising()->stop();
        if (this->server->getConnectedCount() > 0)
        {
            server->disconnect(0); // NOLINT
            delay(100); // Allow client disconnect to propagate
        }
        esp_restart();
    }

    [[nodiscard]] BleStatus getStatus() const
    {
        if (this->server == nullptr)
            return BleStatus::OFF;
        if (this->server->getConnectedCount() > 0)
            return BleStatus::CONNECTED;
        return BleStatus::ADVERTISING;
    }

    [[nodiscard]] const char* getStatusString() const
    {
        switch (getStatus())
        {
        case BleStatus::OFF:
            return "OFF";
        case BleStatus::ADVERTISING:
            return "ADVERTISING";
        case BleStatus::CONNECTED:
            return "CONNECTED";
        default:
            return "UNKNOWN";
        }
    }

    void toJson(const JsonObject& to) const
    {
        to["status"] = getStatusString();
    }

private:
    void startAdvertising()
    {
        const auto advertising = this->server->getAdvertising();
        advertising->setName(wifiManager.getDeviceName());
        advertising->start();
        bluetoothAdvertisementTimeout = millis() + BLE_TIMEOUT_MS;
        ESP_LOGI(LOG_TAG, "BLE advertising started with device name: %s", wifiManager.getDeviceName());
    }

    void handleAdvertisementTimeout(const unsigned long now)
    {
        if (this->getStatus() == BleStatus::CONNECTED)
        {
            bluetoothAdvertisementTimeout = now + BLE_TIMEOUT_MS;
        }
        else if (now > bluetoothAdvertisementTimeout && this->server != nullptr)
        {
            ESP_LOGW(LOG_TAG, "No BLE client connected for %d ms, stopping BLE server.", BLE_TIMEOUT_MS);
            this->stop();
        }
    }

    void sendColorNotification(const unsigned long now)
    {
        Output::State state = output.getState();
        if (!colorNotificationThrottle.shouldSend(now, state))
            return;
        alexaColorCharacteristic->setValue(reinterpret_cast<uint8_t*>(&state), sizeof(state));
        if (alexaColorCharacteristic->notify())
        {
            colorNotificationThrottle.setLastSent(now, state);
        }
    }

    void sendHeapNotification(const unsigned long now)
    {
        auto state = ESP.getFreeHeap();
        if (!heapNotificationThrottle.shouldSend(now, state))
            return;
        deviceHeapCharacteristic->setValue(reinterpret_cast<uint8_t*>(&state), sizeof(state));
        if (deviceHeapCharacteristic->notify())
        {
            heapNotificationThrottle.setLastSent(now, state);
        }
    }

    void initiateServicesAndCharacteristics()
    {
        BLEDevice::init(wifiManager.getDeviceName());
        server = BLEDevice::createServer();
        server->setCallbacks(new BLEServerCallback(this));

        setupBleDeviceNameService();
        setupBleWiFiService();
        setupBleAlexaService();
    }

    void setupBleDeviceNameService()
    {
        const auto deviceDetailsService = server->createService(BLE_UUID::DEVICE_DETAILS_SERVICE);

        const auto restartCharacteristic = deviceDetailsService->createCharacteristic(
            BLE_UUID::DEVICE_RESTART_CHARACTERISTIC,
            WRITE
        );
        restartCharacteristic->setCallbacks(new RestartCallback(this));

        deviceNameCharacteristic = deviceDetailsService->createCharacteristic(
            BLE_UUID::DEVICE_NAME_CHARACTERISTIC,
            WRITE | READ | NOTIFY
        );
        deviceNameCharacteristic->setCallbacks(new DeviceNameCallback(this));

        const auto firmwareVersionCharacteristic = deviceDetailsService->createCharacteristic(
            BLE_UUID::FIRMWARE_VERSION_CHARACTERISTIC,
            READ
        );
        firmwareVersionCharacteristic->setCallbacks(new FirmwareVersionCallback());

        const auto httpCredentialsCharacteristic = deviceDetailsService->createCharacteristic(
            BLE_UUID::HTTP_CREDENTIALS_CHARACTERISTIC,
            READ | WRITE
        );
        httpCredentialsCharacteristic->setCallbacks(new HttpCredentialsCallback(this));

        deviceHeapCharacteristic = deviceDetailsService->createCharacteristic(
            BLE_UUID::DEVICE_HEAP_CHARACTERISTIC,
            NOTIFY
        );

        deviceDetailsService->start();
    }

    void setupBleWiFiService()
    {
        const auto bleWiFiService = server->createService(BLE_UUID::WIFI_SERVICE);

        wifiDetailsCharacteristic = bleWiFiService->createCharacteristic(
            BLE_UUID::WIFI_DETAILS_CHARACTERISTIC,
            READ | NOTIFY
        );
        wifiDetailsCharacteristic->setCallbacks(new WiFiDetailsCallback(this));

        wifiStatusCharacteristic = bleWiFiService->createCharacteristic(
            BLE_UUID::WIFI_STATUS_CHARACTERISTIC,
            WRITE | READ | NOTIFY
        );
        wifiStatusCharacteristic->setCallbacks(new WiFiStatusCallback(this));

        wifiScanStatusCharacteristic = bleWiFiService->createCharacteristic(
            BLE_UUID::WIFI_SCAN_STATUS_CHARACTERISTIC,
            WRITE | READ | NOTIFY
        );
        wifiScanStatusCharacteristic->setCallbacks(new WiFiScanStatusCallback(this));

        wifiScanResultCharacteristic = bleWiFiService->createCharacteristic(
            BLE_UUID::WIFI_SCAN_RESULT_CHARACTERISTIC,
            READ | NOTIFY
        );
        wifiScanResultCharacteristic->setCallbacks(new WiFiScanResultCallback(this));

        bleWiFiService->start();
    }

    void setupBleAlexaService()
    {
        const auto alexaService = server->createService(BLE_UUID::ALEXA_SERVICE);

        const auto alexaCharacteristic = alexaService->createCharacteristic(
            BLE_UUID::ALEXA_CHARACTERISTIC,
            READ | WRITE
        );
        alexaCharacteristic->setCallbacks(new AlexaCallback(this));

        alexaColorCharacteristic = alexaService->createCharacteristic(
            BLE_UUID::ALEXA_COLOR_CHARACTERISTIC,
            READ | WRITE | NOTIFY
        );
        alexaColorCharacteristic->setCallbacks(new AlexaColorCallback(this));

        alexaService->start();
    }

    void attachWiFiManagerCallbacks() const
    {
        wifiManager.setDetailsChangedCallback([this](WiFiDetails wiFiScanResult)
        {
            wifiDetailsCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResult), sizeof(wiFiScanResult));
            wifiDetailsCharacteristic->notify(); // NOLINT
        });

        wifiManager.setScanResultChangedCallback([this](WiFiScanResult wiFiScanResult)
        {
            wifiScanResultCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResult), sizeof(wiFiScanResult));
            wifiScanResultCharacteristic->notify(); // NOLINT
        });

        wifiManager.setScanStatusChangedCallback([this](WifiScanStatus wifiScanStatus)
        {
            wifiScanStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wifiScanStatus), sizeof(wifiScanStatus));
            wifiScanStatusCharacteristic->notify(); // NOLINT
        });

        wifiManager.setStatusChangedCallback([this](WiFiStatus wiFiScanResultStatus)
        {
            wifiStatusCharacteristic->setValue(reinterpret_cast<uint8_t*>(&wiFiScanResultStatus),
                                               sizeof(wiFiScanResultStatus));
            wifiStatusCharacteristic->notify(); // NOLINT
        });

        wifiManager.setDeviceNameChangedCallback([this](char* deviceName)
        {
            const auto len = std::min(strlen(deviceName), static_cast<size_t>(DEVICE_NAME_MAX_LENGTH));
            deviceNameCharacteristic->setValue(reinterpret_cast<uint8_t*>(deviceName), len);
            deviceNameCharacteristic->notify(); // NOLINT
        });
    }

    // -------------------- BLE CALLBACKS --------------------

    class RestartCallback final : public NimBLECharacteristicCallbacks
    {
    public:
        BleManager* net;

        explicit RestartCallback(BleManager* n) : net(n)
        {
        }

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
        BleManager* net;

    public:
        explicit DeviceNameCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            const auto name = net->wifiManager.getDeviceName();
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

            net->wifiManager.setDeviceName(deviceName);
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

    class HttpCredentialsCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit HttpCredentialsCallback(BleManager* n) : net(n)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            HttpCredentials credentials;
            if (pCharacteristic->getValue().size() != sizeof(HttpCredentials))
            {
                ESP_LOGE(LOG_TAG, "Received invalid OTA credentials length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(&credentials, pCharacteristic->getValue().data(), sizeof(HttpCredentials));
            net->webServerHandler.updateCredentials(credentials);
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            HttpCredentials credentials = WebServerHandler::getCredentials();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&credentials), sizeof(credentials));
        }
    };

    class WiFiDetailsCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiDetailsCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiDetails details = WiFiDetails::fromWiFi();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&details), sizeof(details));
        }
    };

    class WiFiStatusCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiStatus status = net->wifiManager.getStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&status), sizeof(status));
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiConnectionDetails details = {};
            if (pCharacteristic->getValue().size() != sizeof(WiFiConnectionDetails))
            {
                ESP_LOGE(LOG_TAG, "Received invalid WiFi connection details length: %d",
                         pCharacteristic->getValue().size());
                return;
            }
            memcpy(&details, pCharacteristic->getValue().data(), sizeof(WiFiConnectionDetails));
            net->wifiManager.connect(details);
        }
    };

    class WiFiScanStatusCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanStatusCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WifiScanStatus scanStatus = net->wifiManager.getScanStatus();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanStatus), sizeof(scanStatus));
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            net->wifiManager.triggerScan();
        }
    };

    class WiFiScanResultCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit WiFiScanResultCallback(BleManager* n) : net(n)
        {
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            WiFiScanResult scanResult = net->wifiManager.getScanResult();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&scanResult), sizeof(scanResult));
        }
    };

    class AlexaCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaCallback(BleManager* net): net(net)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            AlexaIntegrationSettings settings;
            if (pCharacteristic->getValue().size() != sizeof(AlexaIntegrationSettings))
            {
                ESP_LOGE(LOG_TAG, "Received invalid Alexa settings length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(&settings, pCharacteristic->getValue().data(), sizeof(AlexaIntegrationSettings));
            net->alexaIntegration.applySettings(settings);
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            auto settings = net->alexaIntegration.getSettings();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&settings), sizeof(AlexaIntegrationSettings));
        }
    };

    class AlexaColorCallback final : public NimBLECharacteristicCallbacks
    {
        BleManager* net;

    public:
        explicit AlexaColorCallback(BleManager* net): net(net)
        {
        }

        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            Output::State state = {};
            constexpr uint8_t size = sizeof(Output::State);
            if (pCharacteristic->getValue().size() != size)
            {
                ESP_LOGE(LOG_TAG, "Received invalid Alexa color values length: %d", pCharacteristic->getValue().size());
                return;
            }
            memcpy(&state, pCharacteristic->getValue().data(), size);
            net->output.setState(state);
            net->alexaIntegration.updateDevices();
            net->colorNotificationThrottle.setLastSent(millis(), state);
        }

        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override
        {
            auto state = net->output.getState();
            pCharacteristic->setValue(reinterpret_cast<uint8_t*>(&state), sizeof(state));
        }
    };

    class BLEServerCallback final : public NimBLEServerCallbacks
    {
        BleManager* net;

    public:
        explicit BLEServerCallback(BleManager* net): net(net)
        {
        }

        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override
        {
            pServer->getAdvertising()->start();
        }
    };
};
