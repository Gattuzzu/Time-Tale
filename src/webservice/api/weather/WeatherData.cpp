#include "WeatherData.h"

WeatherData::WeatherData() {
    reset(); // Initialisiere alle Werte auf Standard
}

void WeatherData::reset() {
    // Initialisiere nur die verbleibenden Felder
    temperature.degrees = 0.0f;
    temperature.unit = "";
    relativeHumidity = 0.0f;
    weatherType = WeatherConditionType::UNKNOWN; // Standardwert für das Enum
}

WeatherConditionType WeatherData::weatherConditionStringToType(const String& typeString){
    // Map the string to the enum value
    if      (typeString == "TYPE_UNSPECIFIED")        return WeatherConditionType::UNKNOWN;
    else if (typeString == "CLEAR")                   return WeatherConditionType::CLEAR;
    else if (typeString == "MOSTLY_CLEAR")            return WeatherConditionType::CLEAR;
    else if (typeString == "PARTLY_CLOUDY")           return WeatherConditionType::PARTLY_CLOUDY;
    else if (typeString == "MOSTLY_CLOUDY")           return WeatherConditionType::CLOUDY;
    else if (typeString == "CLOUDY")                  return WeatherConditionType::CLOUDY;
    else if (typeString == "WINDY")                   return WeatherConditionType::PARTLY_CLOUDY;
    else if (typeString == "WIND_AND_RAIN")           return WeatherConditionType::RAIN;
    else if (typeString == "LIGHT_RAIN_SHOWERS")      return WeatherConditionType::RAIN;
    else if (typeString == "CHANCE_OF_SHOWERS")       return WeatherConditionType::CLOUDY;
    else if (typeString == "SCATTERED_SHOWERS")       return WeatherConditionType::RAIN;
    else if (typeString == "RAIN_SHOWERS")            return WeatherConditionType::RAIN;
    else if (typeString == "HEAVY_RAIN_SHOWERS")      return WeatherConditionType::RAIN;
    else if (typeString == "LIGHT_TO_MODERATE_RAIN")  return WeatherConditionType::RAIN;
    else if (typeString == "MODERATE_TO_HEAVY_RAIN")  return WeatherConditionType::RAIN;
    else if (typeString == "RAIN")                    return WeatherConditionType::RAIN;
    else if (typeString == "LIGHT_RAIN")              return WeatherConditionType::RAIN;
    else if (typeString == "HEAVY_RAIN")              return WeatherConditionType::RAIN;
    else if (typeString == "RAIN_PERIODICALLY_HEAVY") return WeatherConditionType::RAIN;
    else if (typeString == "LIGHT_SNOW_SHOWERS")      return WeatherConditionType::SNOW;
    else if (typeString == "CHANCE_OF_SNOW_SHOWERS")  return WeatherConditionType::CLOUDY;
    else if (typeString == "SCATTERED_SNOW_SHOWERS")  return WeatherConditionType::SNOW;
    else if (typeString == "SNOW_SHOWERS")            return WeatherConditionType::SNOW;
    else if (typeString == "HEAVY_SNOW_SHOWERS")      return WeatherConditionType::SNOW;
    else if (typeString == "LIGHT_TO_MODERATE_SNOW")  return WeatherConditionType::SNOW;
    else if (typeString == "MODERATE_TO_HEAVY_SNOW")  return WeatherConditionType::SNOW;
    else if (typeString == "SNOW")                    return WeatherConditionType::SNOW;
    else if (typeString == "LIGHT_SNOW")              return WeatherConditionType::SNOW;
    else if (typeString == "HEAVY_SNOW")              return WeatherConditionType::SNOW;
    else if (typeString == "SNOWSTORM")               return WeatherConditionType::SNOW;
    else if (typeString == "SNOW_PERIODICALLY_HEAVY") return WeatherConditionType::SNOW;
    else if (typeString == "HEAVY_SNOW_STORM")        return WeatherConditionType::SNOW;
    else if (typeString == "BLOWING_SNOW")            return WeatherConditionType::SNOW;
    else if (typeString == "RAIN_AND_SNOW")           return WeatherConditionType::SNOW;
    else if (typeString == "HAIL")                    return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "HAIL_SHOWERS")            return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "THUNDERSTORM")            return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "THUNDERSHOWER")           return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "LIGHT_THUNDERSTORM_RAIN") return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "SCATTERED_THUNDERSTORMS") return WeatherConditionType::THUNDERSTORM;
    else if (typeString == "HEAVY_THUNDERSTORM")      return WeatherConditionType::THUNDERSTORM;

    return WeatherConditionType::UNKNOWN; // Fallback
}

