#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "Config.h"

class RtcService;

class TimeService {
public:
  bool begin(RtcService* rtc);
  void tick();

  bool valid() const;
  uint32_t epoch() const;
  TimeSource source() const { return source_; }
  const String& primaryNtp() const { return primaryNtp_; }

  bool setFromBrowser(uint32_t epoch);
  bool setFromRtc();
  bool setPrimaryNtp(const String& host);
  bool validNtpHost(const String& host) const;
  void notifyNtpSynchronized();

  String localDate() const;
  String localTime() const;

private:
  void applyTimezone();
  void startNtp();

  Preferences preferences_;
  RtcService* rtc_ = nullptr;
  String primaryNtp_ = UicConfig::DEFAULT_NTP_1;
  TimeSource source_ = TimeSource::None;
  bool ntpNotificationPending_ = false;
};
