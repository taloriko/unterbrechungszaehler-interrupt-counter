#pragma once

#include <Arduino.h>

namespace StatusRegistry {

enum class State : uint8_t {
  Disabled,
  Unknown,
  Checking,
  Ok,
  Warning,
  Error,
  NoResponse,
  Inactive,
  Disconnected,
  AccessPoint,
  Stale,
  Busy
};

struct Provider {
  const char *id;
  const char *translationKey;
  const char *icon;
  bool visible;
  State state;
};

bool registerProvider(const char *id, const char *translationKey, const char *icon, bool visible = true);
void setVisible(const char *id, bool visible);
void setState(const char *id, State state);
State getState(const char *id);
const char *stateName(State state);

size_t providerCount();
const Provider *providerAt(size_t index);

// Appends compact JSON fragments used by the bootstrap API.
void appendStatusObject(String &out);
void appendProviderArray(String &out);

}  // namespace StatusRegistry
