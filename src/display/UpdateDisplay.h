// Logger
#include "logger/Logger.h"
#include "logger/LogLevel.h"

#include "i2cbus/seven_segment/SevenSegmentDisplay.h"

#include "led/led_strip/LedStrip.h"
#include <mp3player/Mp3Player.h>

// Sieben Segment Anzeigen
SevenSegmentDisplay* sevenSegmentDisplays[5]; 
const uint8_t PCF_ADDRESSES[5] = {0x20, 0x21, 0x22, 0x23, 0x24};

// LED
LedStrip* myLedStrip;

// Mp3Player
Mp3Player myMp3Player;

// Variablen
float lastTemp = 0;
CRGB displayColorTime = CRGB::Blue;

class UpdateDisplay {
    public:
        // Wert von Temperatur an die Anzeige übergeben.
        void updateTemperature(float temperature) {
            if(abs(lastTemp - temperature) > 0.1){

                Logger::log(LogLevel::Debug, "Temperatur: " + String(temperature, 2) + "°C"); // 2 Nachkommastellen
                
                // Temperatur Anzeigen auf 7 Segment Anzeige
                int temp = temperature * 10; // Eine Komma Stelle soll angezeigt werden
                lastTemp = static_cast<float>(temp) / 10; // Temperatur in Float umrechnen, aber mit nur einer Komma Zahl.
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
        }

        // Wert von Feuchtigkeit an die Anzeige übergeben
        void updateHumidity(float humidity) {
            
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

            if (airQuality <= 50.0) {
                // IAQ von 0 bis 50: Immer Rot
                r = 255;
                g = 0;
                b = 0;

            } else if (airQuality >= 96.0) {
                // IAQ von 96 bis 100: Immer Grün
                r = 0;
                g = 255;
                b = 0;

            } else if (airQuality <= 73.0) { // Angepasster Mittelpunkt für den Gelb-Übergang
                // IAQ von 50 bis 73: Rot nach Gelb
                // IAQ 50  -> Rot (255, 0, 0)
                // IAQ 73  -> Gelb (255, 255, 0)
                r = 255;
                g = (uint8_t)map(airQuality, 50, 73, 0, 255);
                b = 0;

            } else {
                // IAQ von 73 bis 96: Gelb nach Grün
                // IAQ 73  -> Gelb (255, 255, 0)
                // IAQ 96 -> Grün (0, 255, 0)
                r = (uint8_t)map(airQuality, 73, 96, 255, 0);
                g = 255;
                b = 0;
            }
            myLedStrip->setSingleLED(0, r, g, b);

        }

        // Wert des Wetters an die Anzeige übergeben.
        void updateWeather(WeatherConditionType weather) {
            myLedStrip->clearGroupLEDs(2, 6, false);
            switch(weather){
                // case WeatherConditionType::TYPE_UNSPECIFIED: ; break;
                case WeatherConditionType::CLEAR: myLedStrip->setSingleLED(7, 255, 255, 0); break;
                // case WeatherConditionType::MOSTLY_CLEAR: ; break;
                case WeatherConditionType::PARTLY_CLOUDY: myLedStrip->setSingleLED(6, 143, 139, 102); break;
                // case WeatherConditionType::MOSTLY_CLOUDY: ; break;
                case WeatherConditionType::CLOUDY: myLedStrip->setSingleLED(5, 128, 128, 128); break;
                // case WeatherConditionType::WINDY: ; break;
                // case WeatherConditionType::WIND_AND_RAIN: ; break;
                // case WeatherConditionType::LIGHT_RAIN_SHOWERS: ; break;
                // case WeatherConditionType::CHANCE_OF_SHOWERS: ; break;
                // case WeatherConditionType::SCATTERED_SHOWERS: ; break;
                // case WeatherConditionType::RAIN_SHOWERS: ; break;
                // case WeatherConditionType::HEAVY_RAIN_SHOWERS: ; break;
                // case WeatherConditionType::LIGHT_TO_MODERATE_RAIN: ; break;
                // case WeatherConditionType::MODERATE_TO_HEAVY_RAIN: ; break;
                case WeatherConditionType::RAIN: myLedStrip->setSingleLED(4, 0, 0, 255); break;
                // case WeatherConditionType::LIGHT_RAIN: ; break;
                // case WeatherConditionType::HEAVY_RAIN: ; break;
                // case WeatherConditionType::RAIN_PERIODICALLY_HEAVY: ; break;
                // case WeatherConditionType::LIGHT_SNOW_SHOWERS: ; break;
                // case WeatherConditionType::CHANCE_OF_SNOW_SHOWERS: ; break;
                // case WeatherConditionType::SCATTERED_SNOW_SHOWERS: ; break;
                // case WeatherConditionType::SNOW_SHOWERS: return ; break;
                // case WeatherConditionType::HEAVY_SNOW_SHOWERS: ; break;
                // case WeatherConditionType::LIGHT_TO_MODERATE_SNOW: ; break;
                // case WeatherConditionType::MODERATE_TO_HEAVY_SNOW: ; break;
                case WeatherConditionType::SNOW: myLedStrip->setSingleLED(2, 255, 255, 255); break;
                // case WeatherConditionType::LIGHT_SNOW: ; break;
                // case WeatherConditionType::HEAVY_SNOW: ; break;
                // case WeatherConditionType::SNOWSTORM: ; break;
                // case WeatherConditionType::SNOW_PERIODICALLY_HEAVY: ; break;
                // case WeatherConditionType::HEAVY_SNOW_STORM: ; break;
                // case WeatherConditionType::BLOWING_SNOW: ; break;
                // case WeatherConditionType::RAIN_AND_SNOW: ; break;
                // case WeatherConditionType::HAIL: ; break;
                // case WeatherConditionType::HAIL_SHOWERS: ; break;
                case WeatherConditionType::THUNDERSTORM: myLedStrip->setSingleLED(3, 255, 255, 0); break;
                // case WeatherConditionType::THUNDERSHOWER: ; break;
                // case WeatherConditionType::LIGHT_THUNDERSTORM_RAIN: ; break;
                // case WeatherConditionType::SCATTERED_THUNDERSTORMS: ; break;
                // case WeatherConditionType::HEAVY_THUNDERSTORM: ; break;
                case WeatherConditionType::UNKNOWN: myLedStrip->clearGroupLEDs(2, 6); break; // Fallback -> Nichts anzeigen break;
            }
        }

        // Wert der Pollen an die Anzeige übergeben.
        void updatePollen(int maxPollenLevel) {
            if(maxPollenLevel >= 0 || maxPollenLevel <= 5){

                int r, g, b; 
                r = map(maxPollenLevel, 0, 5, 0, 255); // Rotanteil steigt von 0 (Grün) auf 255 (Rot)
                g = map(maxPollenLevel, 0, 5, 255, 0); // Grünanteil sinkt von 255 (Grün) auf 0 (Rot)
                b = 0;                                 // Blau ist immer 0
                myLedStrip->setGroupLEDs(9, 3, r, g, b);

            } else{
                myLedStrip->clearGroupLEDs(9, 3);
            }
        }

        // Wert der Zeit an die Anzeige übergeben.
        void updateTime(int hour, int min) {

            int r, g, b;
            r = displayColorTime.r;
            g = displayColorTime.g;
            b = displayColorTime.b;

            // Alle LEDs erst Lichter ausstellen
            myLedStrip->clearGroupLEDs(12, 111, false);
            // ES ISCH
            myLedStrip->setGroupLEDs(21, 2, r, g, b, false); // ES
            myLedStrip->setGroupLEDs(16, 4, r, g, b, false); // ISCH

            // Minuten in fünfer Schritten anzeigen
            if ((min >= 5 && min <= 9) || (min >= 25 && min <= 29) || (min >= 35 && min <= 39) || (min >= 55 && min <= 59)) {
                myLedStrip->setGroupLEDs(12, 3, r, g, b, false);                       // FÜF
            }
            else if ((min >= 10 && min <= 14) || (min >= 50 && min <= 54)) {
                myLedStrip->setGroupLEDs(23, 3, r, g, b, false);                       // ZÄÄ
            }
            else if ((min >= 15 && min <= 19) || (min >= 45 && min <= 49)) {
                myLedStrip->setGroupLEDs(27, 6, r, g, b, false);                       // VIERTU
            }
            else if ((min >= 20 && min <= 24) || (min >= 40 && min <= 44)) {
                myLedStrip->setGroupLEDs(38, 6, r, g, b, false);                       // ZWÄNZG
            }
            if (min >= 25 && min <= 39) {
                myLedStrip->setGroupLEDs(50, 5, r, g, b, false);                       // HAUBI
            }
            

            // AB, VOR oder keines von beiden
            if((min >= 5 && min <= 24) || (min >= 35 && min <= 39)) {
                myLedStrip->setGroupLEDs(34, 2, r, g, b, false);                       // AB    
            }
            else if ((min >= 25 && min <= 29) || (min >= 40 && min <= 59)) {
                myLedStrip->setGroupLEDs(45, 3, r, g, b, false);                       // VOR
            }
            else {
                                                                                // Haubi und Punkt
            }
            
            
            //Stunden anzeigen

            // Ab der 25. Minute wird die nachfolgende Stunde angezeigt ( z.B. 6:35 ist FÜF AB HALBI SIBNI)
            if (min >= 25) {
                hour += 1;
            }

            // Wir zeigen die Stunden 1 - 12 an, deshalb für alle zahlen ab 13 werden -12
            if (hour > 12) {
                hour -= 12;
            }

            // 0 Uhr wird als 12 Uhr angezeigt
            if (hour == 0) {
                hour += 12;
            }

            if      (hour == 1)      myLedStrip->setGroupLEDs(59, 3, r, g, b);   // EIS
            else if (hour == 2)      myLedStrip->setGroupLEDs(63, 4, r, g, b);   // ZWÖI
            else if (hour == 3)      myLedStrip->setGroupLEDs(56, 3, r, g, b);   // DRÜ
            else if (hour == 4)      myLedStrip->setGroupLEDs(72, 5, r, g, b);   // VIERI
            else if (hour == 5)      myLedStrip->setGroupLEDs(67, 4, r, g, b);   // FÜFI
            else if (hour == 6)      myLedStrip->setGroupLEDs(78, 6, r, g, b);   // SÄCHSI
            else if (hour == 7)      myLedStrip->setGroupLEDs(84, 5, r, g, b);   // SIBNI
            else if (hour == 8)      myLedStrip->setGroupLEDs(95, 5, r, g, b);   // ACHTI
            else if (hour == 9)      myLedStrip->setGroupLEDs(91, 4, r, g, b);   // NÜNI
            else if (hour == 10)     myLedStrip->setGroupLEDs(101, 4, r, g, b);  // ZÄNI
            else if (hour == 11)     myLedStrip->setGroupLEDs(106, 4, r, g, b);  // EUFI
            else if (hour == 12)     myLedStrip->setGroupLEDs(115, 6, r, g, b);  // ZWÖUFI

            // Soundausgabe
            if (min == 0) {
                myMp3Player.play(hour);
            }
   
        }

        // Farbe der Zeitanzeige einstellen
        void setColorTime(int r, int g, int b){
            displayColorTime = CRGB(r, g, b);
        }

        // Helligkeit alles LED einstellen
        // Erwartet Wert von 0-100 und mappt ihn nach 0-255 um
        void setBrightness(int brightness){
            brightness = map(brightness, 0, 100, 0, 255);
            myLedStrip->setBrightness(brightness);
        }
        
        //
        void updateTempLED(boolean isIndoor) {
            if(isIndoor) {
                myLedStrip->setSingleLED(1, 255, 255, 0);
            }
            else {
                myLedStrip->setSingleLED(1, 255, 0, 0);
            }
        }

        //
        void updateHumiLED(boolean isIndoor) {
            if(isIndoor) {
                myLedStrip->setSingleLED(8, 255, 255, 0);
            }
            else {
                myLedStrip->setSingleLED(8, 255, 0, 0);
            }
        }
        
        // Menu anzeigen
        void showMenu(){
            myLedStrip->clearAll();
            myLedStrip->setGroupLEDs(112, 4, displayColorTime.r, displayColorTime.g, displayColorTime.b);
        }

        // Aktueller Menu Punkt anzeigen
        void showActMenuPoint(int menuPoint){
            myLedStrip->clearSingleLED(8);
            sevenSegmentDisplays[3]->displayDigit(menuPoint);
            sevenSegmentDisplays[4]->allSegmentsOff();
        }

        // IP-Adresse anzeigen
        // Die IP Adresse muss in teilen zu je 3 Digits übergeben werden.
        void showIPAddress(int ipAddressPart, boolean showPoint = true){
            myLedStrip->clearSingleLED(1);
            sevenSegmentDisplays[0]->displayDigit( ipAddressPart        % 10);
            sevenSegmentDisplays[1]->displayDigit((ipAddressPart / 10)  % 10);
            sevenSegmentDisplays[2]->displayDigit((ipAddressPart / 100) % 10, showPoint);
        }

        // Testen der Sieben Segment Anzeige
        void sevenSegmentTest(SevenSegmentDisplay displays){
            for(int i = 0; i <= 10; i++){
                if(i == 10){
                displays.displayDigit(9, true);
                } else{
                displays.displayDigit(i);
                }
                delay(250);
            }
            delay(250);
            displays.allSegmentsOff();
        }

        // Testen des LED Strip
        void ledStripTest(){
            myLedStrip->clearAll();
            delay(200);
            for(int i = 0; i < NUM_LEDS; i++) {
                // LED-Streifen testen
                myLedStrip->setSingleLED(i, 0, 0, 255); // Blaue LED
                delay(200);
                myLedStrip->clearSingleLED(i); // LED wieder ausschalten
            } 
        }

        // Testen der Zeitanzeige
        void textUhrTest(){
            for(int hour = 0; hour <= 12; hour++){
                for(int min = 0; min < 60; min = min + 5){
                    updateTime(hour, min);
                    delay(1000);
                }
            }
        }

};