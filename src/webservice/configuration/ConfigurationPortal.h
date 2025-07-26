#ifndef CONFIGURATION_PORTAL_H
#define CONFIGURATION_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "../../logger/Logger.h" // Pfad zum Logger, bitte bei Bedarf anpassen
#include "../../logger/LogLevel.h" // Pfad zum LogLevel, bitte bei Bedarf anpassen

// Standard-Port für den Webserver
const int HTTP_PORT = 80;

// Forward Declaration für Logger
class Logger;

// Struktur zur Speicherung aller Konfigurationsdaten
// Diese Struktur enthält alle Einstellungen, die über das Webportal konfiguriert werden können.
struct AppConfig {
    String wifiSsid;                  // WLAN SSID
    String wifiPassword;              // WLAN Passwort
    int timeOffsetHours;              // Zeitverschiebung in Stunden (z.B. 1 für MEZ, 2 für MESZ)
    String ntpServer;                 // NTP Server Adresse als String
    String googleAccessToken;         // Access Token für API von Google API's
    int weatherUpdateIntervalMin;     // Update Intervall für das Wetter-API in Minuten
    int pollenUpdateIntervalMin;      // Update Intervall für das Pollen-API in Minuten
    float longitude;                  // Längengrad als Float
    float latitude;                   // Breitengrad als Float
    int indoorTempDisplayTimeSec;     // Anzeigedauer der Innentemperatur in Sekunden
    int outdoorTempDisplayTimeSec;    // Anzeigedauer der Aussentemperatur in Sekunden
    uint32_t textColorCRGB;           // Farbe des Textes für die LED-Anzeige (als 0xRRGGBB Hex-Wert)
    int ledBrightness;                // Helligkeit der LED-Anzeige (0-100)
    int volume;                       // Lautstärke in Schritten von 0-30 (0 = Mute)
};

class ConfigurationPortal {
public:
    // Statische Methode, um die einzige Instanz zu erhalten (Singleton-Muster).
    // Dies stellt sicher, dass es nur eine Instanz des Konfigurationsportals gibt.
    static ConfigurationPortal& getInstance() {
        static ConfigurationPortal instance;
        return instance;
    }

    // Startet den Access Point und den Webserver für die Erstkonfiguration.
    // Wird aufgerufen, wenn keine WLAN-Zugangsdaten gespeichert sind oder die Verbindung fehlschlägt.
    bool startAPAndWebServer(const char* apSsid, const char* apPassword = nullptr);

    // Startet den Webserver im Station-Modus, nachdem WLAN erfolgreich verbunden ist.
    // Ermöglicht die Änderung weiterer Einstellungen über die zugewiesene IP-Adresse.
    bool startWebServerInStationMode();

    // Muss regelmässig in der Arduino loop() Funktion aufgerufen werden,
    // um eingehende Client-Anfragen an den Webserver zu verarbeiten.
    void handleClient();

    // Setzt eine Callback-Funktion, die aufgerufen wird, wenn die Konfiguration
    // erfolgreich gespeichert wurde. Die aktualisierte AppConfig wird übergeben.
    void onConfigSaved(void (*callback)(const AppConfig& config));

    // Lädt alle gespeicherten Konfigurationsdaten aus dem NVS-Speicher (Non-Volatile Storage)
    // in die übergebene AppConfig-Struktur.
    bool loadConfig(AppConfig& config);

    // Speichert alle Konfigurationsdaten aus der übergebenen AppConfig-Struktur
    // in den NVS-Speicher.
    bool saveConfig(const AppConfig& config);

private:
    // Privater Konstruktor, um das Singleton-Muster zu erzwingen.
    // Die Instanz kann nur über getInstance() erstellt werden.
    ConfigurationPortal();

    // Kopierkonstruktor und Zuweisungsoperator sind gelöscht,
    // um das Kopieren der Singleton-Instanz zu verhindern.
    ConfigurationPortal(const ConfigurationPortal&) = delete;
    ConfigurationPortal& operator=(const ConfigurationPortal&) = delete;

    WebServer _server;                                    // Instanz des Webservers
    void (*_configSavedCallback)(const AppConfig& config); // Pointer zur Callback-Funktion
    Preferences _preferences;                             // Instanz für den NVS-Zugriff

    // --- Webserver-Handler-Methoden ---
    void handleRoot();       // Handler für die Startseite ("/") des Webservers
    void handleSaveConfig(); // Handler für die POST-Anfrage zum Speichern der Konfiguration
    void handleNotFound();   // Handler für nicht gefundene Seiten (HTTP 404)

    // Hilfsfunktion zum Einrichten der Webserver-Routen,
    // wird von startAPAndWebServer und startWebServerInStationMode aufgerufen.
    void setupWebServerRoutes();
};

#endif // CONFIGURATION_PORTAL_H
