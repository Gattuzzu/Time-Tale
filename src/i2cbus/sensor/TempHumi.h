#include <Wire.h> // Arduino I2C Bibliothek

#define SHT30_DEFAULT_ADDRESS 0x44 // Standard-I2C-Adresse für den SHT30 Sensor
                                   // Alternativ 0x45, je nach ADDR-Pin-Konfiguration

// Befehle für Single Shot Measurement mit High Repeatability
// und Clock Stretching aktiviert (für einfacheres Auslesen)
#define SHT30_MEASURE_HIGH_REP_STRETCH 0x2C06

class TempHumi {
public:
    /**
     * @brief Konstruktor für die SHT30-Klasse.
     * @param address Die I2C-Adresse des SHT30-Sensors (Standard ist 0x44 oder 0x45).
     */
    TempHumi(uint8_t address = SHT30_DEFAULT_ADDRESS) : _address(address) {}

    /**
     * @brief Initialisiert den SHT30-Sensor.
     * Prüft, ob der Sensor unter der angegebenen Adresse erreichbar ist.
     * Muss im setup() des Arduino aufgerufen werden.
     * @return true, wenn der Sensor gefunden wurde, false sonst.
     */
    bool begin() {
        Wire.begin(); // Initialisiere die I2C-Kommunikation
        Wire.beginTransmission(_address);
        if (Wire.endTransmission() == 0) {
            // Sensor wurde gefunden
            return true;
        }
        return false; // Sensor nicht gefunden
    }

    /**
     * @brief Liest Temperatur- und Feuchtigkeitswerte vom SHT30-Sensor aus.
     * @param temperature Referenz für die Temperatur in °C.
     * @param humidity Referenz für die relative Feuchtigkeit in %.
     * @return true, wenn die Werte erfolgreich gelesen wurden, false sonst.
     */
    bool readData(float &temperature, float &humidity) {
        // Sende den Messbefehl an den Sensor
        Wire.beginTransmission(_address);
        Wire.write((SHT30_MEASURE_HIGH_REP_STRETCH >> 8) & 0xFF); // High Byte des Befehls
        Wire.write(SHT30_MEASURE_HIGH_REP_STRETCH & 0xFF);      // Low Byte des Befehls
        if (Wire.endTransmission() != 0) {
            return false; // Fehler beim Senden des Befehls
        }

        // Warte eine kurze Zeit, damit der Sensor die Messung durchführen kann.
        // Für High Repeatability werden ca. 15ms empfohlen.
        delay(50); // Sicherheitshalber 50ms warten

        // Lese 6 Bytes (Temperatur, CRC, Feuchtigkeit, CRC) vom Sensor
        if (Wire.requestFrom(_address, (uint8_t)6) != 6) {
            return false; // Konnte nicht alle 6 Bytes lesen
        }

        uint16_t tempRaw = Wire.read();
        tempRaw = (tempRaw << 8) | Wire.read();
        uint8_t crcTemp = Wire.read(); // CRC-Byte für Temperatur (aktuell nicht geprüft)

        uint16_t humidityRaw = Wire.read();
        humidityRaw = (humidityRaw << 8) | Wire.read();
        uint8_t crcHum = Wire.read(); // CRC-Byte für Feuchtigkeit (aktuell nicht geprüft)

        // Optional: CRC-Prüfung hier einfügen, um die Datenintegrität zu gewährleisten.
        // Die CRC-Berechnung ist etwas komplexer und würde den Rahmen sprengen.
        // Für eine robuste Anwendung sollte sie implementiert werden.

        // Berechne Temperatur in °C
        temperature = -45.0f + 175.0f * ((float)tempRaw / 65535.0f);

        // Berechne relative Feuchtigkeit in %
        humidity = 100.0f * ((float)humidityRaw / 65535.0f);

        return true;
    }

private:
    uint8_t _address; // Die I2C-Adresse des SHT30-Sensors
};

// Logger::log(LogLevel::Info, "Temperatur: " + String(actTemperature, 2) + "°C"); // 2 Nachkommastellen
// Logger::log(LogLevel::Info, "Feuchtigkeit: " + String(actHumidity, 2) + "%"); // 2 Nachkommastellen