#pragma once

#include <Arduino.h>

namespace WorkCycle {

void begin();
void update();

uint32_t suppressedPhysicalPressCount();
bool hasSuppressedPhysicalPress();
uint32_t lastSuppressedPhysicalPressMs();

}  // namespace WorkCycle
