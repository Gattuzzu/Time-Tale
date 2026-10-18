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

// Menu ist aktiviert
boolean menuIsActiv = false;
int ipAddressIndex = 0;
int menuPoint = 1;
boolean buttonAisPressed = false;
boolean buttonBisPressed = false;
boolean buttonCisPressed = false;

// Globale Konfigurationsinstanz, die alle Einstellungen enthält
AppConfig currentDeviceConfig;

// Anzeige auf dem Display
UpdateDisplay* updateDisplay;

// Forward Declarations für Methoden.
bool connectToWiFi(); // Keine Parameter mehr, nutzt currentDeviceConfig
void onConfigSavedCallback(const AppConfig& config); // Callback für Portal
void applyDeviceSettings(); // Funktion zum Anwenden der Einstellungen
void updateSensorValues(unsigned long &lastApiCall, boolean forceUpdate = false);
void i2cBusScan(); // Beibehalten
void initializeNetworkServices(); // Beibehalten
void updateWeatherApi(unsigned long &lastApiCall, boolean forceUpdate = false); // Beibehalten
void updatePollenApi(unsigned long &lastApiCall , boolean forceUpdate = false); // Beibehalten
bool parseIpString(const String& ipString, std::vector<int>& octets);
void showMenu(unsigned long &lastIPaddressChange, unsigned long &lastChangeFromUser);
boolean isButtonPressed(int button);
boolean buttonPressToImpuls(int button, boolean &buttonIsPressed);


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
  Logger::log(LogLevel::Info, "Programm gestartet (Version " + String(FIRMWARE_VERSION) + ") und Logger initialisiert.");

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

boolean isButtonPressed(int button){
  return !digitalRead(button);
}

// Wandelt den Button Druck in einen Impuls um
boolean buttonPressToImpuls(int button, boolean &buttonIsPressed){
  boolean isPressed = isButtonPressed(button);
  if (isPressed && !buttonIsPressed){
    buttonIsPressed = true;
    return true;
  } else if (!isPressed){
    buttonIsPressed = false;
  }
  return false;
}

void updateSensorValues(unsigned long &lastApiCall, boolean forceUpdate){
  if (!menuIsActiv){

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
        if (iaqValue < 0.0) { iaqValue = 0.0; }
        if (iaqValue > 100.0) { iaqValue = 100.0; }
        
        // Farben definieren
        updateDisplay->updateAirQuality(iaqValue);
        
      }
      else {
        Logger::log(LogLevel::Error, "Fehler beim Lesen der Luftqualität Daten.");
      }
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

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.print(nDevices);
  }
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

    // NTPTimeSync synchronisieren (nutzt die bereits aktive WLAN-Verbindung)
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

    Logger::log(LogLevel::Info, "NTP-Server gesetzt auf: " + currentDeviceConfig.ntpServer + " mit Basis-Zeitverschiebung: " + String(currentDeviceConfig.timeOffsetHours) + "h (Auto-Sommerzeit: " + String(currentDeviceConfig.daylightSavingAutomatic ? "aktiv" : "inaktiv") + ")");
    NTPTimeSync::getInstance(currentDeviceConfig.ntpServer.c_str(), (currentDeviceConfig.timeOffsetHours * 60 * 60), UPDATE_INTERVALL, currentDeviceConfig.daylightSavingAutomatic);

    WeatherClient::getInstance(WEATHER_API_SERVER, currentDeviceConfig.googleAccessToken.c_str());
    PollenClient::getInstance(POLLEN_API_SERVER, currentDeviceConfig.googleAccessToken.c_str());

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

