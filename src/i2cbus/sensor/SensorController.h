#ifndef SENSORCONTROLLER_H
#define SENSORCONTROLLER_H

#include <M5Unified.h>    // oder M5Stack.h je nach Board
#include <Wire.h>

class SensorController {
public:
  SensorController(uint8_t i2c_addr = 0x26);  // Standard-I2C-Adresse ENV III
  bool begin();                                // Sensor initialisieren
  bool update();                               // neue Messwerte holen
  float getTemperature() const;                // Temperatur in °C
  float getHumidity() const;                   // relative Feuchte in %

private:
  uint8_t _addr;
  float _temperature;
  float _humidity;

  bool readRawData(uint8_t reg, uint16_t &raw);
};

#endif
