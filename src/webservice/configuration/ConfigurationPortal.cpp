#include "ConfigurationPortal.h"
#include "../../logger/Logger.h" // Pfad zum Logger, bitte bei Bedarf anpassen

// NVS-Namespace und Keys für die Speicherung der Konfigurationsdaten
// Der Namespace sollte eindeutig sein, um Konflikte zu vermeiden.
#define NVS_NAMESPACE "app_config"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PASSWORD "wifi_pass"
#define NVS_KEY_TIMEOFFSET "time_offset"
#define NVS_KEY_NTPSERVER "ntp_server"
#define NVS_KEY_GOOGLE_ACCESS_TOKEN "google_token"
#define NVS_KEY_WEATHER_INT "weather_int"
#define NVS_KEY_POLLEN_INT "pollen_int"
#define NVS_KEY_LONGITUDE "longitude"
#define NVS_KEY_LATITUDE "latitude"
#define NVS_KEY_INDOOR_TIME "indoor_time"
#define NVS_KEY_OUTDOOR_TIME "outdoor_time"
#define NVS_KEY_TEXTCOLOR "text_color"
#define NVS_KEY_LEDBRIGHTNESS "led_brightness"
#define NVS_KEY_VOLUME "volume"

// Standardwerte für die Konfiguration, falls noch nichts im NVS gespeichert ist.
const char* DEFAULT_NTP_SERVER = "ntp.metas.ch";
const int DEFAULT_TIME_OFFSET = 1; // MEZ
const int DEFAULT_WEATHER_INTERVAL = 10; // Minuten
const int DEFAULT_POLLEN_INTERVAL = 60; // Minuten
const float DEFAULT_LONGITUDE = 0.0;
const float DEFAULT_LATITUDE = 0.0;
const int DEFAULT_INDOOR_TIME = 5; // Sekunden
const int DEFAULT_OUTDOOR_TIME = 5; // Sekunden
const uint32_t DEFAULT_TEXT_COLOR = 0xFFFFFF; // Weiss (CRGB::White)
const int DEFAULT_BRIGHTNESS = 50; // Standard-Helligkeit (0-100)
const int DEFAULT_VOLUME = 15; // Mitte des Bereichs

