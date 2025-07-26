#include <Arduino.h>
#include <WiFi.h> // Wichtig: Für WiFi.status() etc.
#include <Preferences.h> // Für NVS-Zugriff, wird von ConfigurationPortal verwendet

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

#include "Settings.h" // Enthält AP_SSID, AP_PASSWORD, BUTTON_A/B/C, PCF_ADDRESSES etc.

// Lokale Speicher für API-Daten
WeatherData currentWeatherData;
PollenData currentPollenData;

// Feuchtigkeitssensor
TempHumi* tempHumi;
AirQuality* airQuality;

// Zustände des Geräts
enum DeviceState {
    STATE_INITIALIZING,             // Beim Start: Lade Konfiguration und versuche WLAN-Verbindung
    STATE_AP_MODE,                  // Gerät ist im Konfigurations-AP-Modus
    STATE_CONNECTING_WIFI,          // Gerät versucht, sich mit dem konfigurierten WLAN zu verbinden
    STATE_NORMAL_OPERATION,         // Gerät ist mit WLAN verbunden und führt normale Aufgaben aus
    STATE_WIFI_CONNECTION_LOST      // WLAN-Verbindung wurde unterbrochen, versuche Reconnect
};

DeviceState currentState = STATE_INITIALIZING; // Startzustand

// Globale Konfigurationsinstanz, die alle Einstellungen enthält
AppConfig currentDeviceConfig;

// Anzeige auf dem Display
UpdateDisplay* updateDisplay;

// Forward Declarations für Methoden.
bool connectToWiFi(); // Keine Parameter mehr, nutzt currentDeviceConfig
void onConfigSavedCallback(const AppConfig& config); // Callback für Portal
void applyDeviceSettings(); // Funktion zum Anwenden der Einstellungen
void updateSensorValues(unsigned long &lastApiCall, boolean forceUpdate = false); // Beibehalten
void i2cBusScan(); // Beibehalten
void initializeNetworkServices(); // Beibehalten
void updateWeatherApi(unsigned long &lastApiCall, boolean forceUpdate = false); // Beibehalten
void updatePollenApi(unsigned long &lastApiCall , boolean forceUpdate = false); // Beibehalten


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

  // Mp3Player initialisieren
  Mp3Player::getInstance().begin(Serial1);

  // Display Initialisieren
  updateDisplay = new UpdateDisplay();

  // Initialisiere ConfigurationPortal Singleton, damit es Preferences öffnen kann
  ConfigurationPortal& portal = ConfigurationPortal::getInstance();
  // Setze den Callback, der aufgerufen wird, wenn die Konfiguration über das Webportal gespeichert wird.
  portal.onConfigSaved(onConfigSavedCallback);

  // 1. Versuche, die gespeicherte Konfiguration aus NVS zu laden.
  if (portal.loadConfig(currentDeviceConfig)) {
      Logger::log(LogLevel::Info, "Gespeicherte Konfiguration aus NVS geladen.");
      // Wenn eine Konfiguration geladen wurde, versuche, eine WLAN-Verbindung herzustellen.
      currentState = STATE_CONNECTING_WIFI;
  } else {
      Logger::log(LogLevel::Info, "Keine gespeicherte Konfiguration gefunden. Starte Konfigurations-AP.");
      // Wenn keine Konfiguration gefunden wurde (z.B. erster Start),
      // starte den Access Point für die Erstkonfiguration.
      currentState = STATE_AP_MODE;
      portal.startAPAndWebServer(AP_SSID, AP_PASSWORD); // Verwendet AP_SSID/AP_PASSWORD aus Settings.h
  }

  // i2c Bus initialisieren
  Logger::log(LogLevel::Info, "i2c Bus starten...");
  Wire.begin(21, 22);
  Wire.setClock(100000L); // "100000L" (100 kHz)
  Logger::log(LogLevel::Info, "i2c Bus gestartet!");

  // 7-Segment Anzeige Initialisieren
  for(int i = 0; i < 5; i++){
    // Sieben Segment Anzeigen erstellen
    sevenSegmentDisplays[i] = new SevenSegmentDisplay(PCF_ADDRESSES[i]);

    // Test
    // updateDisplay->sevenSegmentTest(*sevenSegmentDisplays[i]);
  }

  // LED-Streifen Initialisieren
  myLedStrip = new LedStrip(LED_PIN, NUM_LEDS);
  // updateDisplay->ledStripTest();
  myLedStrip->setSingleLED(1, 255, 0, 0); // °C
  myLedStrip->setSingleLED(8, 0, 0, 255); // %

  // Sensoren
  tempHumi = new TempHumi();
  airQuality = new AirQuality(&Wire, 0x76);
  // Versuche, den Sensor zu initialisieren
  if (!airQuality->begin()) {
    Logger::log(LogLevel::Error, "Air Qualitäts Sensor konnte nicht gestartet werden!");
  }

  // Wende die geladenen (oder Standard-)Einstellungen auf das Gerät an.
  // Dies geschieht immer nach dem Laden der Konfiguration, unabhängig davon,
  // ob es eine gespeicherte oder die Standardkonfiguration war.
  // Dies ist der erste Punkt, an dem die Einstellungen angewendet werden.
  applyDeviceSettings();

  Logger::log(LogLevel::Info, "Ende vom Setup!");
}

