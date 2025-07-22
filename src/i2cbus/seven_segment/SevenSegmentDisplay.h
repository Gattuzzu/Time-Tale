#include <Wire.h>
#include <PCF8574.h>
#include <logger/LogLevel.h>

class SevenSegmentDisplay {
public:
    // Konstruktor: Nimmt die PCF8574 Instanz und die Zuordnung der Segmente entgegen.
    // pinMap: Array [a,b,c,d,e,f,g]
    // dpPin: Der PCF8574 Pin, an den der Dezimalpunkt (DP) angeschlossen ist.
    SevenSegmentDisplay(uint8_t pcfAddress)
      : _pcf(pcfAddress){
        _pcf.begin();
        allSegmentsOff(); // Erstmal alle Segmente ausschalten
    }

    // Methode zum Anzeigen einer Ziffer (0-9) und Steuern des Dezimalpunkts
    // digit: Die anzuzeigende Ziffer (0-9).
    // showDecimalPoint: true, um den Dezimalpunkt anzuzeigen, false, um ihn auszublenden.
    void displayDigit(int digit, bool showDecimalPoint = false) { // showDecimalPoint ist optional und standardmäßig false
        if (digit < 0 || digit > 9) {
            // Ungültige Ziffer, alle Segmente aus, Punkt aus
            allSegmentsOff();
            return;
        }

        //                    Normal {231,  33, 203, 107,  45, 110, 238,  39, 239, 111}
        // Invertieren der Ausgänge  [255 - Zahl]
        uint8_t digitToSegment[10] = { 24, 222,  52, 148, 210, 145,  17, 216,  16, 144};

        uint8_t outputByte = digitToSegment[digit];

        if(showDecimalPoint){
            // Bit für den Dezimal Punkt einschalten
            outputByte -= 16;
        }

        Logger::log(LogLevel::Debug, "Zahl: " + String(digit)); // Umwandlung zu String für Konkatenation
        Logger::log(LogLevel::Debug, "OutputByte: " + String(outputByte)); // Umwandlung zu String
        // Senden Sie das gesamte Byte an den PCF8574
        _pcf.write8(outputByte);
    }

    // Methode zum Ausschalten aller Segmente und des Dezimalpunkts
    // Bedeutet jetzt: Alle Ausgänge auf HIGH setzen, sodass kein Segment leuchtet.
    void allSegmentsOff() {
        uint8_t outputByte = 0xFF; // Setze alle Bits auf 1 (HIGH), um alle Segmente auszuschalten (Common Anode)
        Logger::log(LogLevel::Debug, "7-Segment Anzeige aus.");

        // Senden Sie das gesamte Byte an den PCF8574
        _pcf.write8(outputByte);
    }

private:
    PCF8574 _pcf;           // Referenz auf die PCF8574 Instanz
};