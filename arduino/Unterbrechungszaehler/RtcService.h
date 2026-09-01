#pragma once

#include <Arduino.h>
#include <Wire.h>

class RtcService {
public:
  void begin();
  void tick();

  bool present() const { return present_; }
  bool timeValid() const { return timeValid_; }
  bool oscillatorStopFlag() const { return oscillatorStopFlag_; }
  bool communicationOk() const { return communicationOk_; }
  float temperatureC() const { return temperatureC_; }
  uint32_t lastCheckAt() const { return lastCheckAt_; }
  uint32_t lastCheckAgeMs() const;
  int32_t systemDifferenceSeconds() const { return systemDifferenceSeconds_; }

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
  bool refreshState();
  bool readTemperature(float& value);
  void updateDifference();

  bool present_ = false;
  bool timeValid_ = false;
  bool oscillatorStopFlag_ = false;
  bool communicationOk_ = false;
  float temperatureC_ = NAN;
  struct tm cachedTime_ = {};
  bool cachedTimeAvailable_ = false;
  uint32_t lastCheckAt_ = 0;
  int32_t systemDifferenceSeconds_ = 0;
};
