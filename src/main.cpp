#include <Arduino.h>
#include <WiFi.h> // Wichtig: Für WiFi.status() etc.

// Deine Bibliotheken
#include "logger/Logger.h"
#include "logger/LogLevel.h"

#include "i2cbus/sensor/TempHumi.h"
#include "i2cbus/sensor/AirQuality.h"
#include "webservice/api/weather/WeatherClient.h"
#include "webservice/api/pollen/PollenClient.h"
#include "webservice/configuration/ConfigurationPortal.h"
#include "webservice/ntp/NTPTimeSync.h"
#include "display/UpdateDisplay.h"

// Geheimnisse (API-Schlüssel, Koordinaten) und Einstellungen (NTP, AP-Details)
#include "Secrets.h" // Enthält GOOGLE_ACCESS_TOKEN, LATITUDE, LONGITUDE
#include "Settings.h" // Enthält NTP_SERVER, TIME_OFFSET, UPDATE_INTERVALL, AP_SSID, AP_PASSWORD

// Lokale Speicher für API-Daten
WeatherData currentWeatherData;
PollenData currentPollenData;

// Globale Variablen für WLAN-Anmeldedaten (werden aus NVS geladen)
String savedWifiSsid = "";
String savedWifiPassword = "";

// Feuchtigkeitssensor
TempHumi* tempHumi;
AirQuality* airQuality;

// Zustände des Geräts
enum DeviceState {
    STATE_INITIALIZING,         // Beim Start: Lade Konfiguration und versuche WLAN-Verbindung
    STATE_AP_MODE,              // Gerät ist im Konfigurations-AP-Modus
    STATE_CONNECTING_WIFI,      // Gerät versucht, sich mit dem konfigurierten WLAN zu verbinden
    STATE_NORMAL_OPERATION,     // Gerät ist mit WLAN verbunden und führt normale Aufgaben aus
    STATE_WIFI_CONNECTION_LOST  // WLAN-Verbindung wurde unterbrochen, versuche Reconnect
};

DeviceState currentState = STATE_INITIALIZING; // Startzustand
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
  if (LOG_LEVEL == LogLevel::Debug){
    while(!Serial);
  } else{
    delay(2000);
  }

  Logger::setup(LOG_LEVEL); // Logger initialisieren (ohne NTP, da noch nicht verfügbar)
  Logger::log(LogLevel::Info, "Programm gestartet und Logger initialisiert.");
  Serial.println("Hallo vom Setup");

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
    // sevenSegmentTest(*sevenSegmentDisplays[i]);
  }

  // LED-Streifen Initialisieren
  myLedStrip = new LedStrip(LED_PIN, NUM_LEDS);
  // ledStripTest();
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

void updateSensorValues(unsigned long &lastApiCall, boolean forceUpdate = false){
  if (forceUpdate || millis() - lastApiCall >= SENSOR_UPDATE_CYCLE ) {
    lastApiCall = millis();

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
  }
}

void i2cBusScan(){
  Serial.println("Scanning...");

  byte error, address;
  int nDevices;

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
}

// --- Callback-Funktion für ConfigurationPortal ---
// Diese Funktion wird aufgerufen, wenn der Benutzer im Web-Portal
// neue WLAN-Daten übermittelt hat.
void onWiFiConfigSaved(const String& ssid, const String& password) {
    Logger::log(LogLevel::Info, "Hauptprogramm-Callback: WLAN-Daten empfangen.");
    savedWifiSsid = ssid;
    savedWifiPassword = password;
    // Wechsle in den Zustand, in dem wir versuchen, uns mit dem WLAN zu verbinden
    currentState = STATE_CONNECTING_WIFI;
}

// --- Funktion zum Verbinden mit WLAN ---
// Diese Funktion kapselt die WLAN-Verbindungslogik.
// Sie gibt true zurück, wenn die Verbindung erfolgreich war, sonst false.
bool connectToWiFi(const String& ssid, const String& password) {
    Logger::log(LogLevel::Info, "Versuche, mich mit WLAN zu verbinden: " + ssid);

    // Beende den SoftAP, falls er noch läuft und wir im STA-Modus sein wollen
    // Dies ist wichtig, wenn wir vom AP_MODE hierher wechseln
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        WiFi.softAPdisconnect(true);
        Logger::log(LogLevel::Info, "SoftAP wurde deaktiviert.");
    }

    // Setze den Wi-Fi-Modus auf Station (Client)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    const int MAX_WIFI_ATTEMPTS = 40; // 20 Sekunden Timeout (40 * 500ms)
    while (WiFi.status() != WL_CONNECTED && attempts < MAX_WIFI_ATTEMPTS) {
        delay(500);
        Serial.print("."); // Visuelle Rückmeldung im Serial Monitor
        attempts++;
    }
    Serial.println(); // Neue Zeile nach den Punkten

    if (WiFi.status() == WL_CONNECTED) {
        Logger::log(LogLevel::Info, "WLAN erfolgreich verbunden!");
        Logger::log(LogLevel::Info, "IP-Adresse: " + WiFi.localIP().toString());
        return true;
    } else {
        Logger::log(LogLevel::Error, "WLAN-Verbindung fehlgeschlagen!");
        return false;
    }
}

