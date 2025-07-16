#ifndef CONFIGURATION_PORTAL_H
#define CONFIGURATION_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "../../logger/Logger.h"
#include "../../logger/LogLevel.h"

// Standard-Port für den Webserver
const int HTTP_PORT = 80;

// Forward Declaration für Logger
class Logger;

class ConfigurationPortal {
public:
    // Statische Methode, um die einzige Instanz zu erhalten (Singleton)
    static ConfigurationPortal& getInstance() {
        static ConfigurationPortal instance;
        return instance;
    }

    // Startet den Access Point und den Webserver
    bool begin(const char* ssid, const char* password = nullptr);

    // Muss in der loop() aufgerufen werden, um Client-Anfragen zu verarbeiten
    void handleClient();

    // Setzt Callback-Funktionen für erfolgreiche Speicherung der Konfiguration
    // Dies ist der Punkt, an dem dein Hauptprogramm benachrichtigt wird,
    // dass neue WLAN-Daten vorliegen.
    void onConfigSaved(void (*callback)(const String& ssid, const String& password));

    // Lädt gespeicherte WLAN-Daten aus NVS
    bool loadWiFiConfig(String& ssid, String& password);

    // Speichert WLAN-Daten in NVS
    bool saveWiFiConfig(const String& ssid, const String& password);

private:
    // Privater Konstruktor, um Singleton-Muster zu erzwingen
    ConfigurationPortal();

    // Kopierkonstruktor und Zuweisungsoperator löschen
    ConfigurationPortal(const ConfigurationPortal&) = delete;
    ConfigurationPortal& operator=(const ConfigurationPortal&) = delete;

    WebServer _server;
    void (*_configSavedCallback)(const String& ssid, const String& password);
    Preferences _preferences;

    // --- Webserver-Handler-Methoden ---
    void handleRoot();       // Handler für die Startseite ("/")
    void handleSaveConfig(); // Handler für die POST-Anfrage zum Speichern der Konfig.
    void handleNotFound();   // Handler für nicht gefundene Seiten
};

#endif // CONFIGURATION_PORTAL_H