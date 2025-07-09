#include "WeatherData.h"

WeatherData::WeatherData() {
    reset(); // Initialisiere alle Werte auf Standard
}

void WeatherData::reset() {
    relativeHumidity = 0;
    uvIndex = 0;
    thunderstormProbability = 0;
    cloudCover = 0;

    dewPoint.degrees = 0; dewPoint.unit = "";
    heatIndex.degrees = 0; heatIndex.unit = "";
    windChill.degrees = 0; windChill.unit = "";

    precipitation.probability.percent = 0; precipitation.probability.type = "";
    precipitation.snowQpf.quantity = 0; precipitation.snowQpf.unit = "";
    precipitation.qpf.quantity = 0; precipitation.qpf.unit = "";

    airPressure.meanSeaLevelMillibars = 0;

    wind.direction.degrees = 0; wind.direction.cardinal = "";
    wind.speed.value = 0; wind.speed.unit = "";
    wind.gust.value = 0; wind.gust.unit = "";

    visibility.distance = 0; visibility.unit = "";

    currentConditionsHistory.temperatureChange.degrees = 0; currentConditionsHistory.temperatureChange.unit = "";
    currentConditionsHistory.maxTemperature.degrees = 0; currentConditionsHistory.maxTemperature.unit = "";
    currentConditionsHistory.minTemperature.degrees = 0; currentConditionsHistory.minTemperature.unit = "";
    currentConditionsHistory.snowQpf.quantity = 0; currentConditionsHistory.snowQpf.unit = "";
    currentConditionsHistory.qpf.quantity = 0; currentConditionsHistory.qpf.unit = "";
}

// void WeatherData::printToSerial() {
//     Serial.println("--- Weather Data ---");
//     Serial.print("Relative Humidity: "); Serial.println(relativeHumidity);
//     Serial.print("UV Index: "); Serial.println(uvIndex);
//     Serial.print("Cloud Cover: "); Serial.println(cloudCover);
//     Serial.print("Dew Point: "); Serial.print(dewPoint.degrees); Serial.println(dewPoint.unit);
//     Serial.print("Heat Index: "); Serial.print(heatIndex.degrees); Serial.println(heatIndex.unit);
//     Serial.print("Wind Chill: "); Serial.print(windChill.degrees); Serial.println(windChill.unit);
//     Serial.print("Air Pressure (MSL): "); Serial.print(airPressure.meanSeaLevelMillibars); Serial.println(F(" mb"));

//     Serial.print("Wind Direction: "); Serial.print(wind.direction.degrees); Serial.print(F(" deg (")); Serial.print(wind.direction.cardinal); Serial.println(F(")"));
//     Serial.print("Wind Speed: "); Serial.print(wind.speed.value); Serial.println(wind.speed.unit);
//     Serial.print("Wind Gust: "); Serial.print(wind.gust.value); Serial.println(wind.gust.unit);

//     Serial.print("Visibility: "); Serial.print(visibility.distance); Serial.println(visibility.unit);

//     Serial.print("Precipitation Probability: "); Serial.print(precipitation.probability.percent); Serial.println(F("%"));
//     Serial.print("Precipitation QPF: "); Serial.print(precipitation.qpf.quantity); Serial.println(precipitation.qpf.unit);
//     Serial.print("Snow QPF: "); Serial.print(precipitation.snowQpf.quantity); Serial.println(precipitation.snowQpf.unit);

//     Serial.println("--- End Weather Data ---");
// }