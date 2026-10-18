// Firmware Version
#define FIRMWARE_VERSION "v1.1"

// W-Lan vom Arduino
#define AP_SSID "Time-Tale"
#define AP_PASSWORD "Time-Tale"

// Menu
#define IP_ADDRESS_SHOW_CYCLE 1000 // 1 Sekunde
#define MENU_OPEN_TIME 8000 // 10 Sekunden

// NTP Konfiguration
#define UPDATE_INTERVALL 900000UL // 15 Minuten (15 * 60 * 1000 ms)

// WLAN Reconnect Konfiguration
#define WIFI_RECONNECT_INTERVAL 300000UL // 5 Minuten (5 * 60 * 1000 ms)

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
