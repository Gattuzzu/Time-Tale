#ifndef POLLEN_CLIENT_H
#define POLLEN_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../../logger/Logger.h"
#include "../../../logger/LogLevel.h"
#include "../ApiClient.h"
#include "PollenData.h"

class PollenClient : public ApiClient {
public:
    static PollenClient& getInstance(const char* host = nullptr, const char* apiKey = nullptr) {
        static PollenClient instance;
        if (host != nullptr && apiKey != nullptr) {
            instance.configure(host, apiKey);
        }
        return instance;
    }

    bool getCurrentPollen(float latitude, float longitude, PollenData& outPollenData);

private:
    PollenClient() : ApiClient() {}

    PollenClient(const PollenClient&) = delete;
    PollenClient& operator=(const PollenClient&) = delete;

    bool parsePollenJson(JsonDocument& doc, PollenData& outPollenData);
};

#endif // POLLEN_CLIENT_H