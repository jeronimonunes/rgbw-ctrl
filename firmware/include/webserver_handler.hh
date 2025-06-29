#pragma once

#include <array>
#include "ESPAsyncWebServer.h"

#pragma pack(push, 1)
struct HttpCredentials
{
    static constexpr auto MAX_USERNAME_LENGTH = 32;
    static constexpr auto MAX_PASSWORD_LENGTH = 32;

    std::array<char, MAX_USERNAME_LENGTH + 1> username = {};
    std::array<char, MAX_PASSWORD_LENGTH + 1> password = {};
};
#pragma pack(pop)

class WebServerHandler
{
    static constexpr auto PREFERENCES_NAME = "http";
    static constexpr auto PREFERENCES_USERNAME_KEY = "u";
    static constexpr auto PREFERENCES_PASSWORD_KEY = "p";

    AsyncWebServer webServer = AsyncWebServer(80);

    AsyncAuthenticationMiddleware authMiddleware;

public:
    void begin(AsyncWebHandler* alexaHandler, AsyncWebHandler* ws, AsyncWebHandler* restHandler)
    {
        webServer.addHandler(ws)
                 .addMiddleware(&authMiddleware);

        webServer.addHandler(restHandler)
                 .addMiddleware(&authMiddleware);

        webServer.addHandler(alexaHandler);
        // Alexa can't have authentication middleware

        webServer.serveStatic("/", LittleFS, "/")
                 .setDefaultFile("index.html")
                 .setTryGzipFirst(true)
                 .setCacheControl("no-cache")
                 .addMiddleware(&authMiddleware);

        updateServerCredentials(getCredentials());
        webServer.begin();
    }

    [[nodiscard]] AsyncWebServer* getWebServer()
    {
        return &webServer;
    }

    [[nodiscard]] const AsyncAuthenticationMiddleware& getAuthenticationMiddleware() const
    {
        return authMiddleware;
    }

    void updateCredentials(const HttpCredentials& credentials)
    {
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);
        prefs.putString(PREFERENCES_USERNAME_KEY, credentials.username.data());
        prefs.putString(PREFERENCES_PASSWORD_KEY, credentials.password.data());
        prefs.end();
        updateServerCredentials(credentials);
    }

    [[nodiscard]] static HttpCredentials getCredentials()
    {
        HttpCredentials credentials;
        if (Preferences prefs; prefs.begin(PREFERENCES_NAME, true))
        {
            strncpy(credentials.username.data(), prefs.getString(PREFERENCES_USERNAME_KEY, "admin").c_str(),
                    HttpCredentials::MAX_USERNAME_LENGTH);
            strncpy(credentials.password.data(), prefs.getString(PREFERENCES_PASSWORD_KEY, "").c_str(),
                    HttpCredentials::MAX_PASSWORD_LENGTH);
            prefs.end();
            return credentials;
        }
        Preferences prefs;
        prefs.begin(PREFERENCES_NAME, false);
        strncpy(credentials.username.data(), "admin", HttpCredentials::MAX_USERNAME_LENGTH);
        strncpy(credentials.password.data(), generateRandomPassword().c_str(), HttpCredentials::MAX_PASSWORD_LENGTH);
        prefs.putString(PREFERENCES_USERNAME_KEY, credentials.username.data());
        prefs.putString(PREFERENCES_PASSWORD_KEY, credentials.password.data());
        prefs.end();
        return credentials;
    }

private:
    void updateServerCredentials(const HttpCredentials& credentials)
    {
        authMiddleware.setUsername(credentials.username.data());
        authMiddleware.setPassword(credentials.password.data());
        authMiddleware.setRealm("rgbw-ctrl");
        authMiddleware.setAuthFailureMessage("Authentication failed");
        authMiddleware.setAuthType(AUTH_BASIC);
        authMiddleware.generateHash();
    }

    [[nodiscard]] static String generateRandomPassword()
    {
        const auto v1 = random(100000, 999999);
        const auto v2 = random(100000, 999999);
        String password = String(v1) + "A-b" + String(v2);
        if (password.length() > HttpCredentials::MAX_PASSWORD_LENGTH)
        {
            password = password.substring(0, HttpCredentials::MAX_PASSWORD_LENGTH);
        }
        return password;
    }
};
