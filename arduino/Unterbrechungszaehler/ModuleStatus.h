#pragma once

#include <Arduino.h>

enum class HealthLevel : uint8_t {
  Ok = 0,
  Degraded = 1,
  Error = 2,
  Disabled = 3
};

inline const char* healthLevelName(HealthLevel level) {
  switch (level) {
    case HealthLevel::Ok: return "ok";
    case HealthLevel::Degraded: return "degraded";
    case HealthLevel::Error: return "error";
    case HealthLevel::Disabled: return "disabled";
    default: return "error";
  }
}

struct ModuleStatus {
  const char* name = "-";
  HealthLevel health = HealthLevel::Disabled;
  const char* detail = "-";
};
