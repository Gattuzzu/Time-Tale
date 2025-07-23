#include "SensorController.h"

SensorController::SensorController(uint8_t i2c_addr)
  : _addr(i2c_addr), _temperature(0), _humidity(0)
{}

bool SensorController::begin() {
  Wire.begin();
  // Ein kurzer Test, ob das Gerät antwortet
  Wire.beginTransmission(_addr);
  if (Wire.endTransmission() != 0) {
    return false;  // kein ACK vom Sensor
  }
  return true;
}

bool SensorController::readRawData(uint8_t reg, uint16_t &raw) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(_addr, (uint8_t)2);
  if (Wire.available() < 2) return false;
  raw = (Wire.read() << 8) | Wire.read();
  return true;
}

bool SensorController::update() {
  uint16_t rawT, rawH;
  // Register-Adressen je nach ENV-Dokumentation
  if (!readRawData(0x00, rawH)) return false;  // Feuchte-Register
  if (!readRawData(0x02, rawT)) return false;  // Temperatur-Register

  // Umrechnung gemäß Datenblatt (Beispiel-Formeln!)
  _humidity    = ((float)rawH / 65535.0f) * 100.0f;
  _temperature = ((float)rawT / 65535.0f) * 165.0f - 40.0f;

  return true;
}

float SensorController::getTemperature() const {
  return _temperature;
}

float SensorController::getHumidity() const {
  return _humidity;
}
