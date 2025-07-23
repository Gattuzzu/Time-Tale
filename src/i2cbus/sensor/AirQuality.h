#include <Wire.h> // Arduino I2C Bibliothek

#define SGP30_DEFAULT_ADDRESS 0x58 // I2C-Adresse für den SGP30 Sensor

// SGP30 Befehle (aus dem Datenblatt)
#define SGP30_INIT_AIR_QUALITY      0x2003 // Initialisiere Air Quality Messung
#define SGP30_MEASURE_AIR_QUALITY   0x2008 // Messe CO2eq und TVOC
#define SGP30_GET_BASELINE          0x2015 // Lese Baseline-Werte aus
#define SGP30_SET_BASELINE          0x201E // Setze Baseline-Werte
#define SGP30_MEASURE_TEST          0x2032 // Selbsttest
#define SGP30_SOFT_RESET            0x0006 // Software Reset

class AirQuality {
public:
    /**
     * @brief Konstruktor für die SGP30-Klasse.
     * @param address Die I2C-Adresse des SGP30-Sensors (Standard ist 0x58).
     */
    AirQuality(uint8_t address = SGP30_DEFAULT_ADDRESS) : _address(address) {}

    /**
     * @brief Initialisiert den SGP30-Sensor und startet die Messung.
     * Muss im setup() des Arduino aufgerufen werden.
     * @return true, wenn der Sensor gefunden und initialisiert wurde, false sonst.
     */
    bool begin() {
        Wire.begin(); // Initialisiere die I2C-Kommunikation
        Wire.beginTransmission(_address);
        if (Wire.endTransmission() != 0) {
            return false; // Sensor nicht gefunden
        }

        // Führe Initialisierungsbefehl aus
        return sendCommand(SGP30_INIT_AIR_QUALITY);
    }

    /**
     * @brief Misst die CO2eq (äquivalenter CO2-Wert) und TVOC (Total Volatile Organic Compounds) Werte.
     * Diese Funktion sollte alle ~1 Sekunde aufgerufen werden, damit der Sensor kalibrieren kann.
     * @param co2eq Referenz für den CO2eq-Wert in ppm.
     * @param tvoc Referenz für den TVOC-Wert in ppb.
     * @return true, wenn die Werte erfolgreich gelesen wurden, false sonst.
     */
    bool readData(uint16_t &co2eq, uint16_t &tvoc) {
        if (!sendCommand(SGP30_MEASURE_AIR_QUALITY)) {
            return false;
        }

        delay(10); // Warte 10ms auf die Messung (siehe Datenblatt)

        if (Wire.requestFrom(_address, (uint8_t)6) != 6) {
            return false; // Konnte nicht alle 6 Bytes lesen
        }

        uint16_t co2eqRaw = Wire.read();
        co2eqRaw = (co2eqRaw << 8) | Wire.read();
        uint8_t crcCO2 = Wire.read(); // CRC-Byte für CO2eq (aktuell nicht geprüft)

        uint16_t tvocRaw = Wire.read();
        tvocRaw = (tvocRaw << 8) | Wire.read();
        uint8_t crcTVOC = Wire.read(); // CRC-Byte für TVOC (aktuell nicht geprüft)

        // Hier könnte eine CRC-Prüfung implementiert werden,
        // um die Datenintegrität zu gewährleisten.

        co2eq = co2eqRaw;
        tvoc = tvocRaw;

        return true;
    }

    /**
     * @brief Liest die aktuelle Baseline der CO2eq- und TVOC-Werte aus.
     * Diese Baseline kann gespeichert und beim nächsten Start des Sensors gesetzt werden.
     * @param baseline_co2eq Referenz für den CO2eq-Baseline-Wert.
     * @param baseline_tvoc Referenz für den TVOC-Baseline-Wert.
     * @return true, wenn die Baseline erfolgreich gelesen wurde, false sonst.
     */
    bool getBaseline(uint16_t &baseline_co2eq, uint16_t &baseline_tvoc) {
        if (!sendCommand(SGP30_GET_BASELINE)) {
            return false;
        }

        delay(10); // Warte auf die Antwort

        if (Wire.requestFrom(_address, (uint8_t)6) != 6) {
            return false;
        }

        baseline_co2eq = Wire.read();
        baseline_co2eq = (baseline_co2eq << 8) | Wire.read();
        Wire.read(); // CRC

        baseline_tvoc = Wire.read();
        baseline_tvoc = (baseline_tvoc << 8) | Wire.read();
        Wire.read(); // CRC

        return true;
    }

