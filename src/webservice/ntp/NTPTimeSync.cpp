#include "NTPTimeSync.h"
#include "../../logger/Logger.h"

// Implementierung der begin() Methode
// Diese Methode nimmt KEINE WiFiUDP Instanz mehr entgegen.
bool NTPTimeSync::begin() {
  // Überprüfen, ob WLAN bereits verbunden ist
  if (WiFi.status() != WL_CONNECTED) {
      Logger::log(LogLevel::Error, "NTPTimeSync::begin(): WLAN ist nicht verbunden. Bitte stellen Sie zuerst eine WLAN-Verbindung her.");
      return false;
  }

  // Der NTPClient kümmert sich um das `begin()` des zugrunde liegenden UDP-Sockets.
  _NtpClient.begin(); // Dies sollte den UDP-Port für NTP starten

  Logger::log(LogLevel::Info, "Synchronisiere Zeit mit NTP...");
  // Erster Update-Versuch beim Start
  bool updateSuccess = _NtpClient.update();
  if (!updateSuccess) {
    int attempts = 0;
    while(!updateSuccess && attempts < 10) { // Max. 10 Versuche
      delay(500);
      Logger::log(LogLevel::Info, ".");
      updateSuccess = _NtpClient.update();
      attempts++;
    }
  }

  if (updateSuccess) {
    Logger::log(LogLevel::Info, "Zeit synchronisiert.");
    return true;
  } else {
    Logger::log(LogLevel::Error, "NTPTimeSync: Zeit konnte nicht synchronisiert werden. NTP-Server oder Netzwerkproblem.");
    return false;
  }
}

// Implementierung der update() Methode (unverändert)
void NTPTimeSync::update() {
  if (WiFi.status() == WL_CONNECTED) {
    _NtpClient.update();
  } else {
    Logger::log(LogLevel::Error, "NTPTimeSync: WLAN nicht verbunden, Zeit-Update übersprungen.");
  }
}

// Implementierung der getFormattedTime() Methode (unverändert)
String NTPTimeSync::getFormattedTime() {
  return _NtpClient.getFormattedTime();
}

// Implementierung der getEpochTime() Methode (unverändert)
time_t NTPTimeSync::getEpochTime() {
  return _NtpClient.getEpochTime();
}

// Implementierung des privaten Konstruktors
NTPTimeSync::NTPTimeSync(const char* ntpServer, long timeOffset, long updateInterval)
  // Hier wird _NtpClient DIREKT mit der privaten _internalNtpUDP initialisiert.
  : _ntpServer(ntpServer), _timeOffset(timeOffset), _updateInterval(updateInterval),
    _NtpClient(_internalNtpUDP, ntpServer, timeOffset, updateInterval) {
  // Der Konstruktor kann leer bleiben.
}