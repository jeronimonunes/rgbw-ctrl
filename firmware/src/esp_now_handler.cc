#include "esp_now_handler.hh"

std::vector<EspNowDevice> EspNowHandler::devices = {};

std::function<void(EspNowMessage* message)> EspNowHandler::callback = nullptr;
