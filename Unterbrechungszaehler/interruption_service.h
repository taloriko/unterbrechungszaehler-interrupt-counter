#pragma once

#include "interruption_types.h"

namespace InterruptionService {

void begin();
void update();
// Fast feedback-only servicing for long, user-triggered read/export operations.
// It never performs filesystem persistence or statistics work.
void serviceUrgent();

bool capture(InterruptionTypes::EventSource source);
bool captureWeb();

const InterruptionTypes::Summary &summary();
bool setSoundEnabled(bool enabled);
bool setSoundVolumePercent(uint8_t percent);
bool soundEnabled();

}  // namespace InterruptionService
