#include <Arduino.h>
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

void startBle();
void toggleOutput();
void beginAlexaAndWebServer();
void onEspNowMessage(const EspNowMessage* message);


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
    boardLED.begin();
    output.begin();
    wifiManager.begin();
    EspNowHandler::begin(onEspNowMessage);
    otaHandler.begin(webServerHandler);
    wifiManager.setGotIpCallback(beginAlexaAndWebServer);
    boardButton.setLongPressCallback(startBle);
    boardButton.setShortPressCallback(toggleOutput);

    LittleFS.begin(true);
    if (const auto credentials = WiFiManager::loadCredentials())
        wifiManager.connect(credentials.value());
    else
        bleManager.start();
}

void loop()
{
    const auto now = millis();

    boardButton.handle(now);
    alexaIntegration.handle(now);
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

void toggleOutput()
{
    output.toggleAll();
}

void startBle()
{
    bleManager.start();
}

void beginAlexaAndWebServer()
{
    alexaIntegration.begin();
    webServerHandler.begin(
        alexaIntegration.createAsyncWebHandler(),
        webSocketHandler.getAsyncWebHandler(),
        restHandler.createAsyncWebHandler()
    );
}

void onEspNowMessage(const EspNowMessage* message)
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
}
