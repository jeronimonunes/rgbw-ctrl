#include <Arduino.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "device_manager.hh"
#include "esp_now_handler_remote.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "remote_hardware.hh"
#include "state_rest_handler.hh"
#include "rotary_encoder_manager.hh"
#include "websocket_handler.hh"

void beginWebServer();

static constexpr auto LOG_TAG = "Remote";

PushButton boardButton(RemoteHardware::Pin::Button::BUTTON1);

PushButton rotaryEncoderButton(RemoteHardware::Pin::Header::H1::P3);
RotaryEncoderManager rotaryEncoderManager(RemoteHardware::Pin::Header::H1::P1,
                                          RemoteHardware::Pin::Header::H1::P2,
                                          RemoteHardware::Pin::Header::H1::P4);

WiFiManager wifiManager;
HTTP::Manager httpManager;
DeviceManager deviceManager;
EspNow::RemoteHandler remoteEspNowHandler;
OTA::Handler otaHandler(httpManager.getAuthenticationMiddleware());

std::array<uint8_t, 4> advertisementData =
    BLE::Manager::buildAdvertisementData(54321, 0xAA, 0xBB);

BLE::Manager bleManager(advertisementData,
                        deviceManager,
                        {
                            &deviceManager,
                            &wifiManager,
                            &httpManager,
                            &remoteEspNowHandler,
                        });

WebSocket::Handler webSocketHandler(nullptr,
                                    &otaHandler,
                                    &wifiManager,
                                    &httpManager,
                                    nullptr,
                                    &bleManager,
                                    &deviceManager,
                                    nullptr,
                                    &remoteEspNowHandler);

StateRestHandler stateRestHandler({
    &deviceManager,
    &wifiManager,
    &bleManager,
    &otaHandler,
    &remoteEspNowHandler
});

void setup()
{
    ESP_LOGI(LOG_TAG, "Starting controller");
    rotaryEncoderManager.begin();
    rotaryEncoderButton.begin();
    wifiManager.begin();
    deviceManager.begin();
    remoteEspNowHandler.begin();

    wifiManager.setGotIpCallback(beginWebServer);

    boardButton.setLongPressCallback([] { bleManager.start(); });
    boardButton.setShortPressCallback([] { remoteEspNowHandler.send(EspNow::Message::Type::ToggleAll); });

    rotaryEncoderManager.onTurnLeft([] { remoteEspNowHandler.send(EspNow::Message::Type::DecreaseBrightness); });
    rotaryEncoderManager.onTurnRight([] { remoteEspNowHandler.send(EspNow::Message::Type::IncreaseBrightness); });

    rotaryEncoderButton.setLongPressCallback([] { bleManager.start(); });
    rotaryEncoderButton.setShortPressCallback([] { remoteEspNowHandler.send(EspNow::Message::Type::ToggleAll); });

    LittleFS.begin(true);
    if (const auto credentials = WiFiManager::loadCredentials())
        wifiManager.connect(credentials.value());
    else
        bleManager.start();
    ESP_LOGI(LOG_TAG, "Startup complete");
}

void loop()
{
    const auto now = millis();

    bleManager.handle(now);
    boardButton.handle(now);
    deviceManager.handle(now);
    webSocketHandler.handle(now);
    rotaryEncoderButton.handle(now);
    delay(1);
}


void beginWebServer()
{
    httpManager.begin(
        nullptr,
        {
            &webSocketHandler,
            &otaHandler,
            &stateRestHandler,
            &bleManager,
            &deviceManager
        }
    );
}
