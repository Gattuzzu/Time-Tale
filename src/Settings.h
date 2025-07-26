// W-Lan vom Arduino
#define AP_SSID "Time-Tale"
#define AP_PASSWORD "Time-Tale"

// NTP Konfiguration
#define UPDATE_INTERVALL 60000 // 1 Minute

// Logger Konfiguration
#define LOG_LEVEL LogLevel::Info

// API Konfiguration
#define WEATHER_API_SERVER "weather.googleapis.com"
#define POLLEN_API_SERVER "pollen.googleapis.com"

// LED Streifen Konfiguration
#define LED_PIN         2 // Beispiel-Pin, passe dies an deinen ESP32 an (Wird nach GPIO nummeriert in der FastLED Library)
#define NUM_LEDS      123 // Die Gesamtzahl deiner LEDs (123)

// Hardware Tasten
#define BUTTON_A 12 // Oberster Button
#define BUTTON_B 11 // Mittlerer Button
#define BUTTON_C 10 // Unterster Button

// Sensor Konfiguration
#define SENSOR_UPDATE_CYCLE 1000 // 1 Sekunde
#define SENSOR_TEMPERATUR_CORRECTION -2 // 2°C Nach unten korrigieren
