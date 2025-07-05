#include <Arduino.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "device_manager.hh"
#include "remote_esp_now_handler.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "remote_hardware.hh"
#include "state_rest_handler.hh"
#include "rotary_encoder_manager.hh"
#include "websocket_handler.hh"

void startBle();
void toggleOutput();
void beginWebServer();
void adjustBrightness(long);
void encoderButtonPressed(unsigned long duration);

static constexpr auto LOG_TAG = "Remote";

PushButton boardButton(RemoteHardware::Pin::Button::BUTTON1);

RotaryEncoderManager rotaryEncoderManager(RemoteHardware::Pin::Header::H1::P1,
                                          RemoteHardware::Pin::Header::H1::P2,
                                          RemoteHardware::Pin::Header::H1::P3,
                                          RemoteHardware::Pin::Header::H1::P4);

WiFiManager wifiManager;
HTTP::Manager httpManager;
DeviceManager deviceManager;
EspNow::RemoteHandler espNowHandler;
OTA::Handler otaHandler(httpManager.getAuthenticationMiddleware());

std::array<uint8_t, 4> advertisementData =
    BLE::Manager::buildAdvertisementData(54321, 0xAA, 0xBB);

BLE::Manager bleManager(advertisementData,
                        deviceManager,
                        {
                            &deviceManager,
                            &wifiManager,
                            &httpManager,
                            &espNowHandler,
                        });

WebSocket::Handler webSocketHandler(nullptr,
                                    &otaHandler,
                                    &wifiManager,
                                    &httpManager,
                                    nullptr,
                                    &bleManager,
                                    &deviceManager,
                                    nullptr);

StateRestHandler stateRestHandler({
    &deviceManager,
    &wifiManager,
    &bleManager,
    &otaHandler,
    &espNowHandler
});

void setup()
{
    ESP_LOGI(LOG_TAG, "Starting controller");
    rotaryEncoderManager.begin();
    wifiManager.begin();
    deviceManager.begin();
    espNowHandler.begin();
    wifiManager.setGotIpCallback(beginWebServer);
    boardButton.setLongPressCallback(startBle);
    boardButton.setShortPressCallback(toggleOutput);
    rotaryEncoderManager.onChanged(adjustBrightness);
    rotaryEncoderManager.onPressed(encoderButtonPressed);

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
}

void toggleOutput()
{
    espNowHandler.send(EspNow::Message::Type::ToggleAll);
}

void startBle()
{
    bleManager.start();
}


void adjustBrightness(const long value)
{
    if (value > 0)
        espNowHandler.send(EspNow::Message::Type::IncreaseBrightness);
    else if (value < 0)
        espNowHandler.send(EspNow::Message::Type::DecreaseBrightness);
    rotaryEncoderManager.setEncoderValue(0);
}

void encoderButtonPressed(const unsigned long duration)
{
    if (duration < 2500)
    {
        espNowHandler.send(EspNow::Message::Type::ToggleAll);
        ESP_LOGI("Encoder", "Short press detected, toggling output");
    }
    else
    {
        bleManager.start();
        ESP_LOGI("Encoder", "Long press detected, starting BLE server");
    }
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
