#include "WeatherClient.h"
#include "WeatherData.h" // Benötigt die vollständige Definition von WeatherData
#include <Secrets.h>

// Implementierung von getCurrentConditions
bool WeatherClient::getCurrentConditions(float latitude, float longitude, WeatherData& outWeatherData) {
    outWeatherData.reset(); // Vor dem Abruf zurücksetzen

    String path = "/v1/currentConditions:lookup?key=";
    path += _apiKey;
    path += "&languageCode=CH&location.latitude=";
    path += String(latitude, 6); // 6 Dezimalstellen für Präzision
    path += "&location.longitude=";
    path += String(longitude, 6);
    path += "&unitsSystem=METRIC&alt=json";

    JsonDocument doc;
    if (!sendGetRequest(path, doc)) { // Aufruf der Basisklassenmethode
        return false;
    }

    // Spezifisches Parsing der Wetterdaten
    return parseWeatherJson(doc, outWeatherData);
}

// Hilfsfunktion zum Parsen des JSON und Befüllen des WeatherData-Objekts
bool WeatherClient::parseWeatherJson(JsonDocument& doc, WeatherData& outWeatherData) {

    String jsonString;
    size_t bytesWritten = serializeJson(doc, jsonString);
    Logger::log(LogLevel::Debug, "Serialized JSON string (" + String(bytesWritten) + " bytes): " + jsonString);

    // 1. Temperatur (aus "temperature" Objekt)
    outWeatherData.temperature.degrees = doc["temperature"]["degrees"] | 0.0f;
    outWeatherData.temperature.unit = doc["temperature"]["unit"].as<String>();

    // 2. Luftfeuchtigkeit (aus "relativeHumidity")
    outWeatherData.relativeHumidity = doc["relativeHumidity"] | 0.0f;

    // 3. Wetterart als Enum (aus "weatherCondition.description.text")
    if (!doc["weatherCondition"].isNull()) {
        String typeString = doc["weatherCondition"]["type"].as<String>();
        outWeatherData.weatherType = WeatherData::weatherConditionStringToType(typeString);
    } else {
        outWeatherData.weatherType = WeatherConditionType::UNKNOWN; // Fallback, falls das Feld fehlt
    }

    Logger::log(LogLevel::Info, outWeatherData.toString());

    return true;
}