void updateSensorValues(unsigned long &lastApiCall, boolean forceUpdate){
  if (forceUpdate || millis() - lastApiCall >= SENSOR_UPDATE_CYCLE ) {
    lastApiCall = millis();

    // Temperatur und Luftfeuchtigkeit
    float actTemperature;
    float actHumidity;
    if (tempHumi->readData(actTemperature, actHumidity)) {
      // Temperatur Anzeigen auf 7 Segment Anzeige
      updateDisplay->updateTemperature(actTemperature);
      updateDisplay->updateTempLED(true);

      // Luftfeuchtigkeit Anzeigen auf 7 Segment Anzeige
      updateDisplay->updateHumidity(actHumidity);
      updateDisplay->updateHumiLED(true);

    } else {
      Logger::log(LogLevel::Error, "Fehler beim Lesen der SHT30(TempHumi) Daten.");
    }

    //Luftqualität
    if (airQuality->readSensorData()) {
      // Anzeigen der Luftqualität
      float iaqValue = airQuality->getIAQ();
      // Sicherstellen, dass der IAQ-Wert im gültigen Bereich liegt
      if (iaqValue < 0.0) iaqValue = 0.0;
      if (iaqValue > 100.0) iaqValue = 100.0;

      // Farben definieren
      updateDisplay->updateAirQuality(iaqValue);

    }
    else {
      Logger::log(LogLevel::Error, "Fehler beim Lesen der Luftqualität Daten.");
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
// neue Konfigurationsdaten übermittelt hat.
void onConfigSavedCallback(const AppConfig& config) {
  Logger::log(LogLevel::Info, "Hauptprogramm-Callback: Neue Konfiguration empfangen und gespeichert.");
  currentDeviceConfig = config; // Aktualisiere die globale Konfiguration

  // Wende die neuen Einstellungen auf das Gerät an.
  applyDeviceSettings();

  // WLAN neu verbinden, falls sich SSID/Passwort geändert haben oder um die Verbindung zu aktualisieren.
  Logger::log(LogLevel::Info, "Versuche, WLAN neu zu verbinden mit neuer Konfiguration...");
  // Trenne die bestehende Verbindung, falls vorhanden
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
    delay(100);
  }
  // Wechsle in den Zustand, in dem wir versuchen, uns mit dem WLAN zu verbinden
  currentState = STATE_CONNECTING_WIFI;
}

// --- Funktion zum Verbinden mit WLAN ---
// Diese Funktion kapselt die WLAN-Verbindungslogik.
// Sie verwendet die global gespeicherte currentDeviceConfig.
// Sie gibt true zurück, wenn die Verbindung erfolgreich war, sonst false.
bool connectToWiFi() {
    Logger::log(LogLevel::Info, "Versuche, mich mit WLAN zu verbinden: " + currentDeviceConfig.wifiSsid);

    // Beende den SoftAP, falls er noch läuft und wir im STA-Modus sein wollen
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        WiFi.softAPdisconnect(true);
        Logger::log(LogLevel::Info, "SoftAP wurde deaktiviert.");
    }

    // Setze den Wi-Fi-Modus auf Station (Client)
    WiFi.mode(WIFI_STA);
    WiFi.begin(currentDeviceConfig.wifiSsid.c_str(), currentDeviceConfig.wifiPassword.c_str());

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
    // Verwendet die Werte aus currentDeviceConfig
    // Annahme: NTPTimeSync::getInstance() kann mit neuen Parametern re-initialisiert werden
    // oder die Parameter werden intern aktualisiert.
    // timeOffsetHours ist in h -> Die Methode benötigt aber Sekunden.
    NTPTimeSync::getInstance(currentDeviceConfig.ntpServer.c_str(), (currentDeviceConfig.timeOffsetHours * 60 * 60), UPDATE_INTERVALL);
    if (NTPTimeSync::getInstance().begin()) {
        // Zeit an Logger weitergeben, falls Logger eine NTP-Instanz zur Zeitstempelung benötigt
        Logger::setup(LOG_LEVEL, &NTPTimeSync::getInstance());
        Logger::log(LogLevel::Info, "NTP-Synchronisation erfolgreich abgeschlossen.");
    } else {
        Logger::log(LogLevel::Error, "NTP-Synchronisation fehlgeschlagen!");
    }

    // Weather API initialisieren
    // Die Koordinaten werden bei jedem API-Aufruf übergeben, daher ist hier keine Re-Initialisierung des Clients nötig.
    WeatherClient::getInstance(WEATHER_API_SERVER, currentDeviceConfig.googleAccessToken.c_str());
    // Pollen API initialisieren
    PollenClient::getInstance(POLLEN_API_SERVER, currentDeviceConfig.googleAccessToken.c_str());

    Logger::log(LogLevel::Info, "Netzwerkdienste initialisiert.");
}

// Funktion zum Anwenden der geladenen/gespeicherten Einstellungen auf die Hardware/Dienste.
void applyDeviceSettings() {
    Logger::log(LogLevel::Info, "Wende Geräteeinstellungen an...");

    // Beispiel: NTP-Client initialisieren (nur wenn NTPTimeSync noch nicht initialisiert wurde oder neu initialisiert werden muss)
    // Wenn NTPTimeSync.begin() im initializeNetworkServices() aufgerufen wird,
    // ist dieser Aufruf hier redundant, es sei denn, die Einstellungen haben sich geändert.
    // configTime(currentDeviceConfig.timeOffsetHours * 3600, 0, currentDeviceConfig.ntpServer.c_str());
    Logger::log(LogLevel::Info, "NTP-Server gesetzt auf: " + currentDeviceConfig.ntpServer + " mit Zeitverschiebung: " + String(currentDeviceConfig.timeOffsetHours) + "h");

    // FastLED Textfarbe setzen
    CRGB displayColor = CRGB(currentDeviceConfig.textColorCRGB);
    updateDisplay->setColorTime(displayColor.r, displayColor.g, displayColor.b);
    Logger::log(LogLevel::Info, "LED-Farbe auf 0x" + String(currentDeviceConfig.textColorCRGB, HEX) + " gesetzt.");

    updateDisplay->setBrightness(currentDeviceConfig.ledBrightness);
    Logger::log(LogLevel::Info, "LED-Helligkeit auf: " + String(currentDeviceConfig.ledBrightness) + " gesetzt.");

    // Lautstärke einstellen (wenn du einen DAC/Verstärker hast)
    // int mappedVolume = map(currentDeviceConfig.volume, 0, 30, 0, 255); // Beispiel-Mapping für 0-255 Bereich
    // analogWrite(DAC_PIN, mappedVolume); // Annahme: DAC_PIN ist definiert und für Lautstärke zuständig
    updateDisplay->updateVolume(currentDeviceConfig.volume);
    Logger::log(LogLevel::Info, "Lautstärke auf " + String(currentDeviceConfig.volume) + " gesetzt.");

}

void updateWeatherApi(unsigned long &lastApiCall, boolean forceUpdate){
    // Nutze das konfigurierte Update-Intervall in Minuten, um Millisekunden zu berechnen
    unsigned long updateIntervalMs = (unsigned long)currentDeviceConfig.weatherUpdateIntervalMin * 60 * 1000UL;
    if (forceUpdate || millis() - lastApiCall >= updateIntervalMs ) {
        lastApiCall = millis();
        Logger::log(LogLevel::Info, "Abfrage von Wetterdaten...");

        // Wetterdaten abrufen, nutze die konfigurierten Koordinaten
        if (WeatherClient::getInstance().getCurrentConditions(currentDeviceConfig.latitude, currentDeviceConfig.longitude, currentWeatherData)) {
            Logger::log(LogLevel::Info, "Wetterdaten erfolgreich abgerufen.");
        } else {
            Logger::log(LogLevel::Error, "Fehler beim Abrufen der Wetterdaten.");
        }
    }
}

void updatePollenApi(unsigned long &lastApiCall , boolean forceUpdate){
    // Nutze das konfigurierte Update-Intervall in Minuten, um Millisekunden zu berechnen
    unsigned long updateIntervalMs = (unsigned long)currentDeviceConfig.pollenUpdateIntervalMin * 60 * 1000UL;
    if (forceUpdate || millis() - lastApiCall >= updateIntervalMs) {
        lastApiCall = millis();
        Logger::log(LogLevel::Info, "Abfrage von Pollendaten...");

        // Pollen Daten abfragen, nutze die konfigurierten Koordinaten
        if (PollenClient::getInstance().getCurrentPollen(currentDeviceConfig.latitude, currentDeviceConfig.longitude, currentPollenData)) {
            Logger::log(LogLevel::Info, "Pollendaten erfolgreich abgerufen.");

            // Anzeigen
            int maxPollenLevel = max(currentPollenData.grassPollenLevel, max(currentPollenData.treePollenLevel, currentPollenData.weedPollenLevel));
            updateDisplay->updatePollen(maxPollenLevel);

        } else {
            Logger::log(LogLevel::Error, "Fehler beim Abrufen der Pollendaten.");
        }
    }
}


void loop() {
  // Muss immer in der loop() aufgerufen werden, damit der Webserver Anfragen verarbeiten kann.
  ConfigurationPortal::getInstance().handleClient();

  // Statische Variable, um sicherzustellen, dass initializeNetworkServices()
  // nur einmal aufgerufen wird, wenn currentState auf STATE_NORMAL_OPERATION wechselt.
  static bool servicesInitializedInNormalOp = false;

  // Letzter API Aufruf
  static unsigned long lastApiCallWeather = 0;
  static unsigned long lastApiCallPollen = 0;
  // Sensoren
  static unsigned long lastSensorCall = 0;
  static unsigned long showDisplayValuesTimer = 0; // Umbenannt von showIndoorValues für Klarheit
  static boolean showingIndoor = true; // Umbenannt von showIndoor

  // Logik für die Anzeige der Innen-/Aussentemperatur
  // Nutzt die konfigurierten Anzeigedauern
  unsigned long indoorDisplayTimeMs = (unsigned long)currentDeviceConfig.indoorTempDisplayTimeSec * 1000UL;
  unsigned long outdoorDisplayTimeMs = (unsigned long)currentDeviceConfig.outdoorTempDisplayTimeSec * 1000UL;

  if (currentState == STATE_NORMAL_OPERATION) {
    if (showingIndoor) {
      updateSensorValues(lastSensorCall); // Zeigt Innentemperatur/Feuchtigkeit
      if (millis() - showDisplayValuesTimer >= indoorDisplayTimeMs) {
        showingIndoor = false;
        showDisplayValuesTimer = millis(); // Timer für Aussentemperatur starten
      }
    } else {
      // Zeigt Aussentemperatur/Wetterdaten
      updateDisplay->updateWeather(currentWeatherData.weatherType);
      updateDisplay->updateTemperature(currentWeatherData.temperature.degrees);
      updateDisplay->updateTempLED(false); // Annahme: false bedeutet Aussentemp-LED
      updateDisplay->updateHumidity(currentWeatherData.relativeHumidity);
      updateDisplay->updateHumiLED(false); // Annahme: false bedeutet Aussentemp-LED

      if (millis() - showDisplayValuesTimer >= outdoorDisplayTimeMs) {
        showingIndoor = true;
        showDisplayValuesTimer = millis(); // Timer für Innentemperatur starten
      }
    }
  } else {
    // Wenn nicht im Normalbetrieb, immer Innensensorwerte anzeigen oder nichts
    updateSensorValues(lastSensorCall);
    // Setze den Timer zurück, damit er beim Wechsel in NORMAL_OPERATION neu startet
    showDisplayValuesTimer = millis();
    showingIndoor = true;
  }


  switch (currentState) {
    case STATE_INITIALIZING:
      Logger::log(LogLevel::Info, "STATE_INITIALIZING: Versuche, WLAN-Konfiguration zu laden.");
      // Die Logik zum Laden der Konfiguration ist bereits im setup()
      // und hat den currentState gesetzt. Hier passiert nichts, ausser warten auf den nächsten Loop-Durchlauf.
      // Der Übergang zu STATE_CONNECTING_WIFI oder STATE_AP_MODE wurde bereits in setup() gehandhabt.
      break;

    case STATE_AP_MODE:
      // Der Webserver läuft und wartet auf Eingaben.
      // handleClient() wird bereits am Anfang der loop() aufgerufen.
      // Der Übergang zu STATE_CONNECTING_WIFI erfolgt über den onConfigSavedCallback.
      break;

    case STATE_CONNECTING_WIFI:
      Logger::log(LogLevel::Info, "STATE_CONNECTING_WIFI: Versuche zu verbinden.");
      if (connectToWiFi()) { // Nutzt currentDeviceConfig
          currentState = STATE_NORMAL_OPERATION;
          // Da wir jetzt eine stabile WLAN-Verbindung haben, initialisiere die Dienste
          if (!servicesInitializedInNormalOp) {
              initializeNetworkServices();
              // API das erste Mal aufrufen, um die aktuellen Daten zu erhalten.
              updateWeatherApi(lastApiCallWeather, true);
              updatePollenApi(lastApiCallPollen, true);
              servicesInitializedInNormalOp = true; // Markiere, dass Dienste initialisiert wurden
          }
          // Starte den Webserver im Station-Modus, um weitere Einstellungen zu ermöglichen.
          ConfigurationPortal::getInstance().startWebServerInStationMode();
      } else {
          Logger::log(LogLevel::Error, "Verbindung fehlgeschlagen. Zurück zum AP-Modus.");
          currentState = STATE_AP_MODE;
          // AP neu starten, wenn Verbindung fehlschlägt
          ConfigurationPortal::getInstance().startAPAndWebServer(AP_SSID, AP_PASSWORD);
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
      updateDisplay->updateTime(NTPTimeSync::getInstance().getHour(), NTPTimeSync::getInstance().getMin(), currentDeviceConfig.volume > 0); // Den Song nur Abspielen, wenn die Lautstärke > 0 ist.

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
      if (connectToWiFi()) { // Nutzt currentDeviceConfig
          Logger::log(LogLevel::Info, "Erneute WLAN-Verbindung erfolgreich hergestellt.");
          currentState = STATE_NORMAL_OPERATION;
          // Dienste müssen hier nicht neu initialisiert werden, da die Instanzen erhalten bleiben.
          // NTPClient.begin() wird bei jedem NTPTimeSync.begin() aufgerufen.
          // APIs sind bereits initialisiert.
          if (!servicesInitializedInNormalOp) { // Falls Dienste aus irgendeinem Grund nicht initialisiert wurden
                initializeNetworkServices();
                servicesInitializedInNormalOp = true;
          }
          ConfigurationPortal::getInstance().startWebServerInStationMode();
      } else {
          Logger::log(LogLevel::Error, "Erneute WLAN-Verbindung fehlgeschlagen. Starte Konfigurations-AP.");
          currentState = STATE_AP_MODE;
          // AP neu starten
          ConfigurationPortal::getInstance().startAPAndWebServer(AP_SSID, AP_PASSWORD);
      }
      break;
  }

  delay(10); // Kurze Pause, um den Watchdog nicht auszulösen und andere Tasks zuzulassen
}
