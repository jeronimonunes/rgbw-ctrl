#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <freertos/FreeRTOS.h>
#include <string>

#include "alexa_integration.hh"
#include "async_call.hh"
#include "device_manager.hh"
#include "esp_now_handler.hh"
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
        static constexpr auto DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ab";
        static constexpr auto DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
        static constexpr auto DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
        static constexpr auto FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
        static constexpr auto DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";

        static constexpr auto HTTP_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ac";
        static constexpr auto HTTP_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";

        static constexpr auto OUTPUT_SERVICE = "12345678-1234-1234-1234-1234567890ad";
        static constexpr auto OUTPUT_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";

        static constexpr auto ALEXA_SERVICE = "12345678-1234-1234-1234-1234567890ae";
        static constexpr auto ALEXA_SETTINGS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";

        static constexpr auto ESP_NOW_SERVICE = "12345678-1234-1234-1234-1234567890af";
        static constexpr auto ESP_NOW_DEVICES_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";

        static constexpr auto WIFI_SERVICE = "12345678-1234-1234-1234-1234567890ba";
        static constexpr auto WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";
        static constexpr auto WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
        static constexpr auto WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a";
        static constexpr auto WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000b";
    };

    static constexpr auto LOG_TAG = "BleManager";
    static constexpr auto BLE_TIMEOUT_MS = 30000;

    unsigned long bluetoothAdvertisementTimeout = 0;

    DeviceManager* deviceManager;
    WiFiManager* wifiManager;
    WebServerHandler* webServerHandler;
    Output* output;
    AlexaIntegration* alexaIntegration;

    NimBLEServer* server = nullptr;

public:
    explicit BleManager(DeviceManager* deviceManager, WiFiManager* wifiManager, WebServerHandler* webServerHandler,
                        Output* output = nullptr, AlexaIntegration* alexaIntegration = nullptr)
        : deviceManager(deviceManager), wifiManager(wifiManager), webServerHandler(webServerHandler),
          output(output), alexaIntegration(alexaIntegration)
    {
    }

    void start()
    {
        bluetoothAdvertisementTimeout = millis() + BLE_TIMEOUT_MS;
        if (server != nullptr) return;

        createServicesAndCharacteristics();
        startAdvertising();
    }

    void handle(const unsigned long now)
    {
        handleAdvertisementTimeout(now);
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
        advertising->setName(deviceManager->getDeviceName());

        constexpr uint16_t manufacturerID = 0x0000;
        uint8_t dataToAdvertise[4];
        dataToAdvertise[0] = manufacturerID >> 8 & 0xFF;
        dataToAdvertise[1] = manufacturerID & 0xFF;
        dataToAdvertise[2] = 0xAA;
        dataToAdvertise[3] = 0xAA;

        advertising->setManufacturerData(dataToAdvertise, sizeof(dataToAdvertise));
        advertising->start();
        bluetoothAdvertisementTimeout = millis() + BLE_TIMEOUT_MS;
        ESP_LOGI(LOG_TAG, "BLE advertising started with device name: %s", deviceManager->getDeviceName());
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

    void createServicesAndCharacteristics()
    {
        BLEDevice::init(deviceManager->getDeviceName());
        server = BLEDevice::createServer();
        server->setCallbacks(new BLEServerCallback());

        deviceManager->createServiceAndCharacteristics(server,
                                                       BLE_UUID::DEVICE_DETAILS_SERVICE,
                                                       BLE_UUID::DEVICE_RESTART_CHARACTERISTIC,
                                                       BLE_UUID::DEVICE_NAME_CHARACTERISTIC,
                                                       BLE_UUID::FIRMWARE_VERSION_CHARACTERISTIC,
                                                       BLE_UUID::DEVICE_HEAP_CHARACTERISTIC);
        output->createServiceAndCharacteristics(server,
                                                BLE_UUID::OUTPUT_SERVICE,
                                                BLE_UUID::OUTPUT_COLOR_CHARACTERISTIC);
        alexaIntegration->createServiceAndCharacteristics(server,
                                                          BLE_UUID::ALEXA_SERVICE,
                                                          BLE_UUID::ALEXA_SETTINGS_CHARACTERISTIC);
        EspNowHandler::createServiceAndCharacteristics(server,
                                                       BLE_UUID::ESP_NOW_SERVICE,
                                                       BLE_UUID::ESP_NOW_DEVICES_CHARACTERISTIC);
        webServerHandler->createServiceAndCharacteristics(server,
                                                          BLE_UUID::HTTP_DETAILS_SERVICE,
                                                          BLE_UUID::HTTP_CREDENTIALS_CHARACTERISTIC);
        wifiManager->createServiceAndCharacteristics(server,
                                                     BLE_UUID::WIFI_SERVICE,
                                                     BLE_UUID::WIFI_DETAILS_CHARACTERISTIC,
                                                     BLE_UUID::WIFI_STATUS_CHARACTERISTIC,
                                                     BLE_UUID::WIFI_SCAN_STATUS_CHARACTERISTIC,
                                                     BLE_UUID::WIFI_SCAN_RESULT_CHARACTERISTIC);
    }

    class BLEServerCallback final : public NimBLEServerCallbacks
    {
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override
        {
            pServer->getAdvertising()->start();
        }
    };
};
