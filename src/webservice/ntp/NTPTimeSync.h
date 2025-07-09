#ifndef NTP_TIME_SYNC_H
#define NTP_TIME_SYNC_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "../../logger/LogLevel.h" // LogLevel.h muss hier bleiben, da es im Public-Interface verwendet wird

// --- FORWARD DECLARATION für die Logger-Klasse ---
// Da NTPTimeSync in seiner Deklaration nur Logger::log aufruft (statische Methode),
// aber kein Logger-Objekt als Member hat, reicht eine Forward Declaration.
// Die vollständige Definition von Logger wird erst in der .cpp-Datei benötigt.
class Logger;

class NTPTimeSync {
public:
  // Statische Methode, um die einzige Instanz von NTPTimeSync zu erhalten.
  // Die Konfigurationsparameter werden hier beim ERSTEN Aufruf übergeben.
  // WICHTIG: Die statische 'instance' wird hier im Header definiert,
  // da sie in einer Inline-Funktion (getInstance) verwendet wird.
  static NTPTimeSync& getInstance(const char* ssid = nullptr, const char* password = nullptr,
                                  const char* ntpServer = nullptr, long timeOffset = 0, long updateInterval = 0) {
    static NTPTimeSync instance(ssid, password, ntpServer, timeOffset, updateInterval);
    return instance;
  }

  // Methode zur Initialisierung der WLAN-Verbindung und NTP
  // Return: true  = Verbindung konnte aufgebaut werden
  //         false = Verbindungsaufbau fehlgeschlagen
  bool begin(); // Nur Deklaration

  // Methode, die regelmäßig aufgerufen werden sollte, um die Zeit zu aktualisieren
  void update(); // Nur Deklaration

  // Gibt die formatierte Uhrzeit (HH:MM:SS) zurück
  String getFormattedTime(); // Nur Deklaration

  // Gibt den Unix-Timestamp (Sekunden seit 1. Januar 1970) zurück
  time_t getEpochTime(); // Nur Deklaration

private:
  // Privater Konstruktor, um direkte Instanziierung zu verhindern
  // Wird nur von getInstance() aufgerufen.
  NTPTimeSync(const char* ssid, const char* password, const char* ntpServer, long timeOffset, long updateInterval);

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