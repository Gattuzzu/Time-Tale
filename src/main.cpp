#include <Arduino.h>
#include "logger/Logger.h"
#include "logger/LogLevel.h"
#include "webservice/ntp/NTPTimeSync.h"
#include "webservice/api/weather/WeatherClient.h"
#include "webservice/api/weather/WeatherData.h"
#include "Secrets.h"

// NTP Konfiguration
const char* ntpServer = "ntp.metas.ch";
const long timeOffset = 2 * 3600; // MESZ (Sommerzeit) = +2 Stunden | MEZ (Winterzeit) = +1 Stunde 
const long updateInterval = 60000; // 1 Minute

// Logger Konfiguration
const LogLevel logLevel = LogLevel::Info;

// API Konfiguration
const char* weatherApiServer = "weather.googleapis.com";
const float latitude = 46.774;
const float longitude = 7.640;


// Lokale Speicher
WeatherData currentWeatherData;

void setup() {
  // Logger initialisieren
  Logger::setup(logLevel);
  Logger::log(LogLevel::Info, "Programm gestartet!");

  // NTPTimeSync initialiseren
  if (NTPTimeSync::getInstance(SECRET_SSID, SECRET_PASSWORD, ntpServer, timeOffset, updateInterval).begin()) {
    // Zeit an Logger weiter geben
    Logger::setup(logLevel, &NTPTimeSync::getInstance());

    Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");

  } else {
    Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
  }

  // Weather API initialisieren
  WeatherClient::getInstance(weatherApiServer, GOOGLE_ACCESS_TOKEN);

}

void loop() {
  // put your main code here, to run repeatedly:

  // Wetter Daten abfragen
  if (WeatherClient::getInstance().getCurrentConditions(latitude, longitude, currentWeatherData)) {
    Logger::log(LogLevel::Info, "Wetterdaten erfolgreich abgerufen.");
    // currentWeatherData.printToSerial();
  } else {
    Logger::log(LogLevel::Error, "Fehler beim Abrufen der Wetterdaten.");
  }
}