// --- Initialisierung der Netzwerkdienste (NTP, APIs) ---
// Diese Funktion wird aufgerufen, sobald eine stabile WLAN-Verbindung besteht.
void initializeNetworkServices() {
    Logger::log(LogLevel::Info, "Initialisiere Netzwerkdienste...");

    // NTPTimeSync initialisieren (nutzt die bereits aktive WLAN-Verbindung)
    // Die getInstance wird hier nur mit den NTP-spezifischen Parametern aufgerufen.
    NTPTimeSync::getInstance(NTP_SERVER, TIME_OFFSET, UPDATE_INTERVALL);
    if (NTPTimeSync::getInstance().begin()) {
        // Zeit an Logger weitergeben, falls Logger eine NTP-Instanz zur Zeitstempelung benötigt
        Logger::setup(LOG_LEVEL, &NTPTimeSync::getInstance());
        Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");
    } else {
        Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
        // Was tun, wenn NTP fehlschlägt? Das Gerät könnte trotzdem funktionieren,
        // aber die Zeitstempel sind nicht korrekt. Für dieses Beispiel machen wir weiter.
    }

    // Weather API initialisieren
    WeatherClient::getInstance(WEATHER_API_SERVER, GOOGLE_ACCESS_TOKEN);
    // Pollen API initialisieren
    PollenClient::getInstance(POLLEN_API_SERVER, GOOGLE_ACCESS_TOKEN);

    Logger::log(LogLevel::Info, "Netzwerkdienste initialisiert.");
}

void updateWeatherApi(unsigned long &lastApiCall, boolean forceUpdate = false){
    if (forceUpdate || millis() - lastApiCall >= WEATHER_API_UPDATE_CYCLE ) {
        lastApiCall = millis();
        Logger::log(LogLevel::Info, "Abfrage von Wetterdaten...");

        // Wetterdaten abrufen
        if (WeatherClient::getInstance().getCurrentConditions(LATITUDE, LONGITUDE, currentWeatherData)) {
            Logger::log(LogLevel::Info, "Wetterdaten erfolgreich abgerufen.");
            
            // Anzeigen
            myLedStrip->clearGroupLEDs(2, 6);
            switch(currentWeatherData.weatherType){
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
              case WeatherConditionType::UNKNOWN: break; // Fallback -> Nichts anzeigen break;
            }

        } else {
            Logger::log(LogLevel::Error, "Fehler beim Abrufen der Wetterdaten.");
        }
    }
}

void updatePollenApi(unsigned long &lastApiCall , boolean forceUpdate = false){
    if (forceUpdate || millis() - lastApiCall >= POLLEN_API_UPDATE_CYCLE) {
        lastApiCall = millis();
        Logger::log(LogLevel::Info, "Abfrage von Pollendaten...");

        // Pollen Daten abfragen
        if (PollenClient::getInstance().getCurrentPollen(LATITUDE, LONGITUDE, currentPollenData)) {
            Logger::log(LogLevel::Info, "Pollendaten erfolgreich abgerufen.");
            
            // Anzeigen
            int r, g, b; 
            int maxPollenLevel = max(currentPollenData.grassPollenLevel, max(currentPollenData.treePollenLevel, currentPollenData.weedPollenLevel));
            r = map(maxPollenLevel, 0, 5, 0, 255); // Rotanteil steigt von 0 (Grün) auf 255 (Rot)
            g = map(maxPollenLevel, 0, 5, 255, 0); // Grünanteil sinkt von 255 (Grün) auf 0 (Rot)
            b = 0;                                 // Blau ist immer 0
            myLedStrip->setGroupLEDs(9, 3, r, g, b);

        } else {
            Logger::log(LogLevel::Error, "Fehler beim Abrufen der Pollendaten.");
        }
    }
}

