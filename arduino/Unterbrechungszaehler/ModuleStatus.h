#pragma once

#include <Arduino.h>

// Fachlicher Zustand eines Moduls. Dieser Zustand ist bewusst getrennt von der
// Laufzeitueberwachung des Watchdogs: Ein optionales Modul kann beispielsweise
// sauber laufen und trotzdem als NOT_DETECTED gemeldet werden.
enum class ModuleState : uint8_t {
  Disabled = 0,
  Initializing,
  Ready,
  Busy,
  NotDetected,
  Degraded,
  Error,
  Timeout
};

inline const char* moduleStateName(ModuleState state) {
  switch (state) {
    case ModuleState::Disabled: return "disabled";
    case ModuleState::Initializing: return "initializing";
    case ModuleState::Ready: return "ready";
    case ModuleState::Busy: return "busy";
    case ModuleState::NotDetected: return "not_detected";
    case ModuleState::Degraded: return "degraded";
    case ModuleState::Error: return "error";
    case ModuleState::Timeout: return "timeout";
    default: return "error";
  }
}

inline bool moduleStateHealthy(ModuleState state) {
  return state == ModuleState::Disabled ||
         state == ModuleState::Initializing ||
         state == ModuleState::Ready ||
         state == ModuleState::Busy ||
         state == ModuleState::NotDetected ||
         state == ModuleState::Degraded;
}

struct ModuleStatus {
  ModuleState state = ModuleState::Initializing;
  const char* detail = "-";
};
