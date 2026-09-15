#pragma once

#include "interruption_types.h"

namespace InterruptionService {

void begin();
void update();
// Fast feedback-only servicing for long, user-triggered read/export operations.
// It never performs filesystem persistence or statistics work.
void serviceUrgent();

bool capture(InterruptionTypes::EventSource source);
// Used by the work-cycle manager when a previously pending short press becomes
// a confirmed interruption. The supplied timestamp is absolute UTC seconds;
// all existing counters, aggregates, feedback and persistence use the normal
// interruption path.
bool captureAtEpoch(uint32_t epochSeconds, InterruptionTypes::EventSource source);
bool captureWeb();

const InterruptionTypes::Summary &summary();
bool setSoundEnabled(bool enabled);
bool setSoundVolumePercent(uint8_t percent);
bool soundEnabled();
uint32_t physicalButtonCooldownMs();
uint32_t suppressedPhysicalPressCount();
bool hasSuppressedPhysicalPress();
uint32_t lastSuppressedPhysicalPressMs();

}  // namespace InterruptionService
