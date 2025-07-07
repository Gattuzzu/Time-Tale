#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <TimeLib.h>         // Für Datumsumrechnung aus Epoch-Zeit
#include "LogLevel.h"        // Dein Enum LogLevel

// --- FORWARD DECLARATION für die NTPTimeSync-Klasse ---
// Da Logger nur einen Zeiger auf NTPTimeSync speichert und statische Methoden aufruft,
// reicht eine Forward Declaration aus, um zirkuläre Abhängigkeiten zu vermeiden.
class NTPTimeSync;

class Logger {
public:
  // Setup Methode bevor das NTPTimeSync vorhanden ist
  static void setup(LogLevel outputLevel);

  // Setup Methode nachdem das NTPTimeSync vorhanden ist
  static void setup(LogLevel outputLevel, NTPTimeSync* timeSync);

  // Methode zum Setzen des maximalen LogLevels für die Ausgabe
  static void setOutputLogLevel(LogLevel level);

  // Statische Log-Methode
  static void log(LogLevel level, const String& message);

private:
  // Statische Member-Variable (kein Objekt wird erstellt)
  static NTPTimeSync* _staticTimeSync;

  // Statische Member-Variable für das globale Ausgabeloglevel
  static LogLevel _outputLogLevel;

  // Statische Hilfsfunktion
  static const char* getLevelName(LogLevel level);
};

// --- WICHTIG: Statische Member-Variablen NICHT HIER DEFINIEREN ODER INITIALISIEREN ---
// Das passiert in der Logger.cpp Datei!

#endif // LOGGER_H