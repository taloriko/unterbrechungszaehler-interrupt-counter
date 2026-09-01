#pragma once

#include <Arduino.h>
#include <Wire.h>

class RtcService {
public:
  void begin();
  bool tick();
  bool checkDue() const;

  bool present() const { return present_; }
  bool timeValid() const { return timeValid_; }
  bool oscillatorStopFlag() const { return oscillatorStopFlag_; }
  bool communicationOk() const { return communicationOk_; }
  uint32_t lastCheckAgeMs() const;
  float temperatureC() const { return temperatureC_; }

  bool applyToSystem();
  bool writeSystemTime();
  bool read(struct tm& value, bool& oscillatorStopFlag);

  String dateText() const;
  String timeText() const;

private:
  uint8_t bcdToDec(uint8_t value) const;
  uint8_t decToBcd(uint8_t value) const;
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool sample();
  bool cachedLocalTime(struct tm& value) const;

  bool present_ = false;
  bool timeValid_ = false;
  bool oscillatorStopFlag_ = false;
  bool communicationOk_ = false;
  float temperatureC_ = NAN;
  time_t cachedEpoch_ = 0;
  uint32_t cachedAtMs_ = 0;
  uint32_t lastCheckAtMs_ = 0;
};
