#include "ApiClient.h"
#include <ArduinoJson.h> // Stellen Sie sicher, dass dies für JsonDocument enthalten ist

// Hilfsfunktion zum Entfernen der Chunked-Codierung aus einem rohen HTTP-Antwort-Body.
// Diese Funktion geht davon aus, dass der Eingabestring 'rawResponse' sowohl die Chunk-Größen als auch die Daten enthält.
String stripChunkedEncoding(const String& rawResponse) {
    String trimmed = rawResponse;
    trimmed.trim();
    // Wenn die Antwort bereits mit '{' oder '[' beginnt, ist sie nicht chunked
    if (trimmed.startsWith("{") || trimmed.startsWith("[")) {
        return rawResponse;
    }

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
    return (cleanedResponse.length() > 0) ? cleanedResponse : rawResponse;
}

// Konstruktor initialisiert Member
ApiClient::ApiClient() : _host(""), _apiKey("") {
    // Insecure-Modus für HTTPS: Erlaubt Verbindung auch wenn die interne Systemuhr
    // des ESP32 vor dem ersten NTP-Sync noch auf 1970 steht.
    _client.setInsecure();
}

void ApiClient::configure(const char* host, const char* apiKey) {
    if (host != nullptr) {
        _host = host;
    }
    if (apiKey != nullptr) {
        _apiKey = apiKey;
    }
    if (_host.length() == 0 || _apiKey.length() == 0) {
        Logger::log(LogLevel::Error, "ApiClient: Host oder API Key sind leer. Sicherstellen, dass ein gültiger Google API Key konfiguriert ist.");
    }
}

bool ApiClient::sendGetRequest(const String& path, JsonDocument& doc) {
    if (_host.length() == 0 || _apiKey.length() == 0) {
        Logger::log(LogLevel::Error, "ApiClient: Host oder API Key nicht gesetzt. Bitte configure() aufrufen oder im Webportal eintragen.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::log(LogLevel::Error, "ApiClient: Kein WLAN verbunden.");
        return false;
    }

    Logger::log(LogLevel::Info, "ApiClient: Verbinde mit " + _host);

    if (!_client.connect(_host.c_str(), 443)) { // 443 ist der Standard-HTTPS-Port
        Logger::log(LogLevel::Error, "ApiClient: Verbindung zum Server fehlgeschlagen!");
        return false;
    }

    // HTTP-Anfrage senden
    _client.print(F("GET "));
    _client.print(path);
    _client.println(F(" HTTP/1.1"));
    _client.print(F("Host: "));
    _client.println(_host);
    _client.println(F("User-Agent: ESP32-TimeTale"));
    _client.println(F("Accept: application/json"));
    _client.println(F("Accept-Encoding: identity"));
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
    statusLine.trim();
    Logger::log(LogLevel::Debug, "ApiClient: Status Line: " + statusLine);

    if (statusLine.indexOf("200") == -1) {
        Logger::log(LogLevel::Error, "ApiClient: API-Fehler (kein 200 OK). Status: " + statusLine);

        // Header bis zur Leerzeile überspringen
        while (_client.available()) {
            String line = _client.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) {
                break;
            }
        }
        String errorBody = _client.readString();
        Logger::log(LogLevel::Error, "ApiClient: Server-Fehlerantwort: " + errorBody);
        _client.stop();
        return false;
    }
    
    // Header lesen und prüfen, ob Transfer-Encoding: chunked verwendet wird
    bool isChunked = false;
    while (_client.available()) {
        String line = _client.readStringUntil('\n');
        Logger::log(LogLevel::Debug, "Header: " + line); // Debug: Zeige alle Header

        String trimmedLine = line;
        trimmedLine.trim();
        if (trimmedLine.length() == 0) {
            Logger::log(LogLevel::Debug, "Ende der Header erreicht.");
            break; // Stoppe nach der leeren Zeile, die das Ende der Header signalisiert
        }

        String lowerLine = trimmedLine;
        lowerLine.toLowerCase();
        if (lowerLine.indexOf("transfer-encoding: chunked") != -1) {
            isChunked = true;
        }
    }

    // Den gesamten Body lesen
    String rawResponseBody = _client.readString();
    Logger::log(LogLevel::Debug, "======================================\nRoher API Antwort Body (inkl. Chunk-Info):" + rawResponseBody +"\n======================================");

    // Nur von Chunk-Informationen bereinigen, wenn tatsächlich chunked übertragen
    String cleanedJsonBody;
    String trimmedBody = rawResponseBody;
    trimmedBody.trim();
    if (isChunked && !trimmedBody.startsWith("{") && !trimmedBody.startsWith("[")) {
        cleanedJsonBody = stripChunkedEncoding(rawResponseBody);
    } else {
        cleanedJsonBody = rawResponseBody;
    }
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
