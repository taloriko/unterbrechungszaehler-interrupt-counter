#pragma once

#include <Arduino.h>

class StorageService;
class TimeService;
class LedService;
class SoundService;

class CounterService {
public:
  void begin(StorageService* storage, TimeService* time, LedService* led, SoundService* sound);
  bool addNormalEvent(bool physicalButton);
  bool deleteNormalEvent();

  uint32_t pulseSequence() const { return pulseSequence_; }
  uint32_t actionSequence() const { return actionSequence_; }
  uint8_t actionKind() const { return actionKind_; }

private:
  void noteAction(uint8_t kind);

  StorageService* storage_ = nullptr;
  TimeService* time_ = nullptr;
  LedService* led_ = nullptr;
  SoundService* sound_ = nullptr;
  uint32_t pulseSequence_ = 0;
  uint32_t actionSequence_ = 0;
  uint8_t actionKind_ = 0;
};
