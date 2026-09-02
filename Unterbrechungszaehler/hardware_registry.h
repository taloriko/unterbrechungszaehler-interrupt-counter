#pragma once

#include <Arduino.h>

namespace HardwareRegistry {

void begin();
void update();

void probeAll();
bool hasModule(const char *moduleId);
bool probe(const char *moduleId);
bool action(const char *moduleId, const char *actionId);
bool anyChecking();

// Appends current cached state only. Calling this never performs a hardware
// probe and avoids a second temporary String when the API builds a response.
void appendJson(String &out);

}  // namespace HardwareRegistry
