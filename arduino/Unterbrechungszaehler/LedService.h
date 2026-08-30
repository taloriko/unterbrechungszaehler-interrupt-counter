#pragma once

#include <Arduino.h>

class LedService {
public:
  struct Step {
    bool on;
    uint16_t durationMs;
  };

  void begin();
  void tick();

  void setButtonPressed(bool pressed);
  void signalStored();
  void signalDeleted();
  void signalWarning();
  void off();

private:
  void startPattern(const Step* pattern, uint8_t length);
  void write(bool on);

  const Step* pattern_ = nullptr;
  uint8_t patternLength_ = 0;
  uint8_t patternIndex_ = 0;
  uint32_t stepStartedAt_ = 0;
  bool buttonPressed_ = false;
  bool outputState_ = false;
};