void showMenu(unsigned long &lastIPaddressChange, unsigned long &lastChangeFromUser){
  // Button Zustänge auslesen
  boolean buttonAimpuls = buttonPressToImpuls(BUTTON_A, buttonAisPressed);
  boolean buttonBimpuls = buttonPressToImpuls(BUTTON_B, buttonBisPressed);
  boolean buttonCimpuls = buttonPressToImpuls(BUTTON_C, buttonCisPressed);
  boolean firstTimeOpenMenuPoint = false;

  // Wurde irgen einer der Buttons betätigt soll die Zeit bis das Menu verlassen wird von vorne beginnen
  if(buttonAimpuls || buttonBimpuls || buttonCimpuls){
    lastChangeFromUser = millis();
  }

  if (!menuIsActiv && buttonBimpuls){
    menuIsActiv = true;
    firstTimeOpenMenuPoint = true;
    buttonBimpuls = false; // Nach einmaliger Verwendung zurücksetzen
  }

  if (menuIsActiv){

    // Menu Schriftpunkt anzeigen
    // Aber nur beim ersten Mal um angezeigte Einstellungen nicht zu überschreiben
    if (firstTimeOpenMenuPoint){
      updateDisplay->showMenu();
    }

    // IP Adresse anzeigen
    if (millis() - lastIPaddressChange >= IP_ADDRESS_SHOW_CYCLE) {
      lastIPaddressChange = millis();
      if (ipAddressIndex == 4){
        updateDisplay->clearTemperature();
        ipAddressIndex = 0;
      } else{

        std::vector<int> octets;
        String ip;
        if (currentState == STATE_AP_MODE){
          ip = WiFi.softAPIP().toString();
          
        } else if (currentState == STATE_NORMAL_OPERATION) {
          ip = WiFi.localIP().toString();
        }
        if (ip != nullptr && parseIpString(ip, octets)){
          updateDisplay->showIPAddress(octets[ipAddressIndex], ipAddressIndex != 3);
        }
        ipAddressIndex++;
      }
    }

    if (buttonBimpuls){
      menuPoint++; // Wechseln zum nächsten Menu Punkt
      if(menuPoint > 4){
        menuPoint = 1;
      }
      firstTimeOpenMenuPoint = true;

      buttonBimpuls = false; // Nach einmaliger Verwendung zurücksetzen
    }

    updateDisplay->showActMenuPoint(menuPoint);
  
    // Es darf nur ein Button gleichzeitig betätigt werden.
    if(firstTimeOpenMenuPoint || ((buttonAimpuls || buttonCimpuls) && !(buttonAimpuls && buttonCimpuls))){
      switch (menuPoint)
      {
        case 1:{ // Textfarbe der Uhr
          const String colorAsHex[] = {"FF0000", "00FF00", "0000FF", "FFFFFF", "F94D00", "7F00FF", "00FFFF", "FFFF00", "FF007F"};
          const int numColors = sizeof(colorAsHex) / sizeof(colorAsHex[0]); // Anzahl der Elemente im Array
          String actColorAsString = String(currentDeviceConfig.textColorCRGB, HEX);
          
          while (actColorAsString.length() < 6){
            actColorAsString = "0" + actColorAsString; // Mit Führenden 0 auffüllen.
          }
          
          actColorAsString.toUpperCase();
          int index = -1; // Standardmässig -1, falls nicht gefunden
          for (int i = 0; i < numColors; i++) {
            if (colorAsHex[i] == actColorAsString) {
              index = i;
              break;
            }
          }
          
          
          if (buttonAimpuls){ index++; }
          if (buttonCimpuls){ index--; }
          
          // Index korrigieren
          if (index < 0)                    { index = numColors - 1; }
          if (index >= numColors){ index = 0; }
          
          Logger::log(LogLevel::Info, "Color: " + String(currentDeviceConfig.textColorCRGB));
          Logger::log(LogLevel::Info, "Index: " + String(index));

          currentDeviceConfig.textColorCRGB = strtoul(colorAsHex[index].c_str(), NULL, 16);
          ConfigurationPortal::getInstance().saveConfig(currentDeviceConfig); // Die Änderung speichern
          
          CRGB displayColor = CRGB(currentDeviceConfig.textColorCRGB);
          updateDisplay->setColorTime(displayColor.r, displayColor.g, displayColor.b);
          updateDisplay->showMenu(); // Um die Farbe zu updaten muss nochmals showMenu() aufgerufen werden.
          break;
        } 
        
        case 2:{ // Helligkeit der Led's
          int brightness = currentDeviceConfig.ledBrightness;
          
          if (buttonAimpuls){ brightness += 5; }
          if (buttonCimpuls){ brightness -= 5; }
          
          // Index korrigieren
          boolean maxReached = false;
          if (brightness <   0){ brightness =   0; maxReached = true; }
          if (brightness > 100){ brightness = 100; maxReached = true; }
          
          Logger::log(LogLevel::Info, "Helligkeit: " + String(brightness));

          currentDeviceConfig.ledBrightness = brightness;
          ConfigurationPortal::getInstance().saveConfig(currentDeviceConfig); // Die Änderung speichern
          
          updateDisplay->setBrightness(currentDeviceConfig.ledBrightness);
          updateDisplay->showMenu(maxReached); // Um die Helligkeit zu updaten muss nochmals showMenu() aufgerufen werden.
          break;
        }
        
        case 3:{ // Lautstärke der Uhr
          int volume = currentDeviceConfig.volume;
          
          if (buttonAimpuls){ volume += 2; }
          if (buttonCimpuls){ volume -= 2; }
          
          // Index korrigieren
          boolean maxReached = false;
          if (volume <  0){ volume =  0; maxReached = true; }
          if (volume > 30){ volume = 30; maxReached = true; }
          
          Logger::log(LogLevel::Info, "Volume: " + String(volume));

          currentDeviceConfig.volume = volume;
          ConfigurationPortal::getInstance().saveConfig(currentDeviceConfig); // Die Änderung speichern
          
          updateDisplay->updateVolume(currentDeviceConfig.volume);

          int playHour = NTPTimeSync::getInstance().getHour() % 12; // 24h auf 12h umwandeln
          playHour = playHour == 0 ? 12 : playHour; // Wenn 0 Uhr -> 12 Ausgeben.
          updateDisplay->playSound(playHour); // Test Sound (Aktuelle Stunde) abspielen

          updateDisplay->showMenu(maxReached); // showMenu() aufrufen, um das erreichen vom Max anzuzeigen
          break;
        }

        case 4:{ // Zeitverschiebung
          int timeOffsetHours = currentDeviceConfig.timeOffsetHours;
          Logger::log(LogLevel::Info, "Zeitverschiebung: " + String(timeOffsetHours));

          if (buttonAimpuls){ timeOffsetHours += 1; }
          if (buttonCimpuls){ timeOffsetHours -= 1; }

          // Index korrigieren
          if (timeOffsetHours < -12){ timeOffsetHours =  12; }
          if (timeOffsetHours >  12){ timeOffsetHours = -12; }
          
          currentDeviceConfig.timeOffsetHours = timeOffsetHours;
          ConfigurationPortal::getInstance().saveConfig(currentDeviceConfig); // Die Änderung speichern
          updateDisplay->updateTime(abs(timeOffsetHours), timeOffsetHours <= 0 ? 55 : 5, false, true); // Die eingestellte Zeitverschiebung anzeigen
          NTPTimeSync::getInstance().setTimeOffset(currentDeviceConfig.timeOffsetHours * 60 * 60);
          NTPTimeSync::getInstance().update();
          break;
        }
      }
    }

    // Menu wieder schliessen
    if (millis() - lastChangeFromUser >= MENU_OPEN_TIME) {
      // Variablen zurücksetzen:
      menuPoint = 1;
      ipAddressIndex = 0;
      updateDisplay->clearTime();

      menuIsActiv = false;
    }

  }
}

