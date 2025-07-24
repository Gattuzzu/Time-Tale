#include <Arduino.h>
#include <WiFi.h> // Wichtig: Für WiFi.status() etc.

// Deine Bibliotheken
#include "logger/Logger.h"
#include "logger/LogLevel.h"

#include "i2cbus/seven_segment/SevenSegmentDisplay.h"
#include "led/led_strip/LedStrip.h"
#include "i2cbus/sensor/TempHumi.h"
#include "i2cbus/sensor/AirQuality.h"
#include "webservice/api/weather/WeatherData.h"
#include "webservice/api/pollen/PollenData.h"
#include "webservice/configuration/ConfigurationPortal.h"

// Geheimnisse (API-Schlüssel, Koordinaten) und Einstellungen (NTP, AP-Details)
#include "Secrets.h" // Enthält GOOGLE_ACCESS_TOKEN, LATITUDE, LONGITUDE
#include "Settings.h" // Enthält NTP_SERVER, TIME_OFFSET, UPDATE_INTERVALL, AP_SSID, AP_PASSWORD
#include "display/UpdateDisplay.h"

// Lokale Speicher für API-Daten
WeatherData currentWeatherData;
PollenData currentPollenData;

// Globale Variablen für WLAN-Anmeldedaten (werden aus NVS geladen)
String savedWifiSsid = "";
String savedWifiPassword = "";

// Sieben Segment Anzeigen
SevenSegmentDisplay* sevenSegmentDisplays[5]; 
const uint8_t PCF_ADDRESSES[5] = {0x20, 0x21, 0x22, 0x23, 0x24};

// Feuchtigkeitssensor
TempHumi* tempHumi;
AirQuality* airQuality;

// LED-Streifen
LedStrip* myLedStrip;

// Anzeige auf dem Display
UpdateDisplay* updateDisplay;

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

void setup() {
    // Eingänge für Buttons Initialisieren
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);
    
    Serial.begin(9600);
    // while(!Serial);
    delay(2000);

    Logger::setup(LOG_LEVEL); // Logger initialisieren (ohne NTP, da noch nicht verfügbar)
    Logger::log(LogLevel::Info, "Programm gestartet und Logger initialisiert.");

    // Initialisiere ConfigurationPortal Singleton, damit es Preferences öffnen kann
    ConfigurationPortal::getInstance();

    // Lade initial die WLAN-Konfiguration aus NVS
    // Die Zustandsmaschine im loop() übernimmt den Rest.

    // i2c Bus initialiseren
    Logger::log(LogLevel::Info, "i2c Bus starten...");
    Wire.begin(21, 22);
    Wire.setClock(100000L); // "100000L" (100 kHz)
    Logger::log(LogLevel::Info, "i2c Bus gestartet!");

    // 7-Segment Anzeige Initialisieren
    for(int i = 0; i < 5; i++){
      // Sieben Segment Anzeigen erstellen
      sevenSegmentDisplays[i] = new SevenSegmentDisplay(PCF_ADDRESSES[i]);

      // Test
      sevenSegmentTest(*sevenSegmentDisplays[i]);
    }

    // LED-Streifen Initialisieren
    myLedStrip = new LedStrip(LED_PIN, NUM_LEDS);
    ledStripTest();
    myLedStrip->setSingleLED(1, 255, 0, 0); // °C
    myLedStrip->setSingleLED(8, 0, 0, 255); // %

    // Sensoren
    tempHumi = new TempHumi();
    airQuality = new AirQuality(&Wire, 0x76);
    // Versuche, den Sensor zu initialisieren
    if (!airQuality->begin()) {
      Logger::log(LogLevel::Error, "Air Qualitäts Sensor konnte nicht gestartet werden!");
    }
    
    Logger::log(LogLevel::Info, "Ende vom Setup!");

}

void loop() {
  byte error, address;
  int nDevices;

  float actTemperature;
  float actHumidity;
  if (tempHumi->readData(actTemperature, actHumidity)) {
    Logger::log(LogLevel::Info, "Temperatur: " + String(actTemperature, 2) + "°C"); // 2 Nachkommastellen
    Logger::log(LogLevel::Info, "Feuchtigkeit: " + String(actHumidity, 2) + "%"); // 2 Nachkommastellen

    // Temperatur Anzeigen auf 7 Segment Anzeige
    updateDisplay->updateInTemp(actTemperature);
    updateDisplay->updateInTempLED();

    // Luftfeuchtigkeit Anzeigen auf 7 Segment Anzeige
    updateDisplay->updateInHumi(actHumidity);
    updateDisplay->updateInHumiLED();

  } else {
    Logger::log(LogLevel::Error, "Fehler beim Lesen der SHT30(TempHumi) Daten.");
  }

  if (airQuality->readSensorData()) {
    Serial.print("Temperatur = ");
    Serial.print(airQuality->getTemperature());
    Serial.println(" *C");

    Serial.print("Luftdruck = ");
    Serial.print(airQuality->getPressure());
    Serial.println(" hPa");

    Serial.print("Luftfeuchtigkeit = ");
    Serial.print(airQuality->getHumidity());
    Serial.println(" %");

    Serial.print("Gaswiderstand = ");
    Serial.print(airQuality->getGasResistance());
    Serial.println(" Ohm");

    // Die IAQ-Berechnung ist hier sehr vereinfacht!
    Serial.print("Vereinfachter IAQ = ");
    Serial.print(airQuality->getIAQ());
    Serial.println(" (0-100)");

    Serial.println();

    // Anzeigen der Luftqualität
    float iaqValue = airQuality->getIAQ();
    // Sicherstellen, dass der IAQ-Wert im gültigen Bereich liegt
    if (iaqValue < 0.0) iaqValue = 0.0;
    if (iaqValue > 100.0) iaqValue = 100.0;

    // Farben definieren
    updateDisplay->updateAirQuality(iaqValue);

    
  }


  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    // Serial.print("Addresse: ");
    // Serial.println(String(address, HEX));

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
  Serial.print(nDevices);
  Serial.print(" gefunden.");
  Serial.println("done\n");

  delay(2000);           // wait 5 seconds for next scan
}