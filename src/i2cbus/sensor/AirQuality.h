#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#define SEALEVELPRESSURE_HPA (1013.25) // Standard Meeresspiegeldruck in hPa

class AirQuality {
public:
    // Konstruktor: Nimmt das Wire-Objekt und die I2C-Adresse entgegen
    AirQuality(TwoWire* wire, uint8_t addr) :
        bme(wire), // Initialisiert das Adafruit BME680 Objekt mit dem übergebenen Wire-Objekt
        _i2cAddress(addr),
        _initialized(false) {}

    // Initialisiert den Sensor
    bool begin() {
        if (!bme.begin(_i2cAddress)) {
            Serial.println("Konnte BME680 nicht finden. Überprüfen Sie die Verkabelung und Adresse!");
            _initialized = false;
            return false;
        }

        // Optional: Sensoreinstellungen anpassen
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150); // 320 C für 150 ms
        _initialized = true;
        Serial.println("BME680 initialisiert.");
        return true;
    }

    // Liest alle Sensordaten und gibt true zurück, wenn erfolgreich
    bool readSensorData() {
        if (!_initialized) {
            Serial.println("Sensor nicht initialisiert.");
            return false;
        }
        if (!bme.performReading()) {
            Serial.println("Fehler beim Auslesen des BME680 Sensors!");
            return false;
        }
        return true;
    }

    // Gibt die Temperatur in Grad Celsius zurück
    float getTemperature() {
        return bme.temperature;
    }

    // Gibt die Luftfeuchtigkeit in Prozent zurück
    float getHumidity() {
        return bme.humidity;
    }

    // Gibt den Luftdruck in hPa (Hektopascal) zurück
    float getPressure() {
        return bme.pressure / 100.0; // Umrechnung von Pascal zu hPa
    }

    // Gibt den Gaswiderstand in Ohm zurück
    int getGasResistance() {
        return bme.gas_resistance;
    }

    // Berechnet und gibt den IAQ (Indoor Air Quality) Index zurück
    // Hinweis: Die Berechnung des IAQ ist komplexer und benötigt typischerweise
    // eine längere Kalibrierungsphase und eine spezifische Logik.
    // Hier ist eine sehr einfache (nicht-standardisierte) Berechnung,
    // die du anpassen musst, um genaue Ergebnisse zu erhalten.
    float getIAQ() {
        // Dies ist eine sehr vereinfachte IAQ-Berechnung.
        // Für eine genauere IAQ, die den BME680 optimal nutzt,
        // solltest du die Boschs BSEC Bibliothek in Betracht ziehen.
        // (https://github.com/boschsensortec/BSEC-Arduino-Library)

        // Beispiel einer vereinfachten IAQ-Berechnung basierend auf Gaswiderstand und Luftfeuchtigkeit
        // Annahme: Höherer Gaswiderstand bei geringer Luftfeuchtigkeit bedeutet bessere Luftqualität
        // (Dies ist nur ein Platzhalter und nicht wissenschaftlich fundiert!)

        float iaq = 0.0;
        if (bme.gas_resistance > 0) {
            iaq = map(bme.gas_resistance, 0, 1000000, 100, 0); // Grobe Schätzung: Höherer Widerstand -> besser (0-100)
            iaq = constrain(iaq, 0, 100); // Werte auf 0-100 begrenzen
        }
        // Du könntest hier auch die Luftfeuchtigkeit einbeziehen, z.B.
        // iaq = iaq * (1 - (bme.humidity / 100.0));
        return iaq;
    }

private:
    Adafruit_BME680 bme;        // Adafruit BME680 Objekt
    uint8_t _i2cAddress;        // I2C Adresse des Sensors
    bool _initialized;          // Flag, ob der Sensor initialisiert wurde
};

    // Serial.print("Temperatur = ");
    // Serial.print(airQuality->getTemperature());
    // Serial.println(" *C");

    // Serial.print("Luftdruck = ");
    // Serial.print(airQuality->getPressure());
    // Serial.println(" hPa");

    // Serial.print("Luftfeuchtigkeit = ");
    // Serial.print(airQuality->getHumidity());
    // Serial.println(" %");

    // Serial.print("Gaswiderstand = ");
    // Serial.print(airQuality->getGasResistance());
    // Serial.println(" Ohm");

    // // Die IAQ-Berechnung ist hier sehr vereinfacht!
    // Serial.print("Vereinfachter IAQ = ");
    // Serial.print(airQuality->getIAQ());
    // Serial.println(" (0-100)");

    // Serial.println();