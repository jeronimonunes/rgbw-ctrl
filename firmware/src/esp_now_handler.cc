#include "esp_now_handler.hh"

std::vector<std::array<uint8_t, 6>> EspNowHandler::allowedMacs = {};

std::function<void(EspNowMessage* message)> EspNowHandler::callback = nullptr;
