#ifndef POLLEN_DATA_H
#define POLLEN_DATA_H

#include <Arduino.h>

class PollenData {
public:
    PollenData();
    void reset();

    // Direkte Belastungswerte für die gewünschten Pollentypen (0-5)
    int grassPollenLevel; // Belastung für Gras
    int treePollenLevel;  // Belastung für Baum
    int weedPollenLevel;  // Belastung für Kraut

    // Optional: Eine Methode zum Debug-Ausgabe
    // void printToSerial();
};

#endif // POLLEN_DATA_H