#include "NTPTimeSync.h" // Inkludiere die eigene Header-Datei
#include "../../logger/Logger.h" // <-- HIER MUSS Logger.h VOLLSTÄNDIG INKLUDIERT WERDEN,
                              // da Logger::log tatsächlich aufgerufen wird und der Compiler
                              // die vollständige Definition von Logger benötigt.

// Implementierung der begin() Methode
bool NTPTimeSync::begin() {
  // Überprüfen, ob die Konfigurationsdaten gesetzt sind
  if (_ssid == nullptr || _password == nullptr || _ntpServer == nullptr || _updateInterval == 0) {
      Logger::log(LogLevel::Error, "NTPTimeSync::begin() aufgerufen ohne vollständige Konfiguration. Bitte getInstance mit allen Parametern aufrufen.");
      return false;
  }

  Logger::log(LogLevel::Info, "Verbinde mit WLAN...");
  WiFi.begin(_ssid, _password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Logger::log(LogLevel::Info, ".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Logger::log(LogLevel::Info, "Verbunden mit WLAN!");
    Logger::log(LogLevel::Info, "IP-Adresse: " + WiFi.localIP().toString());

    _timeClient.begin();
    Logger::log(LogLevel::Info, "Synchronisiere Zeit mit NTP...");
    while(!_timeClient.update()) {
      delay(500);
      Logger::log(LogLevel::Info, ".");
    }
    Logger::log(LogLevel::Info, "Zeit synchronisiert.");
    return true;
  } else {
    Logger::log(LogLevel::Error, "Verbindung fehlgeschlagen! Bitte WLAN-Daten prüfen.");
    return false;
  }
}

// Implementierung der update() Methode
void NTPTimeSync::update() {
  _timeClient.update();
}

// Implementierung der getFormattedTime() Methode
String NTPTimeSync::getFormattedTime() {
  return _timeClient.getFormattedTime();
}

// Implementierung der getEpochTime() Methode
time_t NTPTimeSync::getEpochTime() {
  return _timeClient.getEpochTime();
}

// Implementierung des privaten Konstruktors
NTPTimeSync::NTPTimeSync(const char* ssid, const char* password, const char* ntpServer, long timeOffset, long updateInterval)
  : _ssid(ssid), _password(password), _ntpServer(ntpServer), _timeOffset(timeOffset), _updateInterval(updateInterval),
    _timeClient(_ntpUDP, ntpServer, timeOffset, updateInterval) {
  // Der Konstruktor kann leer bleiben, da alle Member in der Initialisierungsliste gesetzt werden.
}