#include <Wire.h>
#include <PCF8574.h>

class SevenSegmentDisplay {
public:
    // Konstruktor: Nimmt die PCF8574 Instanz und die Zuordnung der Segmente entgegen.
    // pinMap: Array [a,b,c,d,e,f,g]
    // dpPin: Der PCF8574 Pin, an den der Dezimalpunkt (DP) angeschlossen ist.
    SevenSegmentDisplay(PCF8574& pcf, const int* pinMap, int dpPin) 
      : _pcf(pcf), _pinMap(pinMap), _dpPin(dpPin) {
        _pcf.begin();
        allSegmentsOff(); // Erstmal alle Segmente ausschalten.
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

        // Segmentmuster für eine gemeinsame Kathoden-Anzeige (High = Segment an)
        // Segmentreihenfolge: a, b, c, d, e, f, g
        const byte segmentPatterns[10][7] = {
            // Ziffer 0: a,b,c,d,e,f,g
            {1,1,1,1,1,1,0}, // 0
            {0,1,1,0,0,0,0}, // 1
            {1,1,0,1,1,0,1}, // 2
            {1,1,1,1,0,0,1}, // 3
            {0,1,1,0,0,1,1}, // 4
            {1,0,1,1,0,1,1}, // 5
            {1,0,1,1,1,1,1}, // 6
            {1,1,1,0,0,0,0}, // 7
            {1,1,1,1,1,1,1}, // 8
            {1,1,1,1,0,1,1}  // 9
        };

        uint8_t outputByte = 0; // Das Byte, das an den PCF8574 gesendet wird

        // Durchlaufen Sie die Segmente (a-g) und setzen Sie die entsprechenden Bits im outputByte
        for (int i = 0; i < 7; i++) {
            int pcfPin = _pinMap[i]; // PCF8574 Pin für das aktuelle Segment
            byte state = segmentPatterns[digit][i]; // Zustand des Segments (an/aus)

            if (state == 1) {
                // Setze das Bit für den Segment-Pin auf HIGH (für Common Cathode)
                outputByte |= (1 << pcfPin);
            } else {
                // Setze das Bit für den Segment-Pin auf LOW
                outputByte &= ~(1 << pcfPin);
            }
        }
        
        // Dezimalpunkt steuern
        if (showDecimalPoint) {
            // Setze das Bit für den DP-Pin auf HIGH (für Common Cathode)
            outputByte |= (1 << _dpPin);
        } else {
            // Setze das Bit für den DP-Pin auf LOW
            outputByte &= ~(1 << _dpPin);
        }

        // Senden Sie das gesamte Byte an den PCF8574
        _pcf.write8(outputByte);
    }

    // Methode zum Ausschalten aller Segmente und des Dezimalpunkts
    void allSegmentsOff() {
        uint8_t outputByte = 0; // Standardmäßig alle Bits auf 0

        // Stellen Sie sicher, dass die Bits für die tatsächlich verwendeten Pins auf 0 sind.
        // Das ist wichtig, falls outputByte nicht mit 0xFF (für Common Anode) initialisiert wurde.
        for (int i = 0; i < 7; i++) {
            outputByte &= ~(1 << _pinMap[i]); // Setze das Bit für diesen Segment-Pin auf 0
        }
        outputByte &= ~(1 << _dpPin); // Setze das Bit für den DP-Pin auf 0

        _pcf.write8(outputByte);
    }

private:
    PCF8574& _pcf;          // Referenz auf die PCF8574 Instanz
    const int* _pinMap;     // Zeiger auf das Pin-Mapping Array für Segmente a-g
    int _dpPin;             // Der PCF8574 Pin für den Dezimalpunkt
};