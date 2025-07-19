#include <FastLED.h>
#include "Settings.h"

/*
  ___________________________
  | xxx           xx       x |
  | x x x x x x x x x x x    |
  | x x x x x x x x x x x  x |
  | x x x x x x x x x x x    |
  | x x x x x x x x x x x  x |
  | x x x x x x x x x x x    |
  | x x x x x x x x x x x    |
  | x x x x x x x x x x x  x |
  | x x x x x x x x x x x    |
  | x x x x x x x x x x x  x |
  | x x x x x x x x x x x    |
  |   x             x      x |
  |__________________________|

  ___________________________
  | __3__         2_       _ |
  | _____________________    |
  | |        11         |  _ |
  | |                   |    |
  | |                   |  _ |
  | |                   |  6 |
  | | 10     (110)      |    |
  | |                   |  _ |
  | |                   |    |
  | |                   |  _ |
  | | _________________ |    |
  |   1             1      _ |
  |__________________________|

*/

// Definiere die LED-Streifen-Variable
CRGB leds[NUM_LEDS];

class LedStrip {
public:
    // Konstruktor: Initialisiert den LED-Streifen
    LedStrip(int pin, int numLeds) {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, numLeds);
        FastLED.setBrightness(LED_BRIGHTNES); // Standardhelligkeit, kann angepasst werden
        clearAll(); // Alle LEDs beim Start ausschalten
    }

    // Methode zum Ansteuern einer einzelnen LED
    // @param index: Der Index der LED (0-basiert)
    // @param r: Rot-Wert (0-255)
    // @param g: Grün-Wert (0-255)
    // @param b: Blau-Wert (0-255)
    void setSingleLED(int index, byte r, byte g, byte b) {
        if (index >= 0 && index < NUM_LEDS) {
            leds[index] = CRGB(r, g, b);
            FastLED.show();
        }
    }

    // Methode zum Ansteuern einer Gruppe von aufeinanderfolgenden LEDs
    // @param startIndex: Der Start-Index der Gruppe (0-basiert)
    // @param count: Die Anzahl der LEDs in der Gruppe
    // @param r: Rot-Wert (0-255)
    // @param g: Grün-Wert (0-255)
    // @param b: Blau-Wert (0-255)
    void setGroupLEDs(int startIndex, int count, byte r, byte g, byte b) {
        for (int i = 0; i < count; i++) {
            int currentLEDIndex = startIndex + i;
            if (currentLEDIndex >= 0 && currentLEDIndex < NUM_LEDS) {
                leds[currentLEDIndex] = CRGB(r, g, b);
            }
        }
        FastLED.show();
    }

    // Methode zum Ausschalten einer einzelnen LED
    void clearSingleLED(int index){
      setSingleLED(index, 0, 0, 0);
    }

    // Methode zum Ausschalten einer Gruppe von LEDs
    void clearGroupLEDs(int index, int count){
      setGroupLEDs(index, count, 0, 0, 0);
    }

    // Methode zum Ausschalten aller LEDs
    void clearAll() {
        FastLED.clear();
        FastLED.show();
    }
};
