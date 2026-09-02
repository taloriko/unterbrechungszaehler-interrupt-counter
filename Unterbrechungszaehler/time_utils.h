#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace TimeUtils {

uint64_t monotonicMs();
bool isPlausibleDateTime(const HardwareTypes::DateTimeValue &value, uint16_t minYear, uint16_t maxYear);
bool dateTimeToEpochUtc(const HardwareTypes::DateTimeValue &value, int64_t &epochSeconds);
bool epochUtcToDateTime(int64_t epochSeconds, HardwareTypes::DateTimeValue &value);
bool isPlausibleEpochMs(int64_t epochMs, uint16_t minYear, uint16_t maxYear);

}  // namespace TimeUtils
