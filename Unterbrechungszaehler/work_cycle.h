#pragma once

#include <Arduino.h>

namespace WorkCycle {

void begin();
void update();

// True while the manual 10-second goodbye screen exclusively owns the local
// interaction/display path. Normal interruption UI servicing is resumed only
// after the overlay has ended. Persistence may therefore be delayed by at most
// this short user-feedback window, while network/time services keep running.
bool exclusiveGoodbyeActive();

uint32_t suppressedPhysicalPressCount();
bool hasSuppressedPhysicalPress();
uint32_t lastSuppressedPhysicalPressMs();

}  // namespace WorkCycle
