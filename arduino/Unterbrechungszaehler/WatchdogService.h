#pragma once

#include <Arduino.h>

#include "ModuleStatus.h"

class WatchdogService {
public:
  enum Module : uint8_t {
    MainLoop = 0,
    Input,
    Mode,
    Storage,
    Time,
    Rtc,
    Autark,
    Display,
    Led,
    Network,
    Web,
    Analytics,
    Sound,
    Diagnostics,
    Count
  };

  struct State {
    uint32_t lastOkAt = 0;
    uint32_t lastDurationUs = 0;
    uint32_t maxDurationUs = 0;
    uint32_t lastSlowAt = 0;
    uint32_t lastSlowDurationUs = 0;
    uint32_t slowCount = 0;
    uint32_t errorCount = 0;
    bool running = false;
    bool lastResultOk = true;
    ModuleStatus status;
  };

  void begin();
  void beginModule(Module module);
  void endModule(Module module, bool ok = true);
  void recordDuration(Module module, uint32_t durationUs, bool ok = true);
  void heartbeat(Module module, bool ok = true);
  void setStatus(Module module, ModuleState state, const char* detail = "-");
  void feed();

  bool executionHealthy(Module module) const;
  bool healthy(Module module) const;
  uint32_t ageMs(Module module) const;
  uint32_t warnThresholdUs(Module module) const;
  uint32_t heartbeatTimeoutMs(Module module) const;
  const State& state(Module module) const { return states_[module]; }
  static const char* name(Module module);

private:
  State states_[Count] = {};
  uint32_t startedUs_[Count] = {};
};
