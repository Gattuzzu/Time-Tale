#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
// #include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "../../../logger/Logger.h"
#include "../../../logger/LogLevel.h"

// Forward Declaration für WeatherData
class WeatherData;

class WeatherClient {
public:
    // Statische Methode, um die einzige Instanz von WeatherClient zu erhalten.
    // Die Konfigurationsparameter werden hier beim ERSTEN Aufruf übergeben.
    static WeatherClient& getInstance(const char* host = nullptr, const char* apiKey = nullptr) {
        // Statische Variable, die die einzige Instanz hält.
        // Sie wird nur einmal initialisiert, wenn getInstance zum ersten Mal aufgerufen wird.
        static WeatherClient instance(host, apiKey);
        return instance;
    }

    // Methode zum Abrufen der Wetterdaten
    // Gibt true bei Erfolg, false bei Fehler zurück
    bool getCurrentConditions(float latitude, float longitude, WeatherData& outWeatherData);

private:
    // Privater Konstruktor, um direkte Instanziierung zu verhindern.
    // Wird nur von getInstance() aufgerufen.
    WeatherClient(const char* host, const char* apiKey);

    // Private Kopierkonstruktor und Zuweisungsoperator verhindern Kopien
    WeatherClient(const WeatherClient&) = delete;
    WeatherClient& operator=(const WeatherClient&) = delete;

    const char* _host;
    const char* _apiKey; // Dein Google Cloud API Access Token
    WiFiSSLClient _client; // Für HTTPS-Verbindungen

    // Hilfsfunktion zum Parsen des JSON und Befüllen des WeatherData-Objekts
    bool parseJson(JsonDocument& doc, WeatherData& outWeatherData);
};

#endif // WEATHER_CLIENT_H