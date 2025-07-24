#include "ApiClient.h"
#include <ArduinoJson.h> // Stellen Sie sicher, dass dies für JsonDocument enthalten ist

// Hilfsfunktion zum Entfernen der Chunked-Codierung aus einem rohen HTTP-Antwort-Body.
// Diese Funktion geht davon aus, dass der Eingabestring 'rawResponse' sowohl die Chunk-Größen als auch die Daten enthält.
String stripChunkedEncoding(const String& rawResponse) {
    String cleanedResponse = "";
    int currentIndex = 0;

    while (currentIndex < rawResponse.length()) {
        // Finden Sie das Ende der Chunk-Größenzeile (erste Zeilenumbruch)
        int newLineIndex = rawResponse.indexOf('\n', currentIndex);
        if (newLineIndex == -1) {
            // Keine weiteren Zeilenumbrüche, wahrscheinlich ein unvollständiger Chunk oder Ende der Antwort
            break;
        }

        String chunkSizeHex = rawResponse.substring(currentIndex, newLineIndex);
        chunkSizeHex.trim(); // Entfernen Sie eventuelle Wagenrückläufe, falls vorhanden

        // Konvertieren Sie die hexadezimale Chunk-Größe in eine Ganzzahl
        long chunkSize = strtol(chunkSizeHex.c_str(), NULL, 16);

        // Wenn die Chunk-Größe 0 ist, ist es der letzte Chunk und signalisiert das Ende des Bodys
        if (chunkSize == 0) {
            // Nach '0\r\n' sollte ein weiteres '\r\n' zur Beendigung der Nachricht folgen.
            // Wir können hier abbrechen, da der JSON-Inhalt beendet ist.
            break;
        }

        // Bewegen Sie sich über die Chunk-Größenzeile hinaus (einschließlich des Zeilenumbruchzeichens)
        currentIndex = newLineIndex + 1;

        // Lesen Sie die Chunk-Daten
        if (currentIndex + chunkSize <= rawResponse.length()) {
            cleanedResponse += rawResponse.substring(currentIndex, currentIndex + chunkSize);
            currentIndex += chunkSize;
        } else {
            // Fehler: Chunk-Größe überschreitet die verfügbaren Daten in rawResponse
            // Dies sollte nicht passieren, wenn rawResponse vollständig ist.
            break;
        }

        // Überspringen Sie das CRLF (oder LF) nach den Chunk-Daten
        // Prüfen Sie zuerst auf '\r\n', dann nur auf '\n'
        if (rawResponse.substring(currentIndex, currentIndex + 2) == "\r\n") {
            currentIndex += 2;
        } else if (rawResponse.substring(currentIndex, currentIndex + 1) == "\n") {
            currentIndex += 1;
        } else {
            // Unerwartete Beendigung des Chunks, könnte ein Fehler oder Ende des Streams sein
            break;
        }
    }
    return cleanedResponse;
}

// Konstruktor initialisiert Member
ApiClient::ApiClient() : _host(nullptr), _apiKey(nullptr) {
    // Root-CA-Zertifikat hier setzen, da es für alle Google APIs gleich sein sollte
    // Gültig bis 2036-06-22
    // Ein Neues Zertifikat kann von: https://pki.goog/repository/ herunter geladen werden
    const char* google_root_ca = "-----BEGIN CERTIFICATE-----\n"
                                 "MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
                                 "CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
                                 "MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
                                 "MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
                                 "Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
                                 "A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
                                 "27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
                                 "Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
                                 "TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
                                 "qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
                                 "szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
                                 "Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
                                 "MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
                                 "wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
                                 "aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
                                 "VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
                                 "AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
                                 "FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
                                 "C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
                                 "QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
                                 "h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
                                 "7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
                                 "ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
                                 "MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
                                 "Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
                                 "6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
                                 "0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
                                 "2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
                                 "bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
                                 "-----END CERTIFICATE-----\n";
    _client.setCACert(google_root_ca);
}

void ApiClient::configure(const char* host, const char* apiKey) {
    _host = host;
    _apiKey = apiKey;
    if (_host == nullptr || _apiKey == nullptr) {
        Logger::log(LogLevel::Error, "ApiClient: Host oder API Key sind NULL. Sicherstellen, dass getInstance() einmal mit allen Parametern aufgerufen wird.");
    }
}

bool ApiClient::sendGetRequest(const String& path, JsonDocument& doc) {
    if (_host == nullptr || _apiKey == nullptr) {
        Logger::log(LogLevel::Error, "ApiClient: Host oder API Key nicht gesetzt. Bitte configure() aufrufen.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::log(LogLevel::Error, "ApiClient: Kein WLAN verbunden.");
        return false;
    }

    Logger::log(LogLevel::Info, "ApiClient: Verbinde mit " + String(_host));

    if (!_client.connect(_host, 443)) { // 443 ist der Standard-HTTPS-Port
        Logger::log(LogLevel::Error, "ApiClient: Verbindung zum Server fehlgeschlagen!");
        return false;
    }

    // HTTP-Anfrage senden
    _client.print(F("GET "));
    _client.print(path);
    _client.println(F(" HTTP/1.1"));
    _client.print(F("Host: "));
    _client.println(_host);
    _client.println(F("Connection: close")); // Verbindung nach der Anfrage schließen
    _client.println(); // Leere Zeile nach den Headern

    // Auf die Antwort warten und HTTP-Header lesen
    unsigned long timeout = millis() + 10000; // 10 Sekunden Timeout
    while (_client.available() == 0 && millis() < timeout) {
        delay(10);
    }

    if (_client.available() == 0) {
        Logger::log(LogLevel::Error, "ApiClient: Server-Antwort-Timeout!");
        _client.stop();
        return false;
    }

    // Statuszeile lesen (z.B. "HTTP/1.1 200 OK")
    String statusLine = _client.readStringUntil('\n');
    Logger::log(LogLevel::Debug, "ApiClient: Status Line: " + statusLine);

    if (statusLine.indexOf("200 OK") == -1) {
        Logger::log(LogLevel::Error, "ApiClient: API-Fehler (kein 200 OK). Status: " + statusLine);

        // Auch hier die restlichen Header verwerfen, damit die Verbindung sauber geschlossen wird
        while (_client.available() && _client.readStringUntil('\n') != "\r"); // Lies bis zur leeren Zeile
        _client.stop();
        return false;
    }
    
    // Alle weiteren Header bis zur leeren Zeile verwerfen
    while (_client.available()) {
        String line = _client.readStringUntil('\n');
        Logger::log(LogLevel::Debug, "Header: " + line); // Debug: Zeige alle Header

        if (line.length() == 0 || line == "\r") { // Prüfe auf leere Zeile (CRLF oder nur LF)
            Logger::log(LogLevel::Debug, "Ende der Header erreicht.");
            break; // Stoppe nach der leeren Zeile, die das Ende der Header signalisiert
        }
    }

    // Den gesamten Body lesen, der Chunk-Informationen enthalten kann
    String rawResponseBody = _client.readString();
    Logger::log(LogLevel::Debug, "======================================\nRoher API Antwort Body (inkl. Chunk-Info):" + rawResponseBody +"\n======================================");

    // Den Body von Chunk-Informationen bereinigen
    String cleanedJsonBody = stripChunkedEncoding(rawResponseBody);
    Logger::log(LogLevel::Debug, "======================================\nBereinigter JSON Body:" + cleanedJsonBody + "======================================");

    // JSON-Antwort parsen
    DeserializationError error = deserializeJson(doc, cleanedJsonBody);

    _client.stop(); // Verbindung schließen

    if (error) {
        Logger::log(LogLevel::Error, "ApiClient: JSON-Parsing fehlgeschlagen: " + String(error.c_str()));
        return false;
    }

    return true; // JSON-Dokument wurde erfolgreich gefüllt
}
