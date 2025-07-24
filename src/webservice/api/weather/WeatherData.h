#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <Arduino.h>

// Helper struct for degrees and unit
struct WeatherValueDegrees {
    float degrees;
    String unit; // "CELSIUS"
};

// --- Enum für Wetterbedingungen ---
// Dies ist das Enum, das die verschiedenen Wetterbedingungen darstellt.
// Wir fügen einen UNKNOWN-Wert für den Fall hinzu, dass die Bedingung nicht erkannt wird.
enum class WeatherConditionType {
    // TYPE_UNSPECIFIED,
    CLEAR,
    // MOSTLY_CLEAR,
    PARTLY_CLOUDY,
    // MOSTLY_CLOUDY,
    CLOUDY,
    // WINDY,
    // WIND_AND_RAIN,
    // LIGHT_RAIN_SHOWERS,
    // CHANCE_OF_SHOWERS,
    // SCATTERED_SHOWERS,
    // RAIN_SHOWERS,
    // HEAVY_RAIN_SHOWERS,
    // LIGHT_TO_MODERATE_RAIN,
    // MODERATE_TO_HEAVY_RAIN,
    RAIN,
    // LIGHT_RAIN,
    // HEAVY_RAIN,
    // RAIN_PERIODICALLY_HEAVY,
    // LIGHT_SNOW_SHOWERS,
    // CHANCE_OF_SNOW_SHOWERS,
    // SCATTERED_SNOW_SHOWERS,
    // SNOW_SHOWERS,
    // HEAVY_SNOW_SHOWERS,
    // LIGHT_TO_MODERATE_SNOW,
    // MODERATE_TO_HEAVY_SNOW,
    SNOW,
    // LIGHT_SNOW,
    // HEAVY_SNOW,
    // SNOWSTORM,
    // SNOW_PERIODICALLY_HEAVY,
    // HEAVY_SNOW_STORM,
    // BLOWING_SNOW,
    // RAIN_AND_SNOW,
    // HAIL,
    // HAIL_SHOWERS,
    THUNDERSTORM,
    // THUNDERSHOWER,
    // LIGHT_THUNDERSTORM_RAIN,
    // SCATTERED_THUNDERSTORMS,
    // HEAVY_THUNDERSTORM,
    UNKNOWN // Standardwert, falls die Bedingung nicht gemappt werden kann
};

// Nur die benötigten Datenmember werden beibehalten
class WeatherData {
public:
    WeatherData(); // Standardkonstruktor zur Initialisierung

    // --- Benötigte Datenfelder ---
    WeatherValueDegrees temperature; // Für die aktuelle Temperatur (aus "temperature" im JSON)
    float relativeHumidity;          // Luftfeuchtigkeit (aus "relativeHumidity" im JSON)
    WeatherConditionType weatherType; // Beschreibung der Wetterart als Enum (aus "weatherCondition.description.text" im JSON)

    // Methode zum Zurücksetzen aller Werte
    void reset();

    // Methode zum Mappen von / zu WeatherConditionType
    static WeatherConditionType weatherConditionStringToType(const String& typeString);
    static String weatherConditionTypeToString(WeatherConditionType type);

    // Methode zum Debugging (optional, kannst du bei Bedarf wieder einkommentieren)
    String toString();
};

#endif // WEATHER_DATA_H