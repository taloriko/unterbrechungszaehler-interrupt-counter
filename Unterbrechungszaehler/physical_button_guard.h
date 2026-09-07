#pragma once

#include <stdint.h>

namespace PhysicalButtonGuard {

struct State {
  bool hasAccepted = false;
  uint32_t lastAcceptedMs = 0;
  bool hasSuppressed = false;
  uint32_t lastSuppressedMs = 0;
  uint32_t suppressedCount = 0;
};

inline bool accept(State &state, uint32_t nowMs, uint32_t cooldownMs) {
  if (!state.hasAccepted || static_cast<uint32_t>(nowMs - state.lastAcceptedMs) >= cooldownMs) {
    state.hasAccepted = true;
    state.lastAcceptedMs = nowMs;
    return true;
  }
  state.hasSuppressed = true;
  state.lastSuppressedMs = nowMs;
  if (state.suppressedCount != UINT32_MAX) ++state.suppressedCount;
  return false;
}

}  // namespace PhysicalButtonGuard
