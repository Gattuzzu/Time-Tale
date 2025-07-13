#include <Arduino.h>
#include "logger/Logger.h"
#include "logger/LogLevel.h"
#include "webservice/ntp/NTPTimeSync.h"
#include "webservice/api/weather/WeatherClient.h"
#include "webservice/api/weather/WeatherData.h"
#include "Secrets.h"
#include "Settings.h"

// Lokale Speicher
WeatherData currentWeatherData;

void setup() {
  // Logger initialisieren
  Logger::setup(LOG_LEVEL);
  Logger::log(LogLevel::Info, "Programm gestartet!");

  // NTPTimeSync initialiseren
  if (NTPTimeSync::getInstance(SECRET_SSID, SECRET_PASSWORD, NTP_SERVER, TIME_OFFSET, UPDATE_INTERVALL).begin()) {
    // Zeit an Logger weiter geben
    Logger::setup(LOG_LEVEL, &NTPTimeSync::getInstance());

    Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");

  } else {
    Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
  }

  // Weather API initialisieren
  WeatherClient::getInstance(WEATHER_API_SERVER, GOOGLE_ACCESS_TOKEN);

}

void loop() {
  // put your main code here, to run repeatedly:

  // Wetter Daten abfragen
  if (WeatherClient::getInstance().getCurrentConditions(LATITUDE, LONGITUDE, currentWeatherData)) {
    Logger::log(LogLevel::Info, "Wetterdaten erfolgreich abgerufen.");
    // currentWeatherData.printToSerial();
  } else {
    Logger::log(LogLevel::Error, "Fehler beim Abrufen der Wetterdaten.");
  }
}
