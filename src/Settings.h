// W-Lan vom Arduino
#define AP_SSID "Time-Tale"
#define AP_PASSWORD "Time-Tale"

// NTP Konfiguration
#define NTP_SERVER "ntp.metas.ch"
#define TIME_OFFSET 7200 // MESZ (Sommerzeit) = +2 Stunden | MEZ (Winterzeit) = +1 Stunde 
#define UPDATE_INTERVALL 60000 // 1 Minute

// Logger Konfiguration
#define LOG_LEVEL LogLevel::Info

// API Konfiguration
#define WEATHER_API_UPDATE_CYCLE 3600000 // 1 Stunde in Millisekunden
#define WEATHER_API_SERVER "weather.googleapis.com"
#define POLLEN_API_UPDATE_CYCLE 3600000 // 1 Stunde in Millisekunden
#define POLLEN_API_SERVER "pollen.googleapis.com"
#define LATITUDE 46.774
#define LONGITUDE 7.640

// LED Streifen Konfiguration
#define LED_PIN         2 // Beispiel-Pin, passe dies an deinen ESP32 an (Wird nach GPIO nummeriert in der FastLED Library)
#define NUM_LEDS      123 // Die Gesamtzahl deiner LEDs (123)
#define LED_BRIGHTNES 255 // Standard helligkeit der LEDs (0-255)

// Hardware Tasten
#define BUTTON_A 12 // Oberster Button
#define BUTTON_B 11 // Mittlerer Button
#define BUTTON_C 10 // Unterster Button

// Sensor Konfiguration
#define SENSOR_UPDATE_CYCLE 1000 // 1 Sekunde
#define SENSOR_TIME_SHOW_INDOOR 10000 // 10 Sekunde
#define SENSOR_TIME_SHOW_OUTDOOR 5000 // 10 Sekunde
#define SENSOR_TEMPERATUR_CORRECTION -2 // 2°C Nach unten korrigieren
