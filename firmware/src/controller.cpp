#include <Arduino.h>
#include <LittleFS.h>
#include <esp_now.h>

#include "wifi_manager.hh"
#include "board_led.hh"
#include "alexa_integration.hh"
#include "device_manager.hh"
#include "esp_now_handler.hh"
#include "output.hh"
#include "push_button.hh"
#include "ota_handler.hh"
#include "state_rest_handler.hh"
#include "rotary_encoder_manager.hh"
#include "websocket_handler.hh"

void startBle();
void toggleOutput();
void beginAlexaAndWebServer();
void adjustBrightness(long);
void encoderButtonPressed(unsigned long duration);
void onDataReceived(const uint8_t* mac, const uint8_t* incomingData, int len);
void shutdownCallback();

static constexpr auto LOG_TAG = "Controller";

Output output;
BoardLED boardLED;
OtaHandler otaHandler;
PushButton boardButton;
WiFiManager wifiManager;
EspNowHandler espNowHandler;
HTTP::Manager httpManager;
RotaryEncoderManager rotaryEncoderManager;
AlexaIntegration alexaIntegration(output);
DeviceManager deviceManager;

BLE::Manager bleManager(deviceManager, {
                            &deviceManager,
                            &wifiManager,
                            &httpManager,
                            &output,
                            &espNowHandler,
                            &alexaIntegration
                        });

WebSocket::Handler webSocketHandler(&output,
                                  &otaHandler,
                                  &wifiManager,
                                  &httpManager,
                                  &alexaIntegration,
                                  &bleManager,
                                  &deviceManager,
                                  &espNowHandler);

StateRestHandler stateRestHandler({
    &deviceManager,
    &wifiManager,
    &bleManager,
    &output,
    &otaHandler,
    &alexaIntegration,
    &espNowHandler
});

void setup()
{
    ESP_LOGI(LOG_TAG, "Starting controller");
    boardLED.begin();
    output.begin();
    rotaryEncoderManager.begin();
    wifiManager.begin();
    deviceManager.begin();
    esp_now_init();
    esp_now_register_recv_cb(onDataReceived);
    espNowHandler.begin();
    otaHandler.begin(httpManager);
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
    esp_register_shutdown_handler(shutdownCallback);
    ESP_LOGI(LOG_TAG, "Startup complete");
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
    httpManager.begin(
        alexaIntegration.createAsyncWebHandler(),
        webSocketHandler.getAsyncWebHandler(),
        {
            &stateRestHandler,
            &bleManager,
            &deviceManager,
            &output
        }
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

void onDataReceived(const uint8_t* mac, const uint8_t* incomingData, const int len)
{
    ESP_LOGI(LOG_TAG, "Data received from %02X:%02X:%02X:%02X:%02X:%02X, length: %d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);

    if (!espNowHandler.isMacAllowed(mac))
    {
        ESP_LOGW(LOG_TAG, "MAC address not allowed, ignoring packet");
        return;
    }

    if (len != sizeof(EspNowMessage)) return;

    const auto message = reinterpret_cast<EspNowMessage*>(const_cast<uint8_t*>(incomingData));

    onEspNowMessage(message);
}

void shutdownCallback()
{
    ESP_LOGI(LOG_TAG, "Shutdown completed");
}
