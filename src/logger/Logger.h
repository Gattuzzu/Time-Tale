#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <TimeLib.h>         // Für Datumsumrechnung aus Epoch-Zeit
#include "LogLevel.h"
#include "../webservice/NTPTimeSync.h" // Die NTPTimeSync-Klasse inkludieren

class Logger {
public:
  // Setup Methode bevor das NTPTimeSync vorhanden ist
  static void setup() {
    static bool serialInitialized = false;
    if (!serialInitialized) {
      Serial.begin(9600);
      while (!Serial && millis() < 5000);
      serialInitialized = true;
    }
    // _staticTimeSync bleibt hier (initial) nullptr
  }

  // Setup Methode nachdem das NTPTimeSync vorhanden ist
  static void setup(NTPTimeSync* timeSync) {
    // Ruft die erste begin()-Methode auf, um Serial zu initialisieren (falls nötig)
    Logger::setup();
    _staticTimeSync = timeSync; // Setzt den TimeSync-Zeiger
  }

  // Statische Log-Methode
  static void log(LogLevel level, const String& message) {
    if (_staticTimeSync != nullptr) {
      _staticTimeSync->update(); // Zeit aktualisieren
      time_t epochTime = _staticTimeSync->getEpochTime();
      setTime(epochTime); // TimeLib's Zeit setzen

      String dateString = "";
      dateString += String(day());
      dateString += ".";
      dateString += String(month());
      dateString += ".";
      dateString += String(year());

      String formattedTime = _staticTimeSync->getFormattedTime();

      Serial.print(dateString);
      Serial.print(" ");
      Serial.print(formattedTime);
      Serial.print(" - ");
    } else {
      Serial.print("[NO TIME] - "); // Klarere Meldung
    }

    Serial.print(Logger::getLevelName(level));
    Serial.print(" : ");
    Serial.println(message);
  }

private:
  // Statische Member-Variable (kein Objekt wird erstellt)
  static NTPTimeSync* _staticTimeSync;

  // Statische Hilfsfunktion
  static const char* getLevelName(LogLevel level) {
    switch (level) {
      case Info:
        return "Info";
      case Error:
        return "Error";
      case Debug:
        return "Debug";
      default:
        return "UNKNOWN";
    }
  }
};

// --- WICHTIG: Statische Member-Variable außerhalb der Klasse definieren ---
NTPTimeSync* Logger::_staticTimeSync = nullptr;

#endif // LOGGER_H