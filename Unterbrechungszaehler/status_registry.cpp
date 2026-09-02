#include "status_registry.h"

#include <cstring>

#include "config.h"
#include "json_utils.h"

namespace StatusRegistry {
namespace {

Provider providers[AppConfig::STATUS_PROVIDER_CAPACITY]{};
size_t count = 0;

int findIndex(const char *id) {
  if (!id) return -1;
  for (size_t i = 0; i < count; ++i) {
    if (providers[i].id && std::strcmp(providers[i].id, id) == 0) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace

bool registerProvider(const char *id, const char *translationKey, const char *icon, bool visible) {
  if (!id || !translationKey || !icon) return false;
  const int existing = findIndex(id);
  if (existing >= 0) {
    Provider &provider = providers[static_cast<size_t>(existing)];
    provider.translationKey = translationKey;
    provider.icon = icon;
    provider.visible = visible;
    return true;
  }
  if (count >= AppConfig::STATUS_PROVIDER_CAPACITY) return false;
  providers[count++] = Provider{id, translationKey, icon, visible, State::Unknown};
  return true;
}

void setVisible(const char *id, bool visible) {
  const int index = findIndex(id);
  if (index >= 0) providers[static_cast<size_t>(index)].visible = visible;
}

void setState(const char *id, State state) {
  const int index = findIndex(id);
  if (index >= 0) providers[static_cast<size_t>(index)].state = state;
}

State getState(const char *id) {
  const int index = findIndex(id);
  return index >= 0 ? providers[static_cast<size_t>(index)].state : State::Unknown;
}

const char *stateName(State state) {
  switch (state) {
    case State::Disabled: return "disabled";
    case State::Checking: return "checking";
    case State::Ok: return "ok";
    case State::Warning: return "warning";
    case State::Error: return "error";
    case State::NoResponse: return "no_response";
    case State::Inactive: return "inactive";
    case State::Disconnected: return "disconnected";
    case State::AccessPoint: return "ap";
    case State::Stale: return "stale";
    case State::Busy: return "busy";
    case State::Unknown:
    default: return "unknown";
  }
}

size_t providerCount() { return count; }

const Provider *providerAt(size_t index) {
  return index < count ? &providers[index] : nullptr;
}

void appendStatusObject(String &out) {
  out += '{';
  bool first = true;
  for (size_t i = 0; i < count; ++i) {
    const Provider &provider = providers[i];
    if (!provider.id) continue;
    if (!first) out += ',';
    first = false;
    JsonUtils::appendKey(out, provider.id);
    JsonUtils::appendEscapedString(out, stateName(provider.state));
  }
  out += '}';
}

void appendProviderArray(String &out) {
  out += '[';
  bool first = true;
  for (size_t i = 0; i < count; ++i) {
    const Provider &provider = providers[i];
    if (!provider.id) continue;
    if (!first) out += ',';
    first = false;
    out += '{';
    JsonUtils::appendKey(out, "id"); JsonUtils::appendEscapedString(out, provider.id); out += ',';
    JsonUtils::appendKey(out, "key"); JsonUtils::appendEscapedString(out, provider.translationKey); out += ',';
    JsonUtils::appendKey(out, "icon"); JsonUtils::appendEscapedString(out, provider.icon); out += ',';
    JsonUtils::appendKey(out, "visible"); JsonUtils::appendBool(out, provider.visible); out += ',';
    JsonUtils::appendKey(out, "state"); JsonUtils::appendEscapedString(out, stateName(provider.state));
    out += '}';
  }
  out += ']';
}

}  // namespace StatusRegistry
