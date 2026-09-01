#pragma once

#include <Arduino.h>

class StorageService;
class TimeService;
class LedService;

class AutarkService {
public:
  void begin(StorageService* storage, TimeService* time, LedService* led);
  void tick();
  bool enter();
  bool leave();
  bool addEvent();
  bool deleteLastEvent();

  bool active() const { return active_; }
  uint32_t sessionId() const { return sessionId_; }
  uint32_t sessionEvents() const { return sessionEvents_; }
  uint32_t elapsedSeconds() const;
  uint32_t lastElapsedSeconds() const { return lastElapsedSeconds_; }

private:
  StorageService* storage_ = nullptr;
  TimeService* time_ = nullptr;
  LedService* led_ = nullptr;

  bool active_ = false;
  uint32_t sessionId_ = 0;
  uint32_t sessionEvents_ = 0;
  uint32_t lastElapsedSeconds_ = 0;
  uint32_t pendingEndSession_ = 0;
  uint64_t sessionStartedUs_ = 0;
};
