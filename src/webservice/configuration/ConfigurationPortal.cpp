#include "ConfigurationPortal.h"
#include "../../logger/Logger.h"

const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Konfiguration</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f0f0f0; }
        .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); display: inline-block; }
        h1 { color: #333; }
        label { display: block; margin-bottom: 5px; text-align: left; font-weight: bold; }
        input[type="text"], input[type="password"] {
            width: calc(100% - 22px);
            padding: 10px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        input[type="submit"] {
            background-color: #007bff;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        input[type="submit"]:hover {
            background-color: #0056b3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WLAN-Konfiguration</h1>
        <form action="/save" method="POST">
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid" required><br>
            <label for="password">Passwort:</label>
            <input type="password" id="password" name="password"><br>
            <input type="submit" value="Verbinden und Speichern">
        </form>
    </div>
</body>
</html>
)rawliteral";

// NVS-Namespace und Keys
#define NVS_NAMESPACE "wifi_config"
#define NVS_KEY_SSID "ssid"
#define NVS_KEY_PASSWORD "password"


ConfigurationPortal::ConfigurationPortal() : _server(HTTP_PORT), _configSavedCallback(nullptr) {
    // Der Preferences-Namespace wird hier geöffnet.
    // "false" bedeutet Read/Write-Modus.
    _preferences.begin(NVS_NAMESPACE, false);
}

// Startet den Access Point und den Webserver
bool ConfigurationPortal::begin(const char* ssid, const char* password) {
    Logger::log(LogLevel::Info, "Starte Konfigurations-AP mit SSID: " + String(ssid));

    // Aktiviere den AP-Modus
    WiFi.mode(WIFI_AP);
    // Starte den Access Point
    if (!WiFi.softAP(ssid, password)) {
        Logger::log(LogLevel::Error, "Fehler beim Starten des Access Points!");
        return false;
    }

    // Gib die IP-Adresse des AP aus
    IPAddress apIP = WiFi.softAPIP();
    Logger::log(LogLevel::Info, "AP gestartet, IP-Adresse: " + apIP.toString());

    // --- Webserver-Handler einrichten ---
    // Lambda-Funktionen sind hier sehr praktisch, um Member-Funktionen als Handler zu verwenden.
    // 'this' wird capturet, um auf _server und andere Member zugreifen zu können.
    _server.on("/", HTTP_GET, [this]() {
        handleRoot();
    });

    _server.on("/save", HTTP_POST, [this]() {
        handleSaveConfig();
    });

    _server.onNotFound([this]() {
        handleNotFound();
    });

    _server.begin(); // Webserver starten
    Logger::log(LogLevel::Info, "Webserver auf Port " + String(HTTP_PORT) + " gestartet.");

    return true;
}

// Setzt Callback-Funktion
void ConfigurationPortal::onConfigSaved(void (*callback)(const String& ssid, const String& password)) {
    _configSavedCallback = callback;
}

// Handler für die Startseite
void ConfigurationPortal::handleRoot() {
    _server.send(200, "text/html", INDEX_HTML);
}

// Handler für die POST-Anfrage
void ConfigurationPortal::handleSaveConfig() {
    String ssid = "";
    String password = "";

    // Prüfen, ob die Parameter in der Anfrage vorhanden sind
    if (_server.hasArg("ssid")) {
        ssid = _server.arg("ssid");
    }
    if (_server.hasArg("password")) {
        password = _server.arg("password");
    }

    Logger::log(LogLevel::Info, "Empfangene WLAN-Daten - SSID: " + ssid + ", Passwort-Länge: " + String(password.length()));

    // Hier könntest du die Daten validieren, bevor du sie speicherst
    if (ssid.length() > 0) {
        // Speichere die WLAN-Daten in NVS
        if (saveWiFiConfig(ssid, password)) {
            Logger::log(LogLevel::Info, "WLAN-Konfiguration erfolgreich in NVS gespeichert.");
        } else {
            Logger::log(LogLevel::Error, "Fehler beim Speichern der WLAN-Konfiguration in NVS.");
        }

        if (_configSavedCallback) {
            _configSavedCallback(ssid, password);
        }

        // Bestätigungsseite senden
        _server.send(200, "text/plain", "Konfiguration erfolgreich gespeichert! Das Gerät versucht nun, sich mit dem WLAN zu verbinden.");
        Logger::log(LogLevel::Info, "Konfiguration gespeichert. AP wird beendet.");
    } else {
        _server.send(400, "text/plain", "Fehler: SSID darf nicht leer sein.");
        Logger::log(LogLevel::Error, "Fehler: SSID war leer bei Konfigurations-POST.");
    }
}

// Handler für nicht gefundene Seiten
void ConfigurationPortal::handleNotFound() {
    _server.send(404, "text/plain", "Seite nicht gefunden");
}

// Muss in der loop() aufgerufen werden
void ConfigurationPortal::handleClient() {
    _server.handleClient();
}

// Lädt gespeicherte WLAN-Daten aus NVS
bool ConfigurationPortal::loadWiFiConfig(String& ssid, String& password) {
    ssid = _preferences.getString(NVS_KEY_SSID, ""); // Zweiter Parameter ist Standardwert falls Key nicht existiert
    password = _preferences.getString(NVS_KEY_PASSWORD, "");

    if (ssid.length() > 0) {
        Logger::log(LogLevel::Info, "WLAN-Konfiguration aus NVS geladen.");
        return true;
    } else {
        Logger::log(LogLevel::Info, "Keine WLAN-Konfiguration in NVS gefunden.");
        return false;
    }
}

// Speichert WLAN-Daten in NVS
bool ConfigurationPortal::saveWiFiConfig(const String& ssid, const String& password) {
    _preferences.putString(NVS_KEY_SSID, ssid);
    _preferences.putString(NVS_KEY_PASSWORD, password);
    Logger::log(LogLevel::Info, "WLAN-Konfiguration in NVS geschrieben.");
    return true; // putString gibt keinen direkten Fehler zurück, Annahme ist Erfolg
}