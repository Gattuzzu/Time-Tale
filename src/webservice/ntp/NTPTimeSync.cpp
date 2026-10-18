#include "NTPTimeSync.h"
#include "../../logger/Logger.h"
#include <time.h>

bool NTPTimeSync::isEuropeanSummerTime(time_t utcEpoch) {
  if (utcEpoch == 0) {
    return false;
  }
  struct tm tm_utc;
  gmtime_r(&utcEpoch, &tm_utc);
  int month = tm_utc.tm_mon + 1; // 1 = Januar .. 12 = Dezember
  if (month < 3 || month > 10) {
    return false;
  }
  if (month > 3 && month < 10) {
    return true;
  }

  // tm_wday: 0 = Sonntag, 1 = Montag, ..., 6 = Samstag
  // Letzter Sonntag im März / Oktober (beide Monate haben 31 Tage):
  int lastSunday = 31 - ((tm_utc.tm_wday + (31 - tm_utc.tm_mday)) % 7);
  if (month == 3) {
    if (tm_utc.tm_mday < lastSunday) {
      return false;
    }
    if (tm_utc.tm_mday > lastSunday) {
      return true;
    }
    // Beginn am letzten Sonntag im März um 01:00 UTC (02:00 MEZ -> 03:00 MESZ)
    return tm_utc.tm_hour >= 1;
  } else { // month == 10
    if (tm_utc.tm_mday < lastSunday) {
      return true;
    }
    if (tm_utc.tm_mday > lastSunday) {
      return false;
    }
    // Ende am letzten Sonntag im Oktober um 01:00 UTC (03:00 MESZ -> 02:00 MEZ)
    return tm_utc.tm_hour < 1;
  }
}

time_t NTPTimeSync::getUtcEpochTime() {
  if (!_NtpClient.isTimeSet()) {
    return 0;
  }
  return (time_t)(_NtpClient.getEpochTime() - _effectiveOffset);
}

bool NTPTimeSync::isSummerTime() {
  if (!_NtpClient.isTimeSet()) {
    return false;
  }
  return isEuropeanSummerTime(getUtcEpochTime());
}

void NTPTimeSync::applyTimeOffset() {
  long targetOffset = _timeOffset;
  if (_automaticDst && _NtpClient.isTimeSet()) {
    time_t utc = getUtcEpochTime();
    if (isEuropeanSummerTime(utc)) {
      targetOffset += 3600; // +1 Stunde für MESZ
    }
  }

  if (targetOffset != _effectiveOffset || _lastDstCheck == 0) {
    _effectiveOffset = targetOffset;
    _NtpClient.setTimeOffset(_effectiveOffset);
    if (_NtpClient.isTimeSet()) {
      Logger::log(LogLevel::Info, "NTPTimeSync: Effektiver Zeitversatz gesetzt auf " +
                  String(_effectiveOffset / 3600) + "h (Basis: " +
                  String(_timeOffset / 3600) + "h, Sommerzeit: " +
                  (isSummerTime() ? "Ja/MESZ" : "Nein/MEZ") + ")");
    }
  }
  _lastDstCheck = millis();
}

void NTPTimeSync::configure(const char* ntpServer, long timeOffset, long updateInterval, bool automaticDst) {
  if (ntpServer != nullptr && strlen(ntpServer) > 0) {
    _ntpServer = ntpServer;
    _NtpClient.setPoolServerName(_ntpServer.c_str());
  }
  _timeOffset = timeOffset;
  _updateInterval = updateInterval;
  _automaticDst = automaticDst;
  _NtpClient.setUpdateInterval(_updateInterval);
  applyTimeOffset();
}

bool NTPTimeSync::begin() {
  // Überprüfen, ob WLAN bereits verbunden ist
  if (WiFi.status() != WL_CONNECTED) {
      Logger::log(LogLevel::Error, "NTPTimeSync::begin(): WLAN ist nicht verbunden. Bitte stellen Sie zuerst eine WLAN-Verbindung her.");
      return false;
  }

  if (_ntpServer.length() > 0) {
    _NtpClient.setPoolServerName(_ntpServer.c_str());
  }

  // Der NTPClient kümmert sich um das `begin()` des zugrunde liegenden UDP-Sockets.
  _NtpClient.begin();

  Logger::log(LogLevel::Info, "Synchronisiere Zeit mit NTP...");
  // Erster Sync-Versuch beim Start erzwingen (forceUpdate)
  bool updateSuccess = _NtpClient.forceUpdate();
  if (!updateSuccess) {
    int attempts = 0;
    while (!updateSuccess && attempts < 10) { // Max. 10 Versuche
      delay(500);
      Logger::log(LogLevel::Info, ".");
      updateSuccess = _NtpClient.forceUpdate();
      attempts++;
    }
  }

  if (updateSuccess) {
    applyTimeOffset();
    time_t now = (time_t)_NtpClient.getEpochTime();
    struct timeval tv = { .tv_sec = now, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    Logger::log(LogLevel::Info, "Zeit synchronisiert: " + getFormattedTime() + 
                (isSummerTime() ? " (MESZ - Sommerzeit)" : " (MEZ - Winterzeit)"));
    return true;
  } else {
    Logger::log(LogLevel::Error, "NTPTimeSync: Zeit konnte nicht synchronisiert werden. NTP-Server oder Netzwerkproblem.");
    return false;
  }
}

// Implementierung der update() Methode
void NTPTimeSync::update() {
  if (WiFi.status() == WL_CONNECTED) {
    _NtpClient.update();
  }
  if (_NtpClient.isTimeSet()) {
    if (millis() - _lastDstCheck >= 10000 || _lastDstCheck == 0) {
      applyTimeOffset();
    }
  }
}

bool NTPTimeSync::isTimeSet() {
  return _NtpClient.isTimeSet();
}

void NTPTimeSync::setTimeOffset(long timeOffset) {
  _timeOffset = timeOffset;
  applyTimeOffset();
}

void NTPTimeSync::setAutomaticDST(bool enable) {
  _automaticDst = enable;
  applyTimeOffset();
}

bool NTPTimeSync::isAutomaticDST() const {
  return _automaticDst;
}

long NTPTimeSync::getEffectiveTimeOffset() const {
  return _effectiveOffset;
}

long NTPTimeSync::getBaseTimeOffset() const {
  return _timeOffset;
}

String NTPTimeSync::getFormattedTime() {
  return _NtpClient.getFormattedTime();
}

time_t NTPTimeSync::getEpochTime() {
  return _NtpClient.getEpochTime();
}

// Implementierung des privaten Konstruktors
NTPTimeSync::NTPTimeSync(const char* ntpServer, long timeOffset, long updateInterval, bool automaticDst)
  : _ntpServer((ntpServer != nullptr) ? ntpServer : ""),
    _timeOffset(timeOffset),
    _effectiveOffset(timeOffset),
    _updateInterval(updateInterval),
    _automaticDst(automaticDst),
    _lastDstCheck(0),
    _NtpClient(_internalNtpUDP, (_ntpServer.length() > 0) ? _ntpServer.c_str() : nullptr, timeOffset, updateInterval) {
}

int NTPTimeSync::getHour() {
  return _NtpClient.getHours();
}

int NTPTimeSync::getMin() {
  return _NtpClient.getMinutes();
}