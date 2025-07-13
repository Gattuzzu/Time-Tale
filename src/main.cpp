#include <Arduino.h>
#include <WiFi.h>
#include "logger/Logger.h"
#include "logger/LogLevel.h"
#include "webservice/ntp/NTPTimeSync.h"
#include "webservice/api/weather/WeatherClient.h"
#include "webservice/api/weather/WeatherData.h"
#include "Secrets.h"
#include "Settings.h"
#include "webservice/api/pollen/PollenClient.h"

// Lokale Speicher
WeatherData currentWeatherData;
PollenData currentPollenData;

void setup() {
  // Logger initialisieren
  Logger::setup(LOG_LEVEL);
  Logger::log(LogLevel::Info, "Programm gestartet!");

  // W-Lan Verbindung versuchen aufzubauen
  WiFi.mode(WIFI_AP_STA); // (Access Point + Station / Teilnehmer) 
  WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Logger::log(LogLevel::Info, ".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED){
    Logger::log(LogLevel::Info, "Verbunden mit WLAN!");
    Logger::log(LogLevel::Info, "IP-Adresse: " + WiFi.localIP().toString());

  } else {
    Logger::log(LogLevel::Error, "Verbindung fehlgeschlagen! Bitte WLAN-Daten prüfen.");
    return;
  }

  // NTPTimeSync initialiseren
  if (NTPTimeSync::getInstance(NTP_SERVER, TIME_OFFSET, UPDATE_INTERVALL).begin()) {
    // Zeit an Logger weiter geben
    Logger::setup(LOG_LEVEL, &NTPTimeSync::getInstance());

    Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");

  } else {
    Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
  }

  // Weather API initialisieren
  WeatherClient::getInstance(WEATHER_API_SERVER, GOOGLE_ACCESS_TOKEN);

  // Pollen API initialisieren
  PollenClient::getInstance(POLLEN_API_SERVER, GOOGLE_ACCESS_TOKEN);

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

  // Pollen Daten abfragen
  if (PollenClient::getInstance().getCurrentPollen(LATITUDE, LONGITUDE, currentPollenData)) {
    Logger::log(LogLevel::Info, "Wetterdaten erfolgreich abgerufen.");
    // currentWeatherData.printToSerial();
  } else {
    Logger::log(LogLevel::Error, "Fehler beim Abrufen der Pollendaten.");
  }
}
