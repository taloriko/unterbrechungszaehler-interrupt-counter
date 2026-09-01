#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

class RtcService {
public:
  void begin();
  void tick();
  bool present() const { return present_; }
  bool timeValid() const { return timeValid_; }
  bool oscillatorStopFlag() const { return oscillatorStopFlag_; }
  bool lastCheckOk() const { return lastCheckOk_; }
  uint32_t lastCheckAt() const { return lastCheckAt_; }
  uint32_t lastCheckDurationUs() const { return lastCheckDurationUs_; }
  uint32_t checkSequence() const { return checkSequence_; }
  float temperatureC() const { return temperatureC_; }
  bool temperatureValid() const { return temperatureValid_; }
  int32_t systemOffsetSeconds() const { return systemOffsetSeconds_; }
  bool systemOffsetValid() const { return systemOffsetValid_; }

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
  bool probe();
  bool refreshState();
  bool readHardware(struct tm& value, bool& oscillatorStopFlag, float& temperatureC);
  bool cachedTime(struct tm& value) const;
  void configureOutputs();
  void updateSystemOffset();

  bool present_ = false;
  bool timeValid_ = false;
  bool oscillatorStopFlag_ = false;
  bool lastCheckOk_ = false;
  bool temperatureValid_ = false;
  bool systemOffsetValid_ = false;
  float temperatureC_ = 0.0f;
  int32_t systemOffsetSeconds_ = 0;
  time_t cachedEpoch_ = 0;
  uint32_t cacheAtMs_ = 0;
  uint32_t lastCheckAt_ = 0;
  uint32_t lastCheckDurationUs_ = 0;
  uint32_t checkSequence_ = 0;
};