void loop() {
    // Statische Variable, um sicherzustellen, dass initializeNetworkServices()
  // nur einmal aufgerufen wird, wenn currentState auf STATE_NORMAL_OPERATION wechselt.
  static bool servicesInitializedInNormalOp = false;
  
  // Letzter API Aufruf
  static unsigned long lastApiCallWeather = 0;
  static unsigned long lastApiCallPollen = 0;
  // Sensoren
  static unsigned long lastSensorCall = 0;
  
  updateSensorValues(lastSensorCall);

  switch (currentState) {
    case STATE_INITIALIZING:
      Logger::log(LogLevel::Info, "STATE_INITIALIZING: Versuche, WLAN-Konfiguration zu laden.");
      if (ConfigurationPortal::getInstance().loadWiFiConfig(savedWifiSsid, savedWifiPassword)) {
          Logger::log(LogLevel::Info, "Gespeicherte WLAN-Konfiguration gefunden.");
          currentState = STATE_CONNECTING_WIFI;
      } else {
          Logger::log(LogLevel::Info, "Keine gespeicherte WLAN-Konfiguration gefunden. Wechsel zu STATE_AP_MODE.");
          currentState = STATE_AP_MODE;
          // AP-Modus starten, da keine gespeicherten Daten vorhanden sind
          ConfigurationPortal::getInstance().onConfigSaved(onWiFiConfigSaved); // Callback registrieren
          if (!ConfigurationPortal::getInstance().begin(AP_SSID, AP_PASSWORD)) {
              Logger::log(LogLevel::Error, "Konfigurations-AP konnte nicht gestartet werden. Systemfehler.");
              while (true) { delay(100); } // Kritischer Fehler, Gerät blockiert
          }
      }
      break;

    case STATE_AP_MODE:
      ConfigurationPortal::getInstance().handleClient(); // Webserver-Anfragen verarbeiten
      // Wenn der Callback aufgerufen wird (neue Daten empfangen), wechselt currentState zu STATE_CONNECTING_WIFI.
      break;

    case STATE_CONNECTING_WIFI:
      Logger::log(LogLevel::Info, "STATE_CONNECTING_WIFI: Versuche zu verbinden.");
      if (connectToWiFi(savedWifiSsid, savedWifiPassword)) {
          currentState = STATE_NORMAL_OPERATION;
          // Da wir jetzt eine stabile WLAN-Verbindung haben, initialisiere die Dienste
          initializeNetworkServices();

          // API das erste mal aufrufen um die aktuellen Daten zu erhalten.
          updateWeatherApi(lastApiCallWeather, true);
          updatePollenApi(lastApiCallPollen, true);
          servicesInitializedInNormalOp = true; // Markiere, dass Dienste initialisiert wurden
      } else {
          Logger::log(LogLevel::Error, "Verbindung fehlgeschlagen. Zurück zum AP-Modus.");
          currentState = STATE_AP_MODE;
          // AP neu starten, wenn Verbindung fehlschlägt
          ConfigurationPortal::getInstance().onConfigSaved(onWiFiConfigSaved); // Callback erneut registrieren
          if (!ConfigurationPortal::getInstance().begin(AP_SSID, AP_PASSWORD)) {
              Logger::log(LogLevel::Error, "Konfigurations-AP konnte nicht neu gestartet werden. Systemfehler.");
              while (true) { delay(100); } // Kritischer Fehler
          }
      }
      break;

    case STATE_NORMAL_OPERATION:
      // Überprüfe kontinuierlich, ob die WLAN-Verbindung noch besteht
      if (WiFi.status() != WL_CONNECTED) {
          Logger::log(LogLevel::Error, "WLAN-Verbindung im Normalbetrieb verloren. Wechsel zu STATE_WIFI_CONNECTION_LOST.");
          currentState = STATE_WIFI_CONNECTION_LOST;
          servicesInitializedInNormalOp = false; // Dienste müssen eventuell neu initialisiert werden
          break; // Sofortiger Wechsel des Zustands
      }

      // --- Dein normaler Betriebs-Code, der nur bei bestehender WLAN-Verbindung läuft ---
      NTPTimeSync::getInstance().update(); // Zeit aktualisieren
      
      updateWeatherApi(lastApiCallWeather);
      updatePollenApi(lastApiCallPollen);

      // Optional: Zeit alle Sekunde ausgeben
      // static unsigned long lastTimePrint = 0;
      // if (millis() - lastTimePrint >= 1000) {
      //     lastTimePrint = millis();
      //     Serial.println("Mills: " + String(lastTimePrint));
      //     Serial.println("Aktuelle Zeit: " + NTPTimeSync::getInstance().getFormattedTime());
      //     delay(5000);
      // }
      break;

    case STATE_WIFI_CONNECTION_LOST:
      Logger::log(LogLevel::Info, "STATE_WIFI_CONNECTION_LOST: Versuche erneut zu verbinden...");
      if (connectToWiFi(savedWifiSsid, savedWifiPassword)) {
          Logger::log(LogLevel::Info, "Erneute WLAN-Verbindung erfolgreich hergestellt.");
          currentState = STATE_NORMAL_OPERATION;
          // Dienste müssen hier nicht neu initialisiert werden, da die Instanzen erhalten bleiben.
          // NTPClient.begin() wird bei jedem NTPTimeSync.begin() aufgerufen.
          // APIs sind bereits initialisiert.
      } else {
          Logger::log(LogLevel::Error, "Erneute WLAN-Verbindung fehlgeschlagen. Starte Konfigurations-AP.");
          currentState = STATE_AP_MODE;
          // AP neu starten
          ConfigurationPortal::getInstance().onConfigSaved(onWiFiConfigSaved); // Callback erneut registrieren
          if (!ConfigurationPortal::getInstance().begin(AP_SSID, AP_PASSWORD)) {
              Logger::log(LogLevel::Error, "Konfigurations-AP konnte nicht neu gestartet werden. Systemfehler.");
              while (true) { delay(100); } // Kritischer Fehler
          }
      }
      break;
  }

  delay(10); // Kurze Pause, um den Watchdog nicht auszulösen und andere Tasks zuzulassen
}