// HTML-Seite als PROGMEM String.
// Enthält Platzhalter (z.B. %SSID%), die zur Laufzeit durch aktuelle Werte ersetzt werden.
const char PROGMEM INDEX_HTML_TEMPLATE[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8"> <!-- Zeichensatz auf UTF-8 setzen -->
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Time-Tale Konfiguration</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 20px; background-color: #f0f0f0; color: #333; }
        .container { background-color: #fff; padding: 25px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; max-width: 500px; width: 90%; text-align: left; }
        h1 { color: #007bff; margin-bottom: 20px; }
        h2 { color: #007bff; margin-top: 30px; margin-bottom: 15px; border-bottom: 1px solid #eee; padding-bottom: 5px;}
        label { display: block; margin-bottom: 8px; font-weight: bold; color: #555; }
        input[type="text"], input[type="password"], input[type="number"], select {
            width: calc(100% - 22px);
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box; /* Stellt sicher, dass Padding die Gesamtbreite nicht erhöht */
        }
        input[type="range"] {
            width: calc(100% - 22px);
            margin-bottom: 15px;
            -webkit-appearance: none; /* Entfernt Standard-Styling */
            height: 8px;
            background: #ddd;
            border-radius: 5px;
            outline: none;
            opacity: 0.7;
            -webkit-transition: .2s;
            transition: opacity .2s;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #007bff;
            cursor: pointer;
        }
        input[type="range"]::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #007bff;
            cursor: pointer;
        }
        input[type="submit"] {
            background-color: #28a745;
            color: white;
            padding: 12px 25px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 18px;
            width: 100%;
            box-sizing: border-box;
            margin-top: 20px;
            transition: background-color 0.3s ease;
        }
        input[type="submit"]:hover {
            background-color: #218838;
        }
        .info-message {
            margin-top: 20px;
            padding: 10px;
            background-color: #e9ecef;
            border-left: 5px solid #007bff;
            border-radius: 5px;
            font-size: 0.9em;
            color: #495057;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Time-Tale Konfiguration</h1>
        <form action="/save" method="POST">
            <h2>WLAN-Einstellungen</h2>
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid" value="%SSID%" required><br>
            <label for="password">Passwort (leer lassen, um das aktuelle zu behalten):</label>
            <input type="password" id="password" name="password" value=""><br>

            <h2>Weitere Einstellungen</h2>
            <label for="timeOffset">Zeitverschiebung (Stunden, z.B. 1 für MEZ, 2 für MESZ):</label>
            <input type="number" id="timeOffset" name="timeOffset" value="%TIMEOFFSET%"><br>

            <label for="ntpServer">NTP Server (z.B. ntp.metas.ch):</label>
            <input type="text" id="ntpServer" name="ntpServer" value="%NTPSERVER%"><br>

            <label for="googleAccessToken">Google Access Token für API (leer lassen, um das aktuelle zu behalten):</label>
            <input type="password" id="googleAccessToken" name="googleAccessToken" value=""><br>

            <label for="weatherUpdateInterval">Wetter-API Update Intervall (Minuten):</label>
            <input type="number" id="weatherUpdateInterval" name="weatherUpdateInterval" value="%WEATHER_INT%"><br>

            <label for="pollenUpdateInterval">Pollen-API Update Intervall (Minuten):</label>
            <input type="number" id="pollenUpdateInterval" name="pollenUpdateInterval" value="%POLLEN_INT%"><br>

            <label for="longitude">Längengrad (z.B. 7.4474 für Bern):</label>
            <input type="number" step="any" id="longitude" name="longitude" value="%LONGITUDE%"><br>

            <label for="latitude">Breitengrad (z.B. 46.9480 für Bern):</label>
            <input type="number" step="any" id="latitude" name="latitude" value="%LATITUDE%"><br>

            <label for="indoorTempDisplayTime">Anzeigedauer Innentemperatur (Sekunden):</label>
            <input type="number" id="indoorTempDisplayTime" name="indoorTempDisplayTime" value="%INDOOR_TIME%"><br>

            <label for="outdoorTempDisplayTime">Anzeigedauer Aussentemperatur (Sekunden):</label>
            <input type="number" id="outdoorTempDisplayTime" name="outdoorTempDisplayTime" value="%OUTDOOR_TIME%"><br>

            <label for="textColor">Textfarbe für LED Anzeige:</label>
            <select id="textColor" name="textColor">
                <option value="0xFF0000" %COLOR_FF0000_SELECTED%>Rot</option>
                <option value="0x00FF00" %COLOR_00FF00_SELECTED%>Grün</option>
                <option value="0x0000FF" %COLOR_0000FF_SELECTED%>Blau</option>
                <option value="0xFFFFFF" %COLOR_FFFFFF_SELECTED%>Weiss</option>
                <option value="0xFFA500" %COLOR_FFA500_SELECTED%>Orange</option>
                <option value="0x800080" %COLOR_800080_SELECTED%>Lila</option>
                <option value="0x00FFFF" %COLOR_00FFFF_SELECTED%>Cyan</option>
                <option value="0xFFFF00" %COLOR_FFFF00_SELECTED%>Gelb</option>
                <option value="0xFFC0CB" %COLOR_FFC0CB_SELECTED%>Pink</option>
            </select><br>

            <label for="ledBrightness">LED Helligkeit (0-100): <span id="ledBrightnessValue">%LEDBRIGHTNESS%</span></label>
            <input type="range" id="ledBrightness" name="ledBrightness" min="0" max="100" value="%LEDBRIGHTNESS%" oninput="document.getElementById('ledBrightnessValue').innerText = this.value;"><br>

            <label for="volume">Lautstärke (0 = Stumm, 30 = Max): <span id="volumeValue">%VOLUME%</span></label>
            <input type="range" id="volume" name="volume" min="0" max="30" value="%VOLUME%" oninput="document.getElementById('volumeValue').innerText = this.value;"><br>

            <input type="submit" value="Einstellungen Speichern">
        </form>
        <div class="info-message">
            Nach dem Speichern versucht das Gerät, sich mit dem WLAN zu verbinden oder die Einstellungen zu aktualisieren.
            Ein Neustart des Geräts kann erforderlich sein, um alle Änderungen zu übernehmen.
        </div>
    </div>
</body>
</html>
)rawliteral";

// Konstruktor der ConfigurationPortal Klasse.
// Initialisiert den Webserver und öffnet den NVS-Namespace.
ConfigurationPortal::ConfigurationPortal() : _server(HTTP_PORT), _configSavedCallback(nullptr) {
    // Der Preferences-Namespace wird hier geöffnet.
    // "false" bedeutet Read/Write-Modus.
    _preferences.begin(NVS_NAMESPACE, false);
}

// Startet den Access Point und den Webserver für die Erstkonfiguration.
// Dies ist der Modus, in dem der ESP32 einen eigenen WLAN-Hotspot aufspannt.
bool ConfigurationPortal::startAPAndWebServer(const char* apSsid, const char* apPassword) {
    Logger::log(LogLevel::Info, "Starte Konfigurations-AP mit SSID: " + String(apSsid));

    // Setze den WLAN-Modus auf Access Point.
    WiFi.mode(WIFI_AP);
    // Starte den Access Point mit der angegebenen SSID und optionalem Passwort.
    if (!WiFi.softAP(apSsid, apPassword)) {
        Logger::log(LogLevel::Error, "Fehler beim Starten des Access Points!");
        return false;
    }

    // Gib die IP-Adresse des Access Points aus.
    IPAddress apIP = WiFi.softAPIP();
    Logger::log(LogLevel::Info, "AP gestartet, IP-Adresse: " + apIP.toString());

    setupWebServerRoutes(); // Richte die Routen für den Webserver ein.
    _server.begin();        // Starte den Webserver.
    Logger::log(LogLevel::Info, "Webserver auf Port " + String(HTTP_PORT) + " gestartet (AP-Modus).");

    return true;
}

// Startet den Webserver im Station-Modus, nachdem WLAN verbunden ist.
// In diesem Modus ist der ESP32 mit einem bestehenden WLAN verbunden und der Webserver
// ist über die zugewiesene IP-Adresse im lokalen Netzwerk erreichbar.
bool ConfigurationPortal::startWebServerInStationMode() {
    // Prüfe, ob der ESP32 tatsächlich mit einem WLAN verbunden ist.
    if (WiFi.status() != WL_CONNECTED) {
        Logger::log(LogLevel::Error, "WLAN ist nicht verbunden. Kann Webserver nicht im Station-Modus starten.");
        return false;
    }
    Logger::log(LogLevel::Info, "Starte Webserver im Station-Modus auf IP: " + WiFi.localIP().toString());

    setupWebServerRoutes(); // Richte die Routen für den Webserver ein.
    _server.begin();        // Starte den Webserver.
    Logger::log(LogLevel::Info, "Webserver auf Port " + String(HTTP_PORT) + " gestartet (Station-Modus).");
    return true;
}

// Setzt die Callback-Funktion, die nach dem Speichern der Konfiguration aufgerufen wird.
void ConfigurationPortal::onConfigSaved(void (*callback)(const AppConfig& config)) {
    _configSavedCallback = callback;
}

// Lädt alle Konfigurationsdaten aus dem NVS-Speicher in die übergebene AppConfig-Struktur.
// Wenn ein Wert nicht gefunden wird, wird ein Standardwert verwendet.
bool ConfigurationPortal::loadConfig(AppConfig& config) {
    config.wifiSsid = _preferences.getString(NVS_KEY_SSID, "");
    config.wifiPassword = _preferences.getString(NVS_KEY_PASSWORD, "");
    config.timeOffsetHours = _preferences.getInt(NVS_KEY_TIMEOFFSET, DEFAULT_TIME_OFFSET);
    config.ntpServer = _preferences.getString(NVS_KEY_NTPSERVER, DEFAULT_NTP_SERVER);
    config.googleAccessToken = _preferences.getString(NVS_KEY_GOOGLE_ACCESS_TOKEN, "");
    config.weatherUpdateIntervalMin = _preferences.getInt(NVS_KEY_WEATHER_INT, DEFAULT_WEATHER_INTERVAL);
    config.pollenUpdateIntervalMin = _preferences.getInt(NVS_KEY_POLLEN_INT, DEFAULT_POLLEN_INTERVAL);
    config.longitude = _preferences.getFloat(NVS_KEY_LONGITUDE, DEFAULT_LONGITUDE);
    config.latitude = _preferences.getFloat(NVS_KEY_LATITUDE, DEFAULT_LATITUDE);
    config.indoorTempDisplayTimeSec = _preferences.getInt(NVS_KEY_INDOOR_TIME, DEFAULT_INDOOR_TIME);
    config.outdoorTempDisplayTimeSec = _preferences.getInt(NVS_KEY_OUTDOOR_TIME, DEFAULT_OUTDOOR_TIME);
    config.textColorCRGB = _preferences.getUInt(NVS_KEY_TEXTCOLOR, DEFAULT_TEXT_COLOR);
    config.ledBrightness = _preferences.getInt(NVS_KEY_LEDBRIGHTNESS, DEFAULT_BRIGHTNESS);
    config.volume = _preferences.getInt(NVS_KEY_VOLUME, DEFAULT_VOLUME);

    // Prüfe, ob eine WLAN-SSID gefunden wurde, um zu bestimmen, ob eine "gespeicherte" Konfiguration existiert.
    if (config.wifiSsid.length() > 0) {
        Logger::log(LogLevel::Info, "Konfiguration aus NVS geladen.");
        return true;
    } else {
        Logger::log(LogLevel::Info, "Keine WLAN-SSID in NVS gefunden. Lade Standardwerte für andere Einstellungen.");
        return false; // Zeigt an, dass keine vollständige WLAN-Konfiguration gefunden wurde.
    }
}

// Speichert alle Konfigurationsdaten aus der übergebenen AppConfig-Struktur in den NVS-Speicher.
bool ConfigurationPortal::saveConfig(const AppConfig& config) {
    _preferences.putString(NVS_KEY_SSID, config.wifiSsid);
    _preferences.putString(NVS_KEY_PASSWORD, config.wifiPassword);
    _preferences.putInt(NVS_KEY_TIMEOFFSET, config.timeOffsetHours);
    _preferences.putString(NVS_KEY_NTPSERVER, config.ntpServer);
    _preferences.putString(NVS_KEY_GOOGLE_ACCESS_TOKEN, config.googleAccessToken);
    _preferences.putInt(NVS_KEY_WEATHER_INT, config.weatherUpdateIntervalMin);
    _preferences.putInt(NVS_KEY_POLLEN_INT, config.pollenUpdateIntervalMin);
    _preferences.putFloat(NVS_KEY_LONGITUDE, config.longitude);
    _preferences.putFloat(NVS_KEY_LATITUDE, config.latitude);
    _preferences.putInt(NVS_KEY_INDOOR_TIME, config.indoorTempDisplayTimeSec);
    _preferences.putInt(NVS_KEY_OUTDOOR_TIME, config.outdoorTempDisplayTimeSec);
    _preferences.putUInt(NVS_KEY_TEXTCOLOR, config.textColorCRGB);
    _preferences.putInt(NVS_KEY_LEDBRIGHTNESS, config.ledBrightness);
    _preferences.putInt(NVS_KEY_VOLUME, config.volume);

    Logger::log(LogLevel::Info, "Konfiguration erfolgreich in NVS geschrieben.");
    return true; // put-Operationen geben keinen direkten Fehler zurück, Annahme ist Erfolg.
}

// Handler für die Startseite ("/") des Webservers.
// Ersetzt Platzhalter im HTML-Template durch die aktuellen Konfigurationswerte.
void ConfigurationPortal::handleRoot() {
    String html = INDEX_HTML_TEMPLATE; // Kopie des Templates erstellen
    AppConfig currentConfig;
    loadConfig(currentConfig); // Lade die aktuelle Konfiguration

    // Ersetze Platzhalter für WLAN-Einstellungen
    html.replace("%SSID%", currentConfig.wifiSsid);
    // Das Passwort wird aus Sicherheitsgründen nicht im Formular angezeigt,
    // das Feld bleibt leer, es sei denn, ein neues Passwort wird eingegeben.
    // html.replace("%PASSWORD%", currentConfig.wifiPassword);

    // Ersetze Platzhalter für weitere Einstellungen
    html.replace("%TIMEOFFSET%", String(currentConfig.timeOffsetHours));
    html.replace("%NTPSERVER%", currentConfig.ntpServer);

    // Google Access Token soll leer bleiben, da dieses wie ein Passwort zu behandeln ist.
    // Der Wert im HTML-Template ist bereits leer, daher keine Ersetzung nötig,
    // um den gespeicherten Wert nicht anzuzeigen.
    // html.replace("%GOOGLE_ACCESS_TOKEN%", currentConfig.googleAccessToken); // Diese Zeile bleibt auskommentiert oder entfernt

    html.replace("%WEATHER_INT%", String(currentConfig.weatherUpdateIntervalMin));
    html.replace("%POLLEN_INT%", String(currentConfig.pollenUpdateIntervalMin));
    // Float-Werte mit 6 Dezimalstellen für Genauigkeit
    html.replace("%LONGITUDE%", String(currentConfig.longitude, 6));
    html.replace("%LATITUDE%", String(currentConfig.latitude, 6));
    html.replace("%INDOOR_TIME%", String(currentConfig.indoorTempDisplayTimeSec));
    html.replace("%OUTDOOR_TIME%", String(currentConfig.outdoorTempDisplayTimeSec));
    html.replace("%LEDBRIGHTNESS%", String(currentConfig.ledBrightness));
    html.replace("%VOLUME%", String(currentConfig.volume));

    // Setze das "selected"-Attribut für die aktuell gespeicherte Farbe
    String selectedColorHex = "0x" + String(currentConfig.textColorCRGB, HEX);
    selectedColorHex.toUpperCase(); // Sicherstellen, dass die Hex-Zeichen Grossbuchstaben sind
    
    // Vergleiche und setze das "selected"-Attribut entsprechend
    if (selectedColorHex == "0XFF0000") html.replace("%COLOR_FF0000_SELECTED%", "selected");
    else if (selectedColorHex == "0X00FF00") html.replace("%COLOR_00FF00_SELECTED%", "selected");
    else if (selectedColorHex == "0X0000FF") html.replace("%COLOR_0000FF_SELECTED%", "selected");
    else if (selectedColorHex == "0XFFFFFF") html.replace("%COLOR_FFFFFF_SELECTED%", "selected");
    else if (selectedColorHex == "0XFFA500") html.replace("%COLOR_FFA500_SELECTED%", "selected");
    else if (selectedColorHex == "0X800080") html.replace("%COLOR_800080_SELECTED%", "selected");
    else if (selectedColorHex == "0X00FFFF") html.replace("%COLOR_00FFFF_SELECTED%", "selected");
    else if (selectedColorHex == "0XFFFF00") html.replace("%COLOR_FFFF00_SELECTED%", "selected");
    else if (selectedColorHex == "0XFFC0CB") html.replace("%COLOR_FFC0CB_SELECTED%", "selected");
    else html.replace("%COLOR_FFFFFF_SELECTED%", "selected"); // Fallback zu Weiss, falls unbekannte Farbe
    
    // Logik für die Auswahl der Textfarbe im Dropdown-Menü
    // alle anderen temporären tags entfernen.
    html.replace("%COLOR_FF0000_SELECTED%", "");
    html.replace("%COLOR_00FF00_SELECTED%", "");
    html.replace("%COLOR_0000FF_SELECTED%", "");
    html.replace("%COLOR_FFFFFF_SELECTED%", "");
    html.replace("%COLOR_FFA500_SELECTED%", "");
    html.replace("%COLOR_800080_SELECTED%", "");
    html.replace("%COLOR_00FFFF_SELECTED%", "");
    html.replace("%COLOR_FFFF00_SELECTED%", "");
    html.replace("%COLOR_FFC0CB_SELECTED%", "");
    html.replace("%COLOR_000000_SELECTED%", "");

    _server.send(200, "text/html", html); // Sende die generierte HTML-Seite an den Client
}

// Handler für die POST-Anfrage zum Speichern der Konfiguration.
// Liest die übermittelten Formulardaten und speichert sie im NVS.
void ConfigurationPortal::handleSaveConfig() {
    AppConfig newConfig;
    // Zuerst die aktuelle Konfiguration laden.
    // Dies ist wichtig, um Werte zu erhalten, die nicht im Formular gesendet wurden
    // (z.B. wenn das Passwortfeld leer gelassen wird, soll das alte Passwort beibehalten werden).
    loadConfig(newConfig);

    // Verarbeite WLAN-Daten
    if (_server.hasArg("ssid")) {
        newConfig.wifiSsid = _server.arg("ssid");
    }
    // Wenn ein neues Passwort eingegeben wurde, aktualisiere es.
    // Ansonsten behalte das alte Passwort bei (durch das vorherige loadConfig).
    if (_server.hasArg("password") && _server.arg("password").length() > 0) {
        newConfig.wifiPassword = _server.arg("password");
    }

    // Verarbeite weitere Einstellungen
    if (_server.hasArg("timeOffset")) {
        newConfig.timeOffsetHours = _server.arg("timeOffset").toInt();
    }
    if (_server.hasArg("ntpServer")) {
        newConfig.ntpServer = _server.arg("ntpServer");
    }

    // Wenn ein neuer Google Access Token eingegeben wurde, aktualisiere es.
    // Ansonsten behalte den alten Google Access Token bei (durch das vorherige loadConfig).
    if (_server.hasArg("googleAccessToken") && _server.arg("googleAccessToken").length() > 0) {
        newConfig.googleAccessToken = _server.arg("googleAccessToken");
    }

    if (_server.hasArg("weatherUpdateInterval")) {
        newConfig.weatherUpdateIntervalMin = _server.arg("weatherUpdateInterval").toInt();
    }
    if (_server.hasArg("pollenUpdateInterval")) {
        newConfig.pollenUpdateIntervalMin = _server.arg("pollenUpdateInterval").toInt();
    }
    if (_server.hasArg("longitude")) {
        newConfig.longitude = _server.arg("longitude").toFloat();
    }
    if (_server.hasArg("latitude")) {
        newConfig.latitude = _server.arg("latitude").toFloat();
    }
    if (_server.hasArg("indoorTempDisplayTime")) {
        newConfig.indoorTempDisplayTimeSec = _server.arg("indoorTempDisplayTime").toInt();
    }
    if (_server.hasArg("outdoorTempDisplayTime")) {
        newConfig.outdoorTempDisplayTimeSec = _server.arg("outdoorTempDisplayTime").toInt();
    }
    if (_server.hasArg("textColor")) {
        // Konvertiere Hex-String (z.B. "0xFF0000") zu uint32_t
        String colorStr = _server.arg("textColor");
        if (colorStr.startsWith("0x") || colorStr.startsWith("0X")) {
            colorStr = colorStr.substring(2); // "0x" oder "0X" entfernen
        }
        newConfig.textColorCRGB = strtoul(colorStr.c_str(), NULL, 16);
    }
    if (_server.hasArg("ledBrightness")) {
        newConfig.ledBrightness = _server.arg("ledBrightness").toInt();
        // Sicherstellen, dass die Helligkeit im gültigen Bereich bleibt (0-100)
        if (newConfig.ledBrightness < 0) newConfig.ledBrightness = 0;
        if (newConfig.ledBrightness > 100) newConfig.ledBrightness = 100;
    }
    if (_server.hasArg("volume")) {
        newConfig.volume = _server.arg("volume").toInt();
        // Sicherstellen, dass die Lautstärke im gültigen Bereich bleibt (0-30)
        if (newConfig.volume < 0) newConfig.volume = 0;
        if (newConfig.volume > 30) newConfig.volume = 30;
    }

    // Logge die empfangenen Daten zur Überprüfung
    Logger::log(LogLevel::Info, "Empfangene Konfigurationsdaten:");
    Logger::log(LogLevel::Info, "  SSID: " + newConfig.wifiSsid);
    Logger::log(LogLevel::Info, "  Zeitverschiebung: " + String(newConfig.timeOffsetHours));
    Logger::log(LogLevel::Info, "  NTP Server: " + newConfig.ntpServer);
    Logger::log(LogLevel::Info, "  Google Access Token Länge: " + String(newConfig.googleAccessToken.length()));
    Logger::log(LogLevel::Info, "  Wetter-Intervall: " + String(newConfig.weatherUpdateIntervalMin) + " min");
    Logger::log(LogLevel::Info, "  Pollen-Intervall: " + String(newConfig.pollenUpdateIntervalMin) + " min");
    Logger::log(LogLevel::Info, "  Längengrad: " + String(newConfig.longitude, 6));
    Logger::log(LogLevel::Info, "  Breitengrad: " + String(newConfig.latitude, 6));
    Logger::log(LogLevel::Info, "  Innentemp. Anzeigedauer: " + String(newConfig.indoorTempDisplayTimeSec) + " s");
    Logger::log(LogLevel::Info, "  Aussentemp. Anzeigedauer: " + String(newConfig.outdoorTempDisplayTimeSec) + " s");
    Logger::log(LogLevel::Info, "  Textfarbe: 0x" + String(newConfig.textColorCRGB, HEX));
    Logger::log(LogLevel::Info, "  LED Helligkeit: " + String(newConfig.ledBrightness));
    Logger::log(LogLevel::Info, "  Lautstärke: " + String(newConfig.volume));


    // Speichere die neue Konfiguration, wenn die SSID nicht leer ist.
    if (newConfig.wifiSsid.length() > 0) {
        if (saveConfig(newConfig)) {
            Logger::log(LogLevel::Info, "Konfiguration erfolgreich in NVS gespeichert.");
        } else {
            Logger::log(LogLevel::Error, "Fehler beim Speichern der Konfiguration in NVS.");
        }

        // Rufe den Callback auf, um das Hauptprogramm über die neue Konfiguration zu informieren.
        if (_configSavedCallback) {
            _configSavedCallback(newConfig);
        }

        // Sende eine Erfolgsmeldung an den Browser.
        _server.send(200, "text/plain", "Konfiguration erfolgreich gespeichert! Das Gerät versucht nun, sich mit dem WLAN zu verbinden oder die Einstellungen zu aktualisieren.");
        Logger::log(LogLevel::Info, "Konfiguration gespeichert. Server-Antwort gesendet.");
    } else {
        // Sende eine Fehlermeldung, wenn die SSID leer ist.
        _server.send(400, "text/plain", "Fehler: SSID darf nicht leer sein.");
        Logger::log(LogLevel::Error, "Fehler: SSID war leer bei Konfigurations-POST.");
    }
}

// Handler für nicht gefundene Seiten (HTTP 404).
void ConfigurationPortal::handleNotFound() {
    _server.send(404, "text/plain", "Seite nicht gefunden");
    Logger::log(LogLevel::Error, "Anfrage für nicht gefundene Seite: " + _server.uri());
}

// Muss regelmässig in der Arduino loop() Funktion aufgerufen werden,
// um eingehende Client-Anfragen an den Webserver zu verarbeiten.
void ConfigurationPortal::handleClient() {
    _server.handleClient();
}

// Hilfsfunktion zum Einrichten der Webserver-Routen.
// Wird sowohl im AP- als auch im Station-Modus verwendet.
void ConfigurationPortal::setupWebServerRoutes() {
    // Route für die Startseite (GET-Anfragen an "/")
    _server.on("/", HTTP_GET, [this]() {
        handleRoot();
    });

    // Route für das Speichern der Konfiguration (POST-Anfragen an "/save")
    _server.on("/save", HTTP_POST, [this]() {
        handleSaveConfig();
    });

    // Handler für alle anderen nicht definierten Routen (404 Not Found)
    _server.onNotFound([this]() {
        handleNotFound();
    });
}