bool parseIpString(const String& ipString, std::vector<int>& octets) {
    // Temporärer Vektor für die parsed Oktette
    std::vector<int> tempOctets;
    tempOctets.reserve(4); // Reserviere Platz für 4 Elemente

    int dot1 = ipString.indexOf('.');
    int dot2 = ipString.indexOf('.', dot1 + 1);
    int dot3 = ipString.indexOf('.', dot2 + 1);

    if (dot1 == -1 || dot2 == -1 || dot3 == -1) {
        // Nicht genügend Punkte gefunden, ungültiges IP-Format
        Logger::log(LogLevel::Error, "Fehler: Ungültiges IP-Format (nicht genügend Punkte).");
        return false;
    }

    // Extrahieren der Substrings für jedes Oktett
    String sOctet1 = ipString.substring(0, dot1);
    String sOctet2 = ipString.substring(dot1 + 1, dot2);
    String sOctet3 = ipString.substring(dot2 + 1, dot3);
    String sOctet4 = ipString.substring(dot3 + 1);

    // Konvertieren der Substrings in Integer
    int o1 = sOctet1.toInt();
    int o2 = sOctet2.toInt();
    int o3 = sOctet3.toInt();
    int o4 = sOctet4.toInt();

    // Zusätzliche Validierung:
    // Überprüfen, ob die toInt()-Konvertierung erfolgreich war (nicht 0, wenn die Zeichen ungültig waren)
    // und ob die Werte im gültigen Bereich (0-255) liegen.
    if (sOctet1.length() == 0 || sOctet2.length() == 0 || sOctet3.length() == 0 || sOctet4.length() == 0 ||
        o1 < 0 || o1 > 255 ||
        o2 < 0 || o2 > 255 ||
        o3 < 0 || o3 > 255 ||
        o4 < 0 || o4 > 255) {
        Logger::log(LogLevel::Error, "Fehler: Ungültige Oktett-Werte oder leere Oktette.");
        return false;
    }

    // Wenn alle Prüfungen bestanden sind, füge die Oktette zum temporären Vektor hinzu
    tempOctets.push_back(o1);
    tempOctets.push_back(o2);
    tempOctets.push_back(o3);
    tempOctets.push_back(o4);

    // Weise den temporären Vektor dem Referenzvektor zu
    octets = tempOctets;

    return true;
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

  // Menu 
  static unsigned long lastIPAddressPart = 0;
  static unsigned long lastMenuChangeFromUser = 0;

  // Logik für die Anzeige der Innen-/Aussentemperatur
  // Nutzt die konfigurierten Anzeigedauern
  unsigned long indoorDisplayTimeMs = (unsigned long)currentDeviceConfig.indoorTempDisplayTimeSec * 1000UL;
  unsigned long outdoorDisplayTimeMs = (unsigned long)currentDeviceConfig.outdoorTempDisplayTimeSec * 1000UL;

  // Menu Aufruf
  showMenu(lastIPAddressPart, lastMenuChangeFromUser);

  if (!menuIsActiv) {
    // Zeit aktualisieren (falls WLAN da ist, synchronisiert NTPClient; falls nicht, läuft die interne Zeit per millis() weiter)
    NTPTimeSync::getInstance().update();

    // Sobald die Zeit mindestens einmal synchronisiert wurde, die Uhrzeit kontinuierlich anzeigen
    if (NTPTimeSync::getInstance().isTimeSet()) {
      updateDisplay->updateTime(NTPTimeSync::getInstance().getHour(), NTPTimeSync::getInstance().getMin(), currentDeviceConfig.volume > 0);
    }

    if (currentState == STATE_NORMAL_OPERATION) {
      if (showingIndoor) {
        // Helligkeit auf Tageswert zurücksetzen (Innentemperatur-Ansicht)
        updateDisplay->setBrightness(currentDeviceConfig.ledBrightness);
        updateSensorValues(lastSensorCall); // Zeigt Innentemperatur/Feuchtigkeit
        if (millis() - showDisplayValuesTimer >= indoorDisplayTimeMs) {
          showingIndoor = false;
          showDisplayValuesTimer = millis(); // Timer für Aussentemperatur starten
        }
      } else {
        // Zeigt Aussentemperatur/Wetterdaten
        updateDisplay->updateWeather(currentWeatherData.weatherType, currentWeatherData.isDaytime, currentDeviceConfig.thunderFlashesPerHour);
        updateDisplay->applyNightDim(currentWeatherData.isDaytime, currentDeviceConfig.ledBrightness, currentDeviceConfig.nightDimPercent);
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
  }


  // Zeitstempel für periodische WLAN-Wiederverbindungsversuche im AP-Modus
  static unsigned long lastWifiReconnectAttempt = 0;

  switch (currentState) {
    case STATE_INITIALIZING:
      Logger::log(LogLevel::Info, "STATE_INITIALIZING: Versuche, WLAN-Konfiguration zu laden.");
      // Die Logik zum Laden der Konfiguration ist bereits im setup()
      // und hat den currentState gesetzt. Hier passiert nichts, ausser warten auf den nächsten Loop-Durchlauf.
      // Der Übergang zu STATE_CONNECTING_WIFI oder STATE_AP_MODE wurde bereits in setup() gehandhabt.
      break;

    case STATE_AP_MODE: {
      // Wenn gespeicherte Zugangsdaten vorhanden sind und niemand mit dem SoftAP verbunden ist,
      // alle 5 Minuten (WIFI_RECONNECT_INTERVAL) einen erneuten Verbindungsversuch unternehmen.
      if (currentDeviceConfig.wifiSsid.length() > 0 && WiFi.softAPgetStationNum() == 0) {
        if (lastWifiReconnectAttempt == 0) {
          lastWifiReconnectAttempt = millis();
        } else if (millis() - lastWifiReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
          lastWifiReconnectAttempt = millis();
          Logger::log(LogLevel::Info, "Kein Client am AP verbunden. Periodischer Wiederverbindungsversuch mit WLAN: " + currentDeviceConfig.wifiSsid);
          currentState = STATE_CONNECTING_WIFI;
        }
      } else {
        // Falls ein Client verbunden ist oder keine SSID da ist, Timer zurücksetzen
        lastWifiReconnectAttempt = millis();
      }
      break;
    }

    case STATE_CONNECTING_WIFI:
      Logger::log(LogLevel::Info, "STATE_CONNECTING_WIFI: Versuche zu verbinden.");
      if (connectToWiFi()) { // Nutzt currentDeviceConfig
          currentState = STATE_NORMAL_OPERATION;
          lastWifiReconnectAttempt = 0; // Timer zurücksetzen bei erfolgreicher Verbindung
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
          Logger::log(LogLevel::Error, "Verbindung fehlgeschlagen. Starte/Bleibe im AP-Modus.");
          currentState = STATE_AP_MODE;
          lastWifiReconnectAttempt = millis(); // Nächster Versuch in 5 Minuten
          // AP starten/reinitialisieren, damit Konfiguration weiterhin möglich ist
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

      updateWeatherApi(lastApiCallWeather);
      updatePollenApi(lastApiCallPollen);
      break;

    case STATE_WIFI_CONNECTION_LOST:
      Logger::log(LogLevel::Info, "STATE_WIFI_CONNECTION_LOST: Versuche erneut zu verbinden...");
      if (connectToWiFi()) { // Nutzt currentDeviceConfig
          Logger::log(LogLevel::Info, "Erneute WLAN-Verbindung erfolgreich hergestellt.");
          currentState = STATE_NORMAL_OPERATION;
          lastWifiReconnectAttempt = 0;
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
          lastWifiReconnectAttempt = millis(); // 5 Minuten Timer ab jetzt
          // AP neu starten
          ConfigurationPortal::getInstance().startAPAndWebServer(AP_SSID, AP_PASSWORD);
      }
      break;
  }

  delay(10); // Kurze Pause, um den Watchdog nicht auszulösen und andere Tasks zuzulassen
}
