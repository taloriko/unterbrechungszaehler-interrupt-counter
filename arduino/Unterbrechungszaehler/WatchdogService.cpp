#include "WatchdogService.h"

#include <esp_task_wdt.h>

#include "Config.h"

void WatchdogService::begin() {
  esp_task_wdt_config_t config = {};
  config.timeout_ms = UicConfig::WATCHDOG_TIMEOUT_MS;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  esp_task_wdt_reconfigure(&config);
  esp_task_wdt_add(nullptr);

  for (uint8_t i = 0; i < Count; i++) {
    states_[i].status.state = ModuleState::Initializing;
    states_[i].status.detail = "boot";
  }

  Serial.printf("[WATCHDOG] Hardware-Watchdog %lu ms | %u Modulpfade\n",
                static_cast<unsigned long>(UicConfig::WATCHDOG_TIMEOUT_MS),
                static_cast<unsigned>(Count));
}

void WatchdogService::beginModule(Module module) {
  if (module >= Count) return;
  State& s = states_[module];
  s.running = true;
  startedUs_[module] = micros();
}

void WatchdogService::endModule(Module module, bool ok) {
  if (module >= Count) return;
  State& s = states_[module];
  const uint32_t duration = micros() - startedUs_[module];
  s.running = false;
  s.lastDurationUs = duration;
  if (duration > s.maxDurationUs) s.maxDurationUs = duration;
  if (duration > UicConfig::MODULE_WARN_MS * 1000UL) s.slowCount++;
  s.lastResultOk = ok;
  if (!ok) s.errorCount++;
  if (ok) s.lastOkAt = millis();
}

void WatchdogService::heartbeat(Module module, bool ok) {
  if (module >= Count) return;
  State& s = states_[module];
  s.running = false;
  s.lastDurationUs = 0;
  s.lastResultOk = ok;
  if (!ok) s.errorCount++;
  if (ok) s.lastOkAt = millis();
}

void WatchdogService::setStatus(Module module, ModuleState state, const char* detail) {
  if (module >= Count) return;
  states_[module].status.state = state;
  states_[module].status.detail = detail ? detail : "-";
}

void WatchdogService::feed() {
  esp_task_wdt_reset();
}

uint32_t WatchdogService::ageMs(Module module) const {
  if (module >= Count) return 0xFFFFFFFFUL;
  const State& s = states_[module];
  if (s.lastOkAt == 0) return 0xFFFFFFFFUL;
  return millis() - s.lastOkAt;
}

bool WatchdogService::executionHealthy(Module module) const {
  if (module >= Count) return false;
  const State& s = states_[module];

  if (s.running) {
    const uint32_t runningMs = (micros() - startedUs_[module]) / 1000UL;
    if (runningMs >= UicConfig::WATCHDOG_TIMEOUT_MS) return false;
  }

  if (!s.lastResultOk) return false;
  if (s.lastOkAt == 0) return true;
  return millis() - s.lastOkAt < UicConfig::WATCHDOG_TIMEOUT_MS;
}

bool WatchdogService::healthy(Module module) const {
  if (module >= Count) return false;
  return executionHealthy(module) && moduleStateHealthy(states_[module].status.state);
}

const char* WatchdogService::name(Module module) {
  switch (module) {
    case MainLoop: return "MainLoop";
    case Input: return "Input";
    case Mode: return "Mode";
    case Storage: return "Storage";
    case Time: return "Time";
    case Rtc: return "RTC";
    case Autark: return "Autark";
    case Display: return "Display";
    case Led: return "LED";
    case Network: return "Network";
    case Web: return "Web";
    case Analytics: return "Analytics";
    case Sound: return "Sound";
    case Diagnostics: return "Diagnostics";
    default: return "?";
  }
}
