#include "esp_now_handler.hh"

std::function<void(EspNowMessage* message)> EspNowHandler::callback = nullptr;
EspNowDeviceData EspNowHandler::deviceData = {};

