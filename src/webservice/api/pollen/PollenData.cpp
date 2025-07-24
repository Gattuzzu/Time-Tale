#include "PollenData.h"

PollenData::PollenData() {
    reset();
}

void PollenData::reset() {
    grassPollenLevel = -1; // -1 als Indikator, dass der Wert noch nicht gesetzt wurde
    treePollenLevel  = -1;
    weedPollenLevel  = -1;
}

String PollenData::toString() {
    String s = "--- Pollen Data ---";
           s += "\nGrass Pollen Level: " + String(grassPollenLevel);
           s += "\nTree Pollen Level: " + String(treePollenLevel);
           s += "\nWeed Pollen Level: " + String(weedPollenLevel);
           s += "\n--- End Pollen Data ---";
    return s;
}