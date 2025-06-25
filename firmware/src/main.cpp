#include <Arduino.h>
#include <nvs_flash.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "esp_now_handler.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "rest_handler.hh"
#include "websocket_handler.hh"

std::vector<std::array<uint8_t, 6>> EspNowHandler::allowedMacs = {};

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
WebServerHandler webServerHandler;
AlexaIntegration alexaIntegration(output);
BleManager bleManager(output,
                      wifiManager,
                      alexaIntegration,
                      webServerHandler);
WebSocketHandler webSocketHandler(output,
                                  otaHandler,
                                  wifiManager,
                                  webServerHandler,
                                  alexaIntegration,
                                  bleManager);

RestHandler restHandler(output,
                        otaHandler,
                        wifiManager,
                        alexaIntegration,
                        bleManager);

void setup()
{
    nvs_flash_init();
    boardLED.begin();
    output.begin();
    wifiManager.begin();
    EspNowHandler::begin([](const uint8_t* mac, EspNowMessage* message)
    {
        switch (message->type)
        {
        case EspNowMessage::Type::ToggleRed:
            output.toggle(Color::Red);
            break;
        case EspNowMessage::Type::ToggleGreen:
            output.toggle(Color::Green);
            break;
        case EspNowMessage::Type::ToggleBlue:
            output.toggle(Color::Blue);
            break;
        case EspNowMessage::Type::ToggleWhite:
            output.toggle(Color::White);
            break;
        case EspNowMessage::Type::ToggleAll:
            output.toggleAll();
            break;
        case EspNowMessage::Type::TurnOffAll:
            output.turnOffAll();
            break;
        case EspNowMessage::Type::TurnOnAll:
            output.turnOnAll();
            break;
        case EspNowMessage::Type::IncreaseBrightness:
            output.increaseBrightness();
            break;
        case EspNowMessage::Type::DecreaseBrightness:
            output.decreaseBrightness();
            break;
        }
        alexaIntegration.updateDevices();
    });

    otaHandler.begin(webServerHandler);
    wifiManager.setGotIpCallback([]
    {
        alexaIntegration.begin();
        webServerHandler.begin(
            alexaIntegration.createAsyncWebHandler(),
            webSocketHandler.getAsyncWebHandler(),
            restHandler.createAsyncWebHandler()
        );
    });

    if (const auto credentials = WiFiManager::loadCredentials())
        wifiManager.connect(credentials.value());
    else
        bleManager.start();

    boardButton.setLongPressCallback([]
    {
        bleManager.start();
    });

    boardButton.setShortPressCallback([]
    {
        output.toggleAll();
        alexaIntegration.updateDevices();
    });

    LittleFS.begin(true);
}

void loop()
{
    const auto now = millis();

    boardButton.handle(now);
    alexaIntegration.handle();
    webSocketHandler.handle(now);
    bleManager.handle(now);
    output.handle(now);

    boardLED.handle(
        now,
        bleManager.getStatus(),
        wifiManager.getScanStatus(),
        wifiManager.getStatus(),
        otaHandler.getStatus() == OtaStatus::Started
    );
}
