#pragma once

#include <Arduino.h>
#include <Wire.h>

class RtcService {
public:
  void begin();
  bool present() const { return present_; }
  bool timeValid() const { return timeValid_; }
  bool oscillatorStopFlag() const { return oscillatorStopFlag_; }

  bool applyToSystem();
  bool writeSystemTime();
  bool read(struct tm& value, bool& oscillatorStopFlag);

  String dateText();
  String timeText();

private:
  uint8_t bcdToDec(uint8_t value) const;
  uint8_t decToBcd(uint8_t value) const;
  bool readRegister(uint8_t reg, uint8_t& value);
  bool writeRegister(uint8_t reg, uint8_t value);
  void refreshState();

  bool present_ = false;
  bool timeValid_ = false;
  bool oscillatorStopFlag_ = false;
};
