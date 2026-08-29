#pragma once

#include <Arduino.h>

class CounterService;
class AutarkService;
class LedService;
class DisplayService;

class InputService {
public:
  void begin(CounterService* counter, AutarkService* autark, LedService* led, DisplayService* display);
  void tick();

  bool autarkSwitchOn() const { return autarkStable_ == LOW; }

private:
  void processButton();
  void processAutarkSwitch();

  CounterService* counter_ = nullptr;
  AutarkService* autark_ = nullptr;
  LedService* led_ = nullptr;
  DisplayService* display_ = nullptr;

  bool buttonRaw_ = HIGH;
  bool buttonStable_ = HIGH;
  uint32_t buttonChangedAt_ = 0;
  uint32_t buttonPressedAt_ = 0;

  bool autarkRaw_ = HIGH;
  bool autarkStable_ = HIGH;
  uint32_t autarkChangedAt_ = 0;
};
