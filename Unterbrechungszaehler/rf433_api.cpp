#include "rf433_api.h"

#include "hardware_registry.h"
#include "interruption_service.h"
#include "interruption_store.h"
#include "json_utils.h"
#include "rf433_cc1101.h"
#include "rf433_service.h"
#include "source_registry.h"

namespace Rf433Api {
namespace {

void fieldString(String &out, const char *key, const char *value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendEscapedString(out, value ? value : "");
  if (comma) out += ',';
}

void fieldBool(String &out, const char *key, bool value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendBool(out, value);
  if (comma) out += ',';
}

void fieldUInt(String &out, const char *key, uint32_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt(out, value);
  if (comma) out += ',';
}

void fieldUInt64(String &out, const char *key, uint64_t value, bool comma = true) {
  JsonUtils::appendKey(out, key);
  JsonUtils::appendUInt64(out, value);
  if (comma) out += ',';
}

void appendSource(String &out, uint8_t sourceId, bool assigned, bool bound, uint32_t retainedCount, bool &first) {
  if (!first) out += ',';
  first = false;
  out += '{';
  fieldUInt(out, "id", sourceId);
  fieldString(out, "kind", SourceRegistry::sourceKind(sourceId));
  fieldString(out, "name", SourceRegistry::sourceName(sourceId));
  fieldBool(out, "assigned", assigned);
  fieldBool(out, "bound", bound);
  fieldUInt(out, "retainedCount", retainedCount);

  const SourceRegistry::Entry *entry = SourceRegistry::entryForSource(sourceId);
  if (entry && entry->assigned) {
    fieldUInt(out, "bitCount", entry->bitCount);
    fieldUInt(out, "pulseBucket", entry->pulseBucket);
    fieldUInt(out, "code", entry->code, false);
  } else {
    fieldUInt(out, "code", 0, false);
  }
  out += '}';
}

}  // namespace

String buildSourcesJson(bool includeRetainedCounts) {
  uint32_t counts[SourceRegistry::SOURCE_ID_MAX + 1U]{};
  const bool rawReady = InterruptionStore::ready();
  uint16_t serviceCounter = 0;
  if (rawReady && includeRetainedCounts) {
    const uint64_t first = InterruptionStore::oldestSequence();
    const uint64_t last = InterruptionStore::newestSequence();
    for (uint64_t sequence = first; sequence != 0 && sequence <= last; ++sequence) {
      InterruptionTypes::RawEvent raw;
      if (InterruptionStore::readSequence(sequence, raw) && raw.sourceId <= SourceRegistry::SOURCE_ID_MAX) {
        if (counts[raw.sourceId] < UINT32_MAX) ++counts[raw.sourceId];
      }
      if ((++serviceCounter & 0x1FU) == 0U) {
        HardwareRegistry::update();
        Rf433Service::update();
        InterruptionService::serviceUrgent();
        delay(0);
      }
    }
  }

  const auto &radio = Rf433Cc1101::info();
  const auto &learn = SourceRegistry::learnState();
  const uint32_t elapsed = learn.active ? static_cast<uint32_t>(millis() - learn.startedMs) : 0U;
  const uint32_t remaining = learn.active && elapsed < learn.timeoutMs ? learn.timeoutMs - elapsed : 0U;

  String out;
  out.reserve(2600);
  out += '{';
  fieldBool(out, "ok", true);
  fieldBool(out, "rawReady", rawReady);
  fieldUInt64(out, "oldestSequence", InterruptionStore::oldestSequence());
  fieldUInt64(out, "newestSequence", InterruptionStore::newestSequence());
  fieldUInt(out, "sourceIdBits", 4);
  fieldUInt(out, "radioCapacity", SourceRegistry::RADIO_SOURCE_CAPACITY);
  fieldUInt(out, "assignedRadio", static_cast<uint32_t>(SourceRegistry::assignedRadioCount()));
  fieldUInt(out, "boundRadio", static_cast<uint32_t>(SourceRegistry::boundRadioCount()));
  fieldBool(out, "retainedCountsIncluded", includeRetainedCounts);

  JsonUtils::appendKey(out, "rf");
  out += '{';
  fieldBool(out, "ready", radio.ready);
  fieldString(out, "error", radio.error);
  fieldUInt(out, "frequencyHz", 433920000UL);
  fieldUInt(out, "partNumber", radio.partNumber);
  fieldUInt(out, "version", radio.version);
  fieldUInt(out, "decodedFrames", radio.decodedFrames);
  fieldUInt(out, "rejectedFrames", radio.rejectedFrames);
  fieldUInt(out, "overflowFrames", radio.overflowFrames);
  fieldUInt(out, "lastCode", radio.lastFrame.code);
  fieldUInt(out, "lastBits", radio.lastFrame.bitCount);
  fieldUInt(out, "lastPulseBucket", radio.lastFrame.pulseBucket, false);
  out += "},";

  JsonUtils::appendKey(out, "learn");
  out += '{';
  fieldBool(out, "active", learn.active);
  fieldUInt(out, "targetSourceId", learn.targetSourceId);
  fieldUInt(out, "remainingMs", remaining);
  fieldString(out, "name", learn.pendingName);
  fieldString(out, "error", learn.error, false);
  out += "},";

  JsonUtils::appendKey(out, "sources");
  out += '[';
  bool firstSource = true;
  for (uint8_t sourceId = SourceRegistry::SOURCE_ID_MASTER; sourceId <= SourceRegistry::SOURCE_ID_HARDWARE; ++sourceId) {
    appendSource(out, sourceId, true, sourceId == SourceRegistry::SOURCE_ID_MASTER, counts[sourceId], firstSource);
  }
  for (size_t i = 0; i < SourceRegistry::RADIO_SOURCE_CAPACITY; ++i) {
    const SourceRegistry::Entry *entry = SourceRegistry::entryAt(i);
    if (!entry || !entry->assigned) continue;
    appendSource(out, entry->sourceId, entry->assigned, entry->bound, counts[entry->sourceId], firstSource);
  }
  out += ']';
  out += '}';
  return out;
}

}  // namespace Rf433Api
