#pragma once

#include <AsyncJson.h>


class HttpHandler
{
public:
    virtual ~HttpHandler() = default;
    virtual AsyncWebHandler* createAsyncWebHandler() = 0;

    static void sendMessageJsonResponse(AsyncWebServerRequest* request, const char* message)
    {
        auto* response = new AsyncJsonResponse();
        response->getRoot().to<JsonObject>()["message"] = message;
        response->addHeader("Cache-Control", "no-store");
        response->setLength();
        request->send(response);
    }

    static std::optional<uint8_t> extractUint8Param(const AsyncWebServerRequest* req, const char* key)
    {
        if (!req->hasParam(key)) return std::nullopt;
        const auto value = std::clamp(req->getParam(key)->value().toInt(), 0l, 255l);
        return static_cast<uint8_t>(value);
    }
};
