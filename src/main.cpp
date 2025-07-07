#include <Arduino.h>
#include "logger/Logger.h"
#include "logger/LogLevel.h"
#include "webservice/NTPTimeSync.h"
#include "Secrets.h"

// NTP-Konfiguration
const char* ntpServer = "ntp.metas.ch";
const long timeOffset = 2 * 3600; // MESZ (Sommerzeit) = +2 Stunden | MEZ (Winterzeit) = +1 Stunde 
const long updateInterval = 60000; // 1 Minute

void setup() {
  // Logger Initialisieren
  Logger::setup(LogLevel::Info);
  Logger::log(LogLevel::Info, "Programm gestartet!");

  // NTPTimeSync Initialiseren
  if (NTPTimeSync::getInstance(SECRET_SSID, SECRET_PASSWORD, ntpServer, timeOffset, updateInterval).begin()) {
    Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");
  } else {
    Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
}
