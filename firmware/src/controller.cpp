#include <Arduino.h>
#include <LittleFS.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "device_manager.hh"
#include "esp_now_handler.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "rest_handler.hh"
#include "rotary_encoder_manager.hh"
#include "websocket_handler.hh"

void startBle();
void toggleOutput();
void beginAlexaAndWebServer();
void onEspNowMessage(const EspNowMessage* message);
void adjustBrightness(long);
void encoderButtonPressed(unsigned long duration);


Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
WebServerHandler webServerHandler;
RotaryEncoderManager rotaryEncoderManager;
AlexaIntegration alexaIntegration(output);
DeviceManager deviceManager;
BleManager bleManager(&deviceManager,
                      &wifiManager,
                      &webServerHandler,
                      &output,
                      &alexaIntegration);

WebSocketHandler webSocketHandler(output,
                                  otaHandler,
                                  wifiManager,
                                  webServerHandler,
                                  alexaIntegration,
                                  bleManager,
                                  deviceManager);

RestHandler restHandler(output,
                        otaHandler,
                        wifiManager,
                        alexaIntegration,
                        bleManager,
                        deviceManager);

void setup()
{
    boardLED.begin();
    output.begin();
    rotaryEncoderManager.begin();
    wifiManager.begin();
    deviceManager.begin();
    EspNowHandler::begin(onEspNowMessage);
    otaHandler.begin(webServerHandler);
    wifiManager.setGotIpCallback(beginAlexaAndWebServer);
    boardButton.setLongPressCallback(startBle);
    boardButton.setShortPressCallback(toggleOutput);
    rotaryEncoderManager.onChanged(adjustBrightness);
    rotaryEncoderManager.onPressed(encoderButtonPressed);

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
    deviceManager.handle(now);
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


void adjustBrightness(const long value)
{
    if (value > 0)
        output.increaseBrightness();
    else if (value < 0)
        output.decreaseBrightness();
    rotaryEncoderManager.setEncoderValue(0);
}

void encoderButtonPressed(const unsigned long duration)
{
    if (duration < 2500)
    {
        output.toggleAll();
        ESP_LOGI("Encoder", "Short press detected, toggling output");
    }
    else
    {
        bleManager.start();
        ESP_LOGI("Encoder", "Long press detected, starting BLE server");
    }
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