// Hilfsfunktion, um den Enum-Wert als String auszugeben (für Debugging)
String WeatherData::weatherConditionTypeToString(WeatherConditionType type) {
    switch (type) {
        // case WeatherConditionType::TYPE_UNSPECIFIED: return "TYPE_UNSPECIFIED";
        case WeatherConditionType::CLEAR: return "CLEAR";
        // case WeatherConditionType::MOSTLY_CLEAR: return "MOSTLY_CLEAR";
        case WeatherConditionType::PARTLY_CLOUDY: return "PARTLY_CLOUDY";
        // case WeatherConditionType::MOSTLY_CLOUDY: return "MOSTLY_CLOUDY";
        case WeatherConditionType::CLOUDY: return "CLOUDY";
        // case WeatherConditionType::WINDY: return "WINDY";
        // case WeatherConditionType::WIND_AND_RAIN: return "WIND_AND_RAIN";
        // case WeatherConditionType::LIGHT_RAIN_SHOWERS: return "LIGHT_RAIN_SHOWERS";
        // case WeatherConditionType::CHANCE_OF_SHOWERS: return "CHANCE_OF_SHOWERS";
        // case WeatherConditionType::SCATTERED_SHOWERS: return "SCATTERED_SHOWERS";
        // case WeatherConditionType::RAIN_SHOWERS: return "RAIN_SHOWERS";
        // case WeatherConditionType::HEAVY_RAIN_SHOWERS: return "HEAVY_RAIN_SHOWERS";
        // case WeatherConditionType::LIGHT_TO_MODERATE_RAIN: return "LIGHT_TO_MODERATE_RAIN";
        // case WeatherConditionType::MODERATE_TO_HEAVY_RAIN: return "MODERATE_TO_HEAVY_RAIN";
        case WeatherConditionType::RAIN: return "RAIN";
        // case WeatherConditionType::LIGHT_RAIN: return "LIGHT_RAIN";
        // case WeatherConditionType::HEAVY_RAIN: return "HEAVY_RAIN";
        // case WeatherConditionType::RAIN_PERIODICALLY_HEAVY: return "RAIN_PERIODICALLY_HEAVY";
        // case WeatherConditionType::LIGHT_SNOW_SHOWERS: return "LIGHT_SNOW_SHOWERS";
        // case WeatherConditionType::CHANCE_OF_SNOW_SHOWERS: return "CHANCE_OF_SNOW_SHOWERS";
        // case WeatherConditionType::SCATTERED_SNOW_SHOWERS: return "SCATTERED_SNOW_SHOWERS";
        // case WeatherConditionType::SNOW_SHOWERS: return "SNOW_SHOWERS";
        // case WeatherConditionType::HEAVY_SNOW_SHOWERS: return "HEAVY_SNOW_SHOWERS";
        // case WeatherConditionType::LIGHT_TO_MODERATE_SNOW: return "LIGHT_TO_MODERATE_SNOW";
        // case WeatherConditionType::MODERATE_TO_HEAVY_SNOW: return "MODERATE_TO_HEAVY_SNOW";
        case WeatherConditionType::SNOW: return "SNOW";
        // case WeatherConditionType::LIGHT_SNOW: return "LIGHT_SNOW";
        // case WeatherConditionType::HEAVY_SNOW: return "HEAVY_SNOW";
        // case WeatherConditionType::SNOWSTORM: return "SNOWSTORM";
        // case WeatherConditionType::SNOW_PERIODICALLY_HEAVY: return "SNOW_PERIODICALLY_HEAVY";
        // case WeatherConditionType::HEAVY_SNOW_STORM: return "HEAVY_SNOW_STORM";
        // case WeatherConditionType::BLOWING_SNOW: return "BLOWING_SNOW";
        // case WeatherConditionType::RAIN_AND_SNOW: return "RAIN_AND_SNOW";
        // case WeatherConditionType::HAIL: return "HAIL";
        // case WeatherConditionType::HAIL_SHOWERS: return "HAIL_SHOWERS";
        case WeatherConditionType::THUNDERSTORM: return "THUNDERSTORM";
        // case WeatherConditionType::THUNDERSHOWER: return "THUNDERSHOWER";
        // case WeatherConditionType::LIGHT_THUNDERSTORM_RAIN: return "LIGHT_THUNDERSTORM_RAIN";
        // case WeatherConditionType::SCATTERED_THUNDERSTORMS: return "SCATTERED_THUNDERSTORMS";
        // case WeatherConditionType::HEAVY_THUNDERSTORM: return "HEAVY_THUNDERSTORM";
        case WeatherConditionType::UNKNOWN: return "UNKNOWN"; // Fallback
    }
    return "UNKNOWN"; // Fallback für unbekannte Werte
}

String WeatherData::toString() {
    String s = "--- Weather Data ---";
           s += "\nTemperature: " + String(temperature.degrees) + String(temperature.unit);
           s += "\nRelative Humidity: " + String(relativeHumidity) + "%";
           s += "\nWeather Type: " + String(weatherConditionTypeToString(weatherType));
           s += "\n--- End Weather Data ---";
    return s;
}