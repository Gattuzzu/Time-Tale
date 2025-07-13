#include "PollenData.h"

PollenData::PollenData() {
    reset();
}

void PollenData::reset() {
    grassPollenLevel = -1; // -1 als Indikator, dass der Wert noch nicht gesetzt wurde
    treePollenLevel  = -1;
    weedPollenLevel  = -1;
}

// void PollenData::printToSerial() {
//     Serial.println("--- Pollen Data ---");
//     Serial.print("Grass Pollen Level: "); Serial.println(grassPollenLevel);
//     Serial.print("Tree Pollen Level: "); Serial.println(treePollenLevel);
//     Serial.print("Weed Pollen Level: "); Serial.println(weedPollenLevel);
//     Serial.println("--- End Pollen Data ---");
// }