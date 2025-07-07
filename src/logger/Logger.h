#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <TimeLib.h>         // Für Datumsumrechnung aus Epoch-Zeit
#include "LogLevel.h"
#include "../webservice/NTPTimeSync.h"

class Logger {
public:
  // Setup Methode bevor das NTPTimeSync vorhanden ist
  static void setup(LogLevel outputLevel) {
    static bool serialInitialized = false;
    if (!serialInitialized) {
      Serial.begin(9600);
      while (!Serial && millis() < 5000);
      serialInitialized = true;
    }
    // _staticTimeSync bleibt hier (initial) nullptr
    // Standard-LogLevel setzen
    setOutputLogLevel(outputLevel); 
  }

  // Setup Methode nachdem das NTPTimeSync vorhanden ist
  static void setup(LogLevel outputLevel, NTPTimeSync* timeSync) {
    Logger::setup(outputLevel); // Ruft die erste setup()-Methode auf, um Serial zu initialisieren (falls nötig)
    _staticTimeSync = timeSync; // Setzt den TimeSync-Zeiger
  }

  // NEU: Methode zum Setzen des maximalen LogLevels für die Ausgabe
  static void setOutputLogLevel(LogLevel level) {
    _outputLogLevel = level;
    Logger::log(LogLevel::Info, "Ausgabe LogLevel auf: " + String(getLevelName(level)) + " gesetzt.");
  }

  // Statische Log-Methode
  static void log(LogLevel level, const String& message) {
    // Log Ausgabe nur machen, wenn <= eingestelltes Log Level.
    if (level > _outputLogLevel) {
      return; // Nachricht verwerfen, da ihr Level zu niedrig (weniger kritisch) ist, um ausgegeben zu werden
    }

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
      Serial.print("[NO TIME] - ");
    }

    Serial.print(Logger::getLevelName(level));
    Serial.print(" : ");
    Serial.println(message);
  }

private:
  // Statische Member-Variable (kein Objekt wird erstellt)
  static NTPTimeSync* _staticTimeSync;

  // NEU: Statische Member-Variable für das globale Ausgabeloglevel
  static LogLevel _outputLogLevel;

  // Statische Hilfsfunktion
  static const char* getLevelName(LogLevel level) {
    switch (level) {
      case Error:
        return "Error";
      case Info:
        return "Info";
      case Debug:
        return "Debug";
      default:
        return "UNKNOWN";
    }
  }
};

// --- WICHTIG: Statische Member-Variablen AUßERHALB der Klasse definieren und initialisieren ---
NTPTimeSync* Logger::_staticTimeSync = nullptr;
LogLevel Logger::_outputLogLevel = LogLevel::Info; // Standardwert setzen

#endif