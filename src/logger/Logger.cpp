#include "Logger.h" // Eigenen Header inkludieren
#include "../webservice/ntp/NTPTimeSync.h" // Hier ist die vollständige Definition von NTPTimeSync notwendig,
                                      // da NTPTimeSync* _staticTimeSync tatsächlich verwendet wird (Methodenaufruf).

// --- DEFINITION UND INITIALISIERUNG DER STATISCHEN MEMBER-VARIABLEN ---
NTPTimeSync* Logger::_staticTimeSync = nullptr;
LogLevel Logger::_outputLogLevel = LogLevel::Info; // Standardwert setzen, z.B. Info

// Implementierung der setup() Methode (ohne TimeSync)
void Logger::setup(LogLevel outputLevel) {
  static bool serialInitialized = false;
  if (!serialInitialized) {
    Serial.begin(9600);
    while (!Serial && millis() < 5000);
    serialInitialized = true;
  }
  _staticTimeSync = nullptr; // Sicherstellen, dass es initial nullptr ist
  Logger::setOutputLogLevel(outputLevel);
}

// Implementierung der setup() Methode (mit TimeSync)
void Logger::setup(LogLevel outputLevel, NTPTimeSync* timeSync) {
  Logger::setup(outputLevel); // Ruft die erste setup()-Methode auf, um Serial zu initialisieren (falls nötig)
  _staticTimeSync = timeSync; // Setzt den TimeSync-Zeiger
}

// Implementierung der setOutputLogLevel() Methode
void Logger::setOutputLogLevel(LogLevel level) {
  _outputLogLevel = level;
  // Hinweis: Hier wird bereits Logger::log aufgerufen.
  // Es ist wichtig, dass _outputLogLevel bereits korrekt gesetzt ist,
  // bevor diese Log-Nachricht tatsächlich geloggt wird.
  // Das sollte hier kein Problem sein.
  Logger::log(LogLevel::Info, "Ausgabe LogLevel auf: " + String(Logger::getLevelName(level)) + " gesetzt.");
}

// Implementierung der log() Methode
void Logger::log(LogLevel level, const String& message) {
  // Log Ausgabe nur machen, wenn <= eingestelltes Log Level.
  if (level > _outputLogLevel) {
    return; // Nachricht verwerfen
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

// Implementierung der getLevelName() Hilfsfunktion
const char* Logger::getLevelName(LogLevel level) {
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