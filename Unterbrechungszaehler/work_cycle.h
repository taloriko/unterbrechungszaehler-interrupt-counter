#pragma once

#include <Arduino.h>

namespace WorkCycle {

void begin();
void update();

// True while the manual 10-second goodbye screen exclusively owns the local
// interaction/display path. Physical edges are ignored and normal interruption
// display servicing resumes only after the overlay has ended.
bool exclusiveGoodbyeActive();
// Both short start acknowledgement and goodbye have exclusive OLED ownership.
bool exclusiveDisplayActive();
// Read-only live state; no flash read, timer, or extra state allocation.
bool active();
const char *stateName();

uint32_t suppressedPhysicalPressCount();
bool hasSuppressedPhysicalPress();
uint32_t lastSuppressedPhysicalPressMs();

}  // namespace WorkCycle
