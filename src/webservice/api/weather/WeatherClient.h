#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../../logger/Logger.h"
#include "../../../logger/LogLevel.h"
#include "../ApiClient.h" // Neue Basisklasse
#include "WeatherData.h" // Die WeatherData Klasse

// Forward Declaration für WeatherData (nicht mehr nötig, wenn include)
// class WeatherData; // <--- Dies kann jetzt entfernt werden, da es includiert wird

class WeatherClient : public ApiClient { // Erbt von ApiClient
public:
    // Statische Methode, um die einzige Instanz von WeatherClient zu erhalten.
    static WeatherClient& getInstance(const char* host = nullptr, const char* apiKey = nullptr) {
        static WeatherClient instance; // Singleton-Instanz
        if (host != nullptr && apiKey != nullptr) {
            instance.configure(host, apiKey); // Konfiguration über Basisklasse
        }
        return instance;
    }

    // Methode zum Abrufen der Wetterdaten
    bool getCurrentConditions(float latitude, float longitude, WeatherData& outWeatherData);

private:
    // Privater Konstruktor, um direkte Instanziierung zu verhindern.
    // Wird nur von getInstance() aufgerufen.
    // Ruft den Konstruktor der Basisklasse auf
    WeatherClient() : ApiClient() {}

    // Private Kopierkonstruktor und Zuweisungsoperator verhindern Kopien
    WeatherClient(const WeatherClient&) = delete;
    WeatherClient& operator=(const WeatherClient&) = delete;

    // Hilfsfunktion zum Parsen des JSON und Befüllen des WeatherData-Objekts
    // Diese Methode ist spezifisch für Wetterdaten
    bool parseWeatherJson(JsonDocument& doc, WeatherData& outWeatherData);
};

#endif // WEATHER_CLIENT_H