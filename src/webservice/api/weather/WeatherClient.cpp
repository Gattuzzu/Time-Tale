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

    // ACHTUNG: WiFiSSLClient hat möglicherweise KEINE setInsecure() Methode.
    // Dies hängt von der genauen Implementierung der WiFiS3/WiFiSSLClient Bibliothek ab.
    // Wenn setInsecure() nicht existiert, müssen Sie Root-CA-Zertifikate verwenden
    // oder eine andere Methode finden, um die Zertifikatsprüfung zu umgehen (nicht empfohlen).

    // _client.setInsecure(); // <-- DIESE ZEILE MUSS EVENTUELL ENTFERNT/ANGEPASST WERDEN,
                              // falls WiFiSSLClient diese Methode nicht anbietet.
                              // Wenn Sie HTTPS ohne Zertifikatsprüfung testen wollen,
                              // müssen Sie prüfen, ob die WiFiSSLClient-Lib eine Alternative hat.
                              // Oft ist es aber einfacher, das Root-CA-Zertifikat einzubetten.

    // Beispiel für das Einbetten eines Root-CA-Zertifikats (wenn WiFiSSLClient dies unterstützt):
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
        _client.stop();
        // Hier könntest du versuchen, den Fehlerbody zu lesen, wenn vorhanden
        return false;
    }

    // Alle weiteren Header bis zur leeren Zeile verwerfen
    while (_client.available() && _client.readStringUntil('\n').length() != 0);

    // JSON-Antwort parsen
    // Bestimme die Kapazität des JSON-Dokuments dynamisch oder statisch.
    // Ein großer JSON-String kann viel RAM benötigen.
    // Die Dokumentation von ArduinoJson empfiehlt das ArduinoJson Assistant Tool
    // (https://arduinojson.org/v6/assistant/) um die richtige Größe zu bestimmen.
    // Ich schätze hier konservativ für dein Beispiel-JSON.
    const size_t capacity = 2048; // Schätzung, anpassen nach tatsächlicher Antwortgröße
    StaticJsonDocument<capacity> doc; // Oder DynamicJsonDocument doc;

    DeserializationError error = deserializeJson(doc, _client);

    _client.stop();

    if (error) {
        Logger::log(LogLevel::Error, "WeatherClient: JSON-Parsing fehlgeschlagen: " + String(error.c_str()));
        return false;
    }

    return parseJson(doc, outWeatherData);
}

// Hilfsfunktion zum Parsen des JSON und Befüllen des WeatherData-Objekts
bool WeatherClient::parseJson(JsonDocument& doc, WeatherData& outWeatherData) {
    // Einfache Werte
    outWeatherData.relativeHumidity = doc["relativeHumidity"] | 0; // Standardwert 0
    outWeatherData.uvIndex = doc["uvIndex"] | 0;
    outWeatherData.thunderstormProbability = doc["thunderstormProbability"] | 0;
    outWeatherData.cloudCover = doc["cloudCover"] | 0;

    // Geschachtelte Objekte (degrees & unit)
    outWeatherData.dewPoint.degrees = doc["dewPoint"]["degrees"] | 0.0f;
    outWeatherData.dewPoint.unit = doc["dewPoint"]["unit"].as<String>();

    outWeatherData.heatIndex.degrees = doc["heatIndex"]["degrees"] | 0.0f;
    outWeatherData.heatIndex.unit = doc["heatIndex"]["unit"].as<String>();

    outWeatherData.windChill.degrees = doc["windChill"]["degrees"] | 0.0f;
    outWeatherData.windChill.unit = doc["windChill"]["unit"].as<String>();

    // Air Pressure
    outWeatherData.airPressure.meanSeaLevelMillibars = doc["airPressure"]["meanSeaLevelMillibars"] | 0.0f;

    // Wind
    outWeatherData.wind.direction.degrees = doc["wind"]["direction"]["degrees"] | 0;
    outWeatherData.wind.direction.cardinal = doc["wind"]["direction"]["cardinal"].as<String>();
    outWeatherData.wind.speed.value = doc["wind"]["speed"]["value"] | 0.0f;
    outWeatherData.wind.speed.unit = doc["wind"]["speed"]["unit"].as<String>();
    outWeatherData.wind.gust.value = doc["wind"]["gust"]["value"] | 0.0f;
    outWeatherData.wind.gust.unit = doc["wind"]["gust"]["unit"].as<String>();

    // Visibility
    outWeatherData.visibility.distance = doc["visibility"]["distance"] | 0.0f;
    outWeatherData.visibility.unit = doc["visibility"]["unit"].as<String>();

    // Precipitation
    outWeatherData.precipitation.probability.percent = doc["precipitation"]["probability"]["percent"] | 0;
    outWeatherData.precipitation.probability.type = doc["precipitation"]["probability"]["type"].as<String>();
    outWeatherData.precipitation.snowQpf.quantity = doc["precipitation"]["snowQpf"]["quantity"] | 0.0f;
    outWeatherData.precipitation.snowQpf.unit = doc["precipitation"]["snowQpf"]["unit"].as<String>();
    outWeatherData.precipitation.qpf.quantity = doc["precipitation"]["qpf"]["quantity"] | 0.0f;
    outWeatherData.precipitation.qpf.unit = doc["precipitation"]["qpf"]["unit"].as<String>();

    // Current Conditions History (nur wenn im JSON vorhanden)
    // Überprüfe, ob das Objekt existiert, bevor du darauf zugreifst
    if (doc.containsKey("currentConditionsHistory")) {
        outWeatherData.currentConditionsHistory.temperatureChange.degrees = doc["currentConditionsHistory"]["temperatureChange"]["degrees"] | 0.0f;
        outWeatherData.currentConditionsHistory.temperatureChange.unit = doc["currentConditionsHistory"]["temperatureChange"]["unit"].as<String>();
        outWeatherData.currentConditionsHistory.maxTemperature.degrees = doc["currentConditionsHistory"]["maxTemperature"]["degrees"] | 0.0f;
        outWeatherData.currentConditionsHistory.maxTemperature.unit = doc["currentConditionsHistory"]["maxTemperature"]["unit"].as<String>();
        outWeatherData.currentConditionsHistory.minTemperature.degrees = doc["currentConditionsHistory"]["minTemperature"]["degrees"] | 0.0f;
        outWeatherData.currentConditionsHistory.minTemperature.unit = doc["currentConditionsHistory"]["minTemperature"]["unit"].as<String>();
        outWeatherData.currentConditionsHistory.snowQpf.quantity = doc["currentConditionsHistory"]["snowQpf"]["quantity"] | 0.0f;
        outWeatherData.currentConditionsHistory.snowQpf.unit = doc["currentConditionsHistory"]["snowQpf"]["unit"].as<String>();
        outWeatherData.currentConditionsHistory.qpf.quantity = doc["currentConditionsHistory"]["qpf"]["quantity"] | 0.0f;
        outWeatherData.currentConditionsHistory.qpf.unit = doc["currentConditionsHistory"]["qpf"]["unit"].as<String>();
    }

    return true;
}