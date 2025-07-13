#include "WeatherClient.h"
#include "WeatherData.h" // Benötigt die vollständige Definition von WeatherData

// Implementierung des privaten Konstruktors
// Er wird von WeatherClient::getInstance() aufgerufen.
WeatherClient::WeatherClient(const char* host, const char* apiKey)
    : _host(host), _apiKey(apiKey)
{
    if (_host == nullptr || _apiKey == nullptr) {
        Logger::log(LogLevel::Error, "WeatherClient: Host oder API Key sind im Konstruktor NULL. Sicherstellen, dass getInstance() einmal mit allen Parametern aufgerufen wird.");
    }

    // Für WiFiClientSecure auf dem ESP32:
    // setInsecure() kann für Testzwecke verwendet werden, um die Zertifikatsprüfung zu umgehen.
    // In einer Produktionsumgebung wird DRINGEND empfohlen, Root-CA-Zertifikate zu verwenden.
    // Die ESP32 Arduino Core's WiFiClientSecure unterstützt setCACert().
    // Wenn Sie unsicher sind, ob setInsecure() funktioniert, können Sie es auskommentieren
    // und sich ausschließlich auf das Zertifikat verlassen.
    // _client.setInsecure(); // Kann aktiviert werden, um die Zertifikatsprüfung zu deaktivieren (NICHT FÜR PRODUKTION!)

    // Beispiel für das Einbetten eines Root-CA-Zertifikats
    // Dies ist der empfohlene Weg für sichere HTTPS-Verbindungen.
    const char* google_root_ca = "-----BEGIN CERTIFICATE-----\n" \
                                 "MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n" \
                                 "CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n" \
                                 "MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n" \
                                 "MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n" \
                                 "Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n" \
                                 "A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n" \
                                 "27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n" \
                                 "Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n" \
                                 "TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n" \
                                 "qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n" \
                                 "szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n" \
                                 "Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n" \
                                 "MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n" \
                                 "wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n" \
                                 "aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n" \
                                 "VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n" \
                                 "AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n" \
                                 "FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n" \
                                 "C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n" \
                                 "QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n" \
                                 "h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n" \
                                 "7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n" \
                                 "ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n" \
                                 "MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n" \
                                 "Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n" \
                                 "6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n" \
                                 "0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n" \
                                 "2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n" \
                                 "bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n" \
                                 "-----END CERTIFICATE-----\n";
    _client.setCACert(google_root_ca);
}

bool WeatherClient::getCurrentConditions(float latitude, float longitude, WeatherData& outWeatherData) {
    outWeatherData.reset(); // Vor dem Abruf zurücksetzen

    // Überprüfe, ob die Konfiguration gesetzt ist (nach der ersten getInstance-Initialisierung)
    if (_host == nullptr || _apiKey == nullptr) {
        Logger::log(LogLevel::Error, "WeatherClient: Host oder API Key nicht gesetzt. Bitte getInstance mit Host und API Key aufrufen.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::log(LogLevel::Error, "WeatherClient: Kein WLAN verbunden.");
        return false;
    }

    String url = "/v1/currentConditions:lookup?languageCode=CH&location.latitude=";
    url += String(latitude, 6); // 6 Dezimalstellen für Präzision
    url += "&location.longitude=";
    url += String(longitude, 6);
    url += "&unitsSystem=METRIC&alt=json";

    Logger::log(LogLevel::Info, "WeatherClient: Verbinde mit " + String(_host));

    if (!_client.connect(_host, 443)) { // 443 ist der Standard-HTTPS-Port
        Logger::log(LogLevel::Error, "WeatherClient: Verbindung zum Server fehlgeschlagen!");
        return false;
    }

    // HTTP-Anfrage senden
    _client.print(F("GET "));
    _client.print(url);
    _client.println(F(" HTTP/1.1"));
    _client.print(F("Host: "));
    _client.println(_host);
    _client.println(F("Accept: application/json"));
    _client.print(F("Authorization: Bearer "));
    _client.println(_apiKey); // Dein Access Token
    _client.println(F("Connection: close")); // Verbindung nach der Anfrage schließen
    _client.println(); // Leere Zeile nach den Headern

    // Auf die Antwort warten und HTTP-Header lesen
    unsigned long timeout = millis() + 10000; // 10 Sekunden Timeout
    while (_client.available() == 0 && millis() < timeout) {
        delay(10);
    }

    if (_client.available() == 0) {
        Logger::log(LogLevel::Error, "WeatherClient: Server-Antwort-Timeout!");
        _client.stop();
        return false;
    }

    // Statuszeile lesen (z.B. "HTTP/1.1 200 OK")
    String statusLine = _client.readStringUntil('\n');
    Logger::log(LogLevel::Debug, "WeatherClient: Status Line: " + statusLine);
    if (statusLine.indexOf("200 OK") == -1) {
        Logger::log(LogLevel::Error, "WeatherClient: API-Fehler (kein 200 OK). Status: " + statusLine);
        // Versuchen, den restlichen Header zu verwerfen, falls noch Daten im Puffer sind
        while (_client.available() && _client.readStringUntil('\n').length() != 0);
        _client.stop();
        return false;
    }

    // Alle weiteren Header bis zur leeren Zeile verwerfen
    // Achtung: Nach dem Lesen der Statuszeile können noch andere Header folgen.
    // Die Schleife muss wirklich bis zur leeren Zeile lesen.
    while (_client.available()) {
        String line = _client.readStringUntil('\n');
        if (line.length() == 0 || line == "\r") { // Berücksichtige CRLF
            break;
        }
    }

    // JSON-Antwort parsen
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, _client);

    _client.stop();

    if (error) {
        Logger::log(LogLevel::Error, "WeatherClient: JSON-Parsing fehlgeschlagen: " + String(error.c_str()));
        Logger::log(LogLevel::Error, error.c_str());
        return false;
    }

    return parseJson(doc, outWeatherData);
}

// Hilfsfunktion zum Parsen des JSON und Befüllen des WeatherData-Objekts
bool WeatherClient::parseJson(JsonDocument& doc, WeatherData& outWeatherData) {
    // 1. Temperatur (aus "temperature" Objekt)
    outWeatherData.temperature.degrees = doc["temperature"]["degrees"] | 0.0f;
    outWeatherData.temperature.unit = doc["temperature"]["unit"].as<String>();

    // 2. Luftfeuchtigkeit (aus "relativeHumidity")
    outWeatherData.relativeHumidity = doc["relativeHumidity"] | 0.0f;

    // 3. Wetterart als Enum (aus "weatherCondition.description.text")
    // Überprüfen, ob das weatherCondition-Objekt und das description-Objekt existieren
    if (!doc["weatherCondition"].isNull() && !doc["weatherCondition"]["description"].isNull()) {
        String typeString = doc["weatherCondition"]["description"]["text"].as<String>();

        // Map the string to the enum value
        outWeatherData.weatherType = WeatherData::weatherConditionStringToType(typeString);

    } else {
        outWeatherData.weatherType = WeatherConditionType::UNKNOWN; // Fallback, falls das Feld fehlt
    }

    return true;
}