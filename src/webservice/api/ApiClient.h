#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "../../logger/Logger.h" // Angenommener Pfad zum Logger
#include "../../logger/LogLevel.h"

// Basisklasse für alle API-Clients
class ApiClient {
public:
    // Virtual Destructor ist WICHTIG für Polymorphie!
    virtual ~ApiClient() = default;

    // Methode zum Initialisieren des Clients (z.B. Setzen von Host und API Key)
    // Wird von den getInstance-Methoden der abgeleiteten Klassen aufgerufen.
    void configure(const char* host, const char* apiKey);

protected:
    // Konstruktor ist protected, damit er nur von abgeleiteten Klassen aufgerufen werden kann
    ApiClient();

    // Private Kopierkonstruktor und Zuweisungsoperator verhindern Kopien
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    const char* _host;
    const char* _apiKey;
    WiFiClientSecure _client; // Für HTTPS-Verbindungen

    // Gemeinsame Methode zum Senden einer GET-Anfrage und Empfangen der JSON-Antwort
    // Gibt true bei Erfolg zurück, füllt JsonDocument
    bool sendGetRequest(const String& path, JsonDocument& doc);

    // Virtuelle Methode zum Parsen des JSON. Muss von Unterklassen implementiert werden.
    // virtual bool parseJson(JsonDocument& doc) = 0; // Macht ApiClient abstrakt
    // Da parseJson oft datenspezifisch ist, lassen wir sie hier noch nicht abstrakt.
    // Stattdessen wird sendGetRequest das Parsing zurückgeben und die Unterklasse
    // kümmert sich dann um die datenspezifische Verarbeitung.
    // Die Unterklasse ruft dann ihre eigene parseJson (oder ähnlich benannte) Methode auf.
};

#endif // API_CLIENT_H