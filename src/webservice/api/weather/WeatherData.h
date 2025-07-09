#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <Arduino.h>

// Helper struct for degrees and unit
struct WeatherValueDegrees {
    float degrees;
    String unit; // "CELSIUS"
};

// Helper struct for quantity and unit (e.g., for precipitation)
struct WeatherValueQuantity {
    float quantity;
    String unit; // "MILLIMETERS"
};

// Helper struct for wind direction
struct WindDirection {
    int degrees;
    String cardinal; // "NORTH_NORTHWEST"
};

// Helper struct for wind speed/gust
struct WindSpeed {
    float value;
    String unit; // "KILOMETERS_PER_HOUR"
};

class WeatherData {
public:
    WeatherData(); // Standardkonstruktor zur Initialisierung

    // --- Direct Values ---
    float relativeHumidity; // 71
    int uvIndex;            // 9
    int thunderstormProbability; // 0
    int cloudCover;         // 12

    // --- Nested Objects (using helper structs) ---
    WeatherValueDegrees dewPoint;
    WeatherValueDegrees heatIndex;
    WeatherValueDegrees windChill;

    // --- Precipitation ---
    struct PrecipitationData {
        struct Probability {
            int percent;
            String type; // "RAIN"
        } probability;
        WeatherValueQuantity snowQpf;
        WeatherValueQuantity qpf;
    } precipitation;

    // --- Air Pressure ---
    struct AirPressureData {
        float meanSeaLevelMillibars; // 1020.74
    } airPressure;

    // --- Wind ---
    struct WindData {
        WindDirection direction;
        WindSpeed speed;
        WindSpeed gust;
    } wind;

    // --- Visibility ---
    struct VisibilityData {
        float distance;
        String unit; // "KILOMETERS"
    } visibility;

    // --- Current Conditions History (only if needed) ---
    struct ConditionsHistoryData {
        WeatherValueDegrees temperatureChange;
        WeatherValueDegrees maxTemperature;
        WeatherValueDegrees minTemperature;
        WeatherValueQuantity snowQpf;
        WeatherValueQuantity qpf;
    } currentConditionsHistory;

    // Methode zum Zurücksetzen aller Werte (optional, aber gut für Wiederverwendung)
    void reset();

    // Methode zum Debugging (optional)
    // void printToSerial();
};

#endif // WEATHER_DATA_H