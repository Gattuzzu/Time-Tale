// Logger
#include "logger/Logger.h"
#include "logger/LogLevel.h"

#include "i2cbus/seven_segment/SevenSegmentDisplay.h"

#include "led/led_strip/LedStrip.h"

// Sieben Segment Anzeigen
SevenSegmentDisplay* sevenSegmentDisplays[5]; 
const uint8_t PCF_ADDRESSES[5] = {0x20, 0x21, 0x22, 0x23, 0x24};

// LED
LedStrip* myLedStrip;

class UpdateDisplay {
    public:

        // Wert von Temperatur an die Anzeige übergeben.
        void updateInTemp(float temperature) {
            
            Logger::log(LogLevel::Debug, "Temperatur: " + String(temperature, 2) + "°C"); // 2 Nachkommastellen

            // Temperatur Anzeigen auf 7 Segment Anzeige
            int temp = temperature * 10; // Eine Komma Stelle soll angezeigt werden

            if (temp >= 0) {
                sevenSegmentDisplays[0]->displayDigit(temp         % 10);
                sevenSegmentDisplays[1]->displayDigit((temp /  10) % 10, true);

                if(temp >= 100) {
                    sevenSegmentDisplays[2]->displayDigit((temp / 100) % 10);
                }
                else {
                    sevenSegmentDisplays[2]->allSegmentsOff();
                }
            }
            else {
                sevenSegmentDisplays[2]->displayMinus();
                temp = abs(temp);
                if (temp <= 99) {
                    sevenSegmentDisplays[0]->displayDigit(temp         % 10);
                    sevenSegmentDisplays[1]->displayDigit((temp /  10) % 10, true);
                }
                else {
                    sevenSegmentDisplays[0]->displayDigit((temp /  10) % 10);
                    sevenSegmentDisplays[1]->displayDigit((temp / 100) % 10);
                }
            }
            
        }

        // Wert von Feuchtigkeit an die Anzeige übergeben
        void updateInHumi(float humidity) {
            
            Logger::log(LogLevel::Debug, "Feuchtigkeit: " + String(humidity, 2) + "%"); // 2 Nachkommastellen

            // Luftfeuchtigkeit Anzeigen auf 7 Segment Anzeige
            int humi = humidity;

            sevenSegmentDisplays[3]->displayDigit(humi % 10);

            if (humi >= 10) {
                sevenSegmentDisplays[4]->displayDigit((humi / 10) % 10);
            }
            else {
                sevenSegmentDisplays[4]->allSegmentsOff();
            }
            

        }

        // Wert der Luftqualität an die Anzeige übergeben.
        void updateAirQuality(float airQuality){

            Logger::log(LogLevel::Debug, "Feuchtigkeit: " + String(airQuality, 2) + "%"); // 2 Nachkommastellen

            // Farben definieren
            uint8_t r, g, b;

            if (airQuality <= 20.0) {
                // IAQ von 0 bis 20: Immer Rot
                r = 255;
                g = 0;
                b = 0;

            } else if (airQuality >= 90.0) {
                // IAQ von 90 bis 100: Immer Grün
                r = 0;
                g = 255;
                b = 0;

            } else if (airQuality <= 55.0) { // Angepasster Mittelpunkt für den Gelb-Übergang
                // IAQ von 20 bis 60: Rot nach Gelb
                // IAQ 20  -> Rot (255, 0, 0)
                // IAQ 55  -> Gelb (255, 255, 0)
                r = 255;
                g = (uint8_t)map(airQuality, 20, 55, 0, 255);
                b = 0;

            } else {
                // IAQ von 60 bis 100: Gelb nach Grün
                // IAQ 55  -> Gelb (255, 255, 0)
                // IAQ 90 -> Grün (0, 255, 0)
                r = (uint8_t)map(airQuality, 55, 90, 255, 0);
                g = 255;
                b = 0;
            }
            myLedStrip->setSingleLED(0, r, g, b);

        }

        // Wert des Wetters an die Anzeige übergeben.
        void updateWeather(){

        }

        // Wert von Temperatur von draussen an die Anzeige übergeben.
        void updateOutTemp() {

        }

        // Wert von Feuchtigkeit von draussen an die Anzeige übergeben.
        void updateOutHumi() {

        }

        // Wert der Pollen an die Anzeige übergeben.
        void updatePollen() {

        }

        // Wert der Zeit an die Anzeige übergeben.
        void updateTime() {

        }
        
        //
        void updateInTempLED() {
            myLedStrip->setSingleLED(1, 255, 255, 0);
        }

        //
        void updateOutTempLED() {
            myLedStrip->setSingleLED(1, 255, 0, 0);
        }

        //
        void updateInHumiLED() {
            myLedStrip->setSingleLED(8, 255, 255, 0);
        }

        //
        void updateOutHumiLED() {
            myLedStrip->setSingleLED(8, 255, 0, 0);
        }
};