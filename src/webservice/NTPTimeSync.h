#ifndef NTP_TIME_SYNC_H
#define NTP_TIME_SYNC_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "../logger/Logger.h"
#include "../logger/LogLevel.h"

class NTPTimeSync {
public:
  // Statische Methode, um die einzige Instanz von NTPTimeSync zu erhalten.
  // Die Konfigurationsparameter werden hier beim ERSTEN Aufruf übergeben.
  static NTPTimeSync& getInstance(const char* ssid = nullptr, const char* password = nullptr,
                                  const char* ntpServer = nullptr, long timeOffset = 0, long updateInterval = 0) {
    // Statische Variable, die die einzige Instanz hält.
    // Sie wird nur einmal initialisiert, wenn getInstance zum ersten Mal aufgerufen wird.
    static NTPTimeSync instance(ssid, password, ntpServer, timeOffset, updateInterval);
    return instance;
  }

  // Methode zur Initialisierung der WLAN-Verbindung und NTP
  // Return: true  = Verbindung konnte aufgebaut werden
  //         false = Verbindungsaufbau fehlgeschlagen
  bool begin() {
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

  // Methode, die regelmäßig aufgerufen werden sollte, um die Zeit zu aktualisieren
  void update() {
    _timeClient.update();
  }

  // Gibt die formatierte Uhrzeit (HH:MM:SS) zurück
  String getFormattedTime() {
    return _timeClient.getFormattedTime();
  }

  // Gibt den Unix-Timestamp (Sekunden seit 1. Januar 1970) zurück
  time_t getEpochTime() {
    return _timeClient.getEpochTime();
  }

private:
  // Privater Konstruktor, um direkte Instanziierung zu verhindern
  // Wird nur von getInstance() aufgerufen.
  NTPTimeSync(const char* ssid, const char* password, const char* ntpServer, long timeOffset, long updateInterval)
    : _ssid(ssid), _password(password), _ntpServer(ntpServer), _timeOffset(timeOffset), _updateInterval(updateInterval),
      _timeClient(_ntpUDP, ntpServer, timeOffset, updateInterval) {
  }

  // Private Kopierkonstruktor und Zuweisungsoperator verhindern Kopien
  NTPTimeSync(const NTPTimeSync&) = delete;
  NTPTimeSync& operator=(const NTPTimeSync&) = delete;

  const char* _ssid;
  const char* _password;
  const char* _ntpServer;
  long _timeOffset;
  long _updateInterval;
  WiFiUDP _ntpUDP;
  NTPClient _timeClient;
};

#endif // NTP_TIME_SYNC_H