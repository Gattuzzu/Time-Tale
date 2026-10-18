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
  static NTPTimeSync& getInstance(const char* ntpServer = nullptr, long timeOffset = 0, long updateInterval = 0, bool automaticDst = true) {
      static NTPTimeSync instance(ntpServer, timeOffset, updateInterval, automaticDst);
      if (ntpServer != nullptr) {
          instance.configure(ntpServer, timeOffset, updateInterval, automaticDst);
      }
      return instance;
  }

  void configure(const char* ntpServer, long timeOffset, long updateInterval, bool automaticDst = true);

  bool begin();

  void update();
  void setTimeOffset(long timeOffset);
  void setAutomaticDST(bool enable);
  bool isAutomaticDST() const;
  bool isSummerTime();
  long getEffectiveTimeOffset() const;
  long getBaseTimeOffset() const;
  time_t getUtcEpochTime();

  String getFormattedTime();
  time_t getEpochTime();

  int getHour();
  int getMin();
  bool isTimeSet();

  static bool isEuropeanSummerTime(time_t utcEpoch);
private:
  NTPTimeSync(const char* ntpServer, long timeOffset, long updateInterval, bool automaticDst = true);
  NTPTimeSync(const NTPTimeSync&) = delete;
  NTPTimeSync& operator=(const NTPTimeSync&) = delete;

  void applyTimeOffset();

  String _ntpServer;
  long _timeOffset;       // Basis-Offset in Sekunden (z.B. 3600 für MEZ)
  long _effectiveOffset;  // An NTPClient übergebener Offset (Basis + ggf. 3600 bei Sommerzeit)
  long _updateInterval;
  bool _automaticDst;     // Automatische Umschaltung Sommer-/Winterzeit
  unsigned long _lastDstCheck;

  // Jede NTPTimeSync Instanz hat ihre eigene WiFiUDP Instanz
  WiFiUDP _internalNtpUDP;
  NTPClient _NtpClient;
};

#endif // NTP_TIME_SYNC_H