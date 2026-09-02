#pragma once

#include <Arduino.h>

#include "hardware_types.h"

namespace RtcDs3231 {

bool begin();
void probe();

bool enabled();
bool detected();
StatusRegistry::State health();
uint32_t lastCheckMs();
uint64_t lastCheckMonotonicMs();
const char *lastError();
HardwareTypes::FeedbackType feedbackType();

bool oscillatorStopFlag();
float temperatureC();
const HardwareTypes::DateTimeValue &dateTime();
bool setDateTime(const HardwareTypes::DateTimeValue &value);

}  // namespace RtcDs3231