    /**
     * @brief Setzt eine gespeicherte Baseline für die CO2eq- und TVOC-Werte.
     * Dies hilft dem Sensor, schneller stabile und genaue Messwerte zu liefern.
     * @param baseline_co2eq Der zu setzende CO2eq-Baseline-Wert.
     * @param baseline_tvoc Der zu setzende TVOC-Baseline-Wert.
     * @return true, wenn die Baseline erfolgreich gesetzt wurde, false sonst.
     */
    bool setBaseline(uint16_t baseline_co2eq, uint16_t baseline_tvoc) {
        Wire.beginTransmission(_address);
        Wire.write((SGP30_SET_BASELINE >> 8) & 0xFF);
        Wire.write(SGP30_SET_BASELINE & 0xFF);

        // CO2eq Baseline
        Wire.write((baseline_co2eq >> 8) & 0xFF);
        Wire.write(baseline_co2eq & 0xFF);
        Wire.write(calculateCrc(baseline_co2eq));

        // TVOC Baseline
        Wire.write((baseline_tvoc >> 8) & 0xFF);
        Wire.write(baseline_tvoc & 0xFF);
        Wire.write(calculateCrc(baseline_tvoc));

        return Wire.endTransmission() == 0;
    }
    
    /**
     * @brief Führt einen Software-Reset des SGP30 durch.
     * @return true, wenn der Reset-Befehl erfolgreich gesendet wurde, false sonst.
     */
    bool softReset() {
        return sendCommand(SGP30_SOFT_RESET);
    }

private:
    uint8_t _address; // Die I2C-Adresse des SGP30-Sensors

    /**
     * @brief Sendet einen 16-Bit-Befehl an den SGP30.
     * @param command Der zu sendende 16-Bit-Befehl.
     * @return true, wenn der Befehl erfolgreich gesendet wurde, false sonst.
     */
    bool sendCommand(uint16_t command) {
        Wire.beginTransmission(_address);
        Wire.write((command >> 8) & 0xFF); // High Byte
        Wire.write(command & 0xFF);      // Low Byte
        return Wire.endTransmission() == 0;
    }

    /**
     * @brief Berechnet das CRC-Checksummen-Byte für 16-Bit-Daten (für Sensirion-Protokoll).
     * @param data Der 16-Bit-Wert, für den das CRC berechnet werden soll.
     * @return Das berechnete 8-Bit-CRC-Checksummen-Byte.
     */
    uint8_t calculateCrc(uint16_t data) {
        uint8_t crc = 0xFF;
        uint8_t byte[2];
        byte[0] = (data >> 8) & 0xFF;
        byte[1] = data & 0xFF;

        for (uint8_t i = 0; i < 2; i++) {
            crc ^= byte[i];
            for (uint8_t bit = 8; bit > 0; --bit) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x31; // 0x31 ist das Sensirion-Polynom
                } else {
                    crc = (crc << 1);
                }
            }
        }
        return crc;
    }
};

/*
 * Beispiel für die Verwendung im Arduino-Sketch (.ino-Datei):
 * * #include "SGP30.h" // Wenn du die Klasse in einer separaten .h/.cpp Datei hast
 * * SGP30 sensor(0x58); // Erstelle ein SGP30-Objekt mit der Standardadresse 0x58
 * * void setup() {
 * Serial.begin(9600); // Starte serielle Kommunikation für Debugging
 * Serial.println("Starte SGP30 Sensor Test...");
 * * if (!sensor.begin()) {
 * Serial.println("SGP30 Sensor nicht gefunden oder initialisiert! Bitte Verkabelung und Adresse prüfen.");
 * while (1); // Stoppe das Programm, wenn der Sensor nicht gefunden wird
 * }
 * Serial.println("SGP30 Sensor gefunden und initialisiert. Bitte 10-15 Sekunden warten, bis Werte stabil sind.");
 * }
 * * void loop() {
 * uint16_t co2eq;
 * uint16_t tvoc;
 * * if (sensor.readData(co2eq, tvoc)) {
 * Serial.print("CO2eq: ");
 * Serial.print(co2eq);
 * Serial.print(" ppm, TVOC: ");
 * Serial.print(tvoc);
 * Serial.println(" ppb");
 * } else {
 * Serial.println("Fehler beim Lesen der SGP30 Daten.");
 * }
 * * // Der SGP30 benötigt alle 1 Sekunde eine Messung, damit die interne Kalibrierung funktioniert.
 * delay(1000); 
 * }
 */