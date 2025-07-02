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
#include "ble_interfaceable.hh"
#include "http_handler.hh"

enum class BleStatus :uint8_t
{
    OFF,
    ADVERTISING,
    CONNECTED
};

class BleManager final : public StateJsonFiller, public HttpHandler
{
    static constexpr auto LOG_TAG = "BleManager";
    static constexpr auto BLE_TIMEOUT_MS = 30000;

    unsigned long bluetoothAdvertisementTimeout = 0;

    const DeviceManager& deviceManager;
    const std::vector<BleInterfaceable*> services;

    NimBLEServer* server = nullptr;

public:
    explicit BleManager(DeviceManager& deviceManager, const std::vector<BleInterfaceable*>&& services)
        : deviceManager(deviceManager), services(services)
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

    void disconnectAllClients() const
    {
        if (server == nullptr) return;
        ESP_LOGI(LOG_TAG, "Disconnecting all BLE clients");
        for (const auto& connInfo : this->server->getPeerDevices())
        {
            this->server->disconnect(connInfo); // NOLINT
        }
    }

    void stop() const
    {
        if (server == nullptr) return;
        disconnectAllClients();
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

    void fillState(const JsonObject& root) const override
    {
        const auto& ble = root["ble"].to<JsonObject>();
        ble["status"] = getStatusString();
    }

    AsyncWebHandler* createAsyncWebHandler() override
    {
        return new AsyncRestWebHandler(this);
    }

private:
    void startAdvertising()
    {
        const auto advertising = this->server->getAdvertising();
        advertising->setName(deviceManager.getDeviceName());

        constexpr uint16_t manufacturerID = 0x0000;
        uint8_t dataToAdvertise[4];
        dataToAdvertise[0] = manufacturerID >> 8 & 0xFF;
        dataToAdvertise[1] = manufacturerID & 0xFF;
        dataToAdvertise[2] = 0xAA;
        dataToAdvertise[3] = 0xAA;

        advertising->setManufacturerData(dataToAdvertise, sizeof(dataToAdvertise));
        advertising->start();
        bluetoothAdvertisementTimeout = millis() + BLE_TIMEOUT_MS;
        ESP_LOGI(LOG_TAG, "BLE advertising started with device name: %s", deviceManager.getDeviceName());
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
        BLEDevice::init(deviceManager.getDeviceName());
        server = BLEDevice::createServer();
        server->setCallbacks(new BLEServerCallback());

        for (const auto& service : services)
        {
            service->createServiceAndCharacteristics(server);
        }
    }

    class AsyncRestWebHandler final : public AsyncWebHandler
    {
        BleManager* bleManager;

    public:
        explicit AsyncRestWebHandler(BleManager* bleManager)
            : bleManager(bleManager)
        {
        }

        bool canHandle(AsyncWebServerRequest* request) const override
        {
            return request->method() == HTTP_GET &&
                request->url().startsWith("/bluetooth");
        }

        void handleRequest(AsyncWebServerRequest* request) override
        {
            if (!request->hasParam("state"))
                return sendMessageJsonResponse(request, "Missing 'state' parameter");

            auto state = request->getParam("state")->value() == "on";
            request->onDisconnect([this, state]
            {
                if (state)
                    bleManager->start();
                else
                    bleManager->stop();
            });

            if (state)
                return sendMessageJsonResponse(request, "Bluetooth enabled");
            return sendMessageJsonResponse(request, "Bluetooth disabled, device will restart");
        }
    };

    class BLEServerCallback final : public NimBLEServerCallbacks
    {
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override
        {
            pServer->getAdvertising()->start();
        }
    };
};
