#ifndef NTP_TIME_SYNC_H
#define NTP_TIME_SYNC_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h> // Wichtig: Für _internalNtpUDP
#include <NTPClient.h>
#include "../../logger/LogLevel.h"

class Logger;

class NTPTimeSync {
public:
  static NTPTimeSync& getInstance(const char* ntpServer = nullptr, long timeOffset = 0, long updateInterval = 0) {
      static NTPTimeSync instance(ntpServer, timeOffset, updateInterval);
      return instance;
  }

  bool begin();

  void update();
  String getFormattedTime();
  time_t getEpochTime();

private:
  NTPTimeSync(const char* ntpServer, long timeOffset, long updateInterval);
  NTPTimeSync(const NTPTimeSync&) = delete;
  NTPTimeSync& operator=(const NTPTimeSync&) = delete;

  const char* _ntpServer;
  long _timeOffset;
  long _updateInterval;

  // Jede NTPTimeSync Instanz hat ihre eigene WiFiUDP Instanz
  WiFiUDP _internalNtpUDP;
  NTPClient _NtpClient;
};

#endif // NTP_TIME_SYNC_H