#include "source_registry.h"

#include <Preferences.h>
#include <cstring>

#include "serial_log.h"

namespace SourceRegistry {
namespace {

constexpr uint32_t MAGIC = 0x52534652UL;  // "RFSR"
constexpr uint16_t VERSION = 1;
constexpr char PREF_NAMESPACE[] = "rf433src";
constexpr char PREF_KEY[] = "registry";
constexpr uint8_t FLAG_ASSIGNED = 0x01;
constexpr uint8_t FLAG_BOUND = 0x02;

#pragma pack(push, 1)
struct StoredEntry {
  uint8_t flags = 0;
  uint8_t bitCount = 0;
  uint8_t pulseBucket = 0;
  uint8_t reserved = 0;
  uint32_t code = 0;
  char name[SOURCE_NAME_MAX_BYTES + 1]{};
};

struct StoredRegistry {
  uint32_t magic = MAGIC;
  uint16_t version = VERSION;
  uint16_t size = 0;
  StoredEntry entries[RADIO_SOURCE_CAPACITY]{};
  uint32_t crc = 0;
};
#pragma pack(pop)

static_assert(sizeof(StoredEntry) == 32, "RF source entry must stay compact");
static_assert(sizeof(StoredRegistry) == 332, "RF source registry storage size changed");

StoredRegistry registry;
Entry decoded[RADIO_SOURCE_CAPACITY];
LearnState learn;
bool initialized = false;

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0UL);
    }
  }
  return ~crc;
}

void copyName(char *out, const char *value) {
  if (!out) return;
  if (!value) value = "";
  size_t length = strlen(value);
  if (length > SOURCE_NAME_MAX_BYTES) length = SOURCE_NAME_MAX_BYTES;
  memcpy(out, value, length);
  out[length] = '\0';

  // Do not leave a trailing UTF-8 continuation byte after byte-based clipping.
  while (length > 0 && (static_cast<uint8_t>(out[length - 1U]) & 0xC0U) == 0x80U) {
    out[--length] = '\0';
  }
}

void decodeEntries() {
  for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
    const StoredEntry &stored = registry.entries[i];
    Entry &entry = decoded[i];
    entry = Entry{};
    entry.sourceId = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
    entry.assigned = (stored.flags & FLAG_ASSIGNED) != 0;
    entry.bound = (stored.flags & FLAG_BOUND) != 0;
    entry.bitCount = stored.bitCount;
    entry.pulseBucket = stored.pulseBucket;
    entry.code = stored.code;
    copyName(entry.name, stored.name);
  }
}

bool validStoredRegistry(const StoredRegistry &candidate) {
  if (candidate.magic != MAGIC || candidate.version != VERSION || candidate.size != sizeof(StoredRegistry)) return false;
  return candidate.crc == crc32(reinterpret_cast<const uint8_t *>(&candidate), sizeof(StoredRegistry) - sizeof(candidate.crc));
}

bool save() {
  registry.magic = MAGIC;
  registry.version = VERSION;
  registry.size = sizeof(StoredRegistry);
  registry.crc = crc32(reinterpret_cast<const uint8_t *>(&registry), sizeof(StoredRegistry) - sizeof(registry.crc));
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) return false;
  const size_t written = prefs.putBytes(PREF_KEY, &registry, sizeof(registry));
  prefs.end();
  if (written != sizeof(registry)) return false;
  decodeEntries();
  return true;
}

int radioIndex(uint8_t sourceId) {
  if (sourceId < SOURCE_ID_RADIO_FIRST || sourceId > SOURCE_ID_RADIO_LAST) return -1;
  return static_cast<int>(sourceId - SOURCE_ID_RADIO_FIRST);
}

uint8_t firstUnassignedSource() {
  for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
    if ((registry.entries[i].flags & FLAG_ASSIGNED) == 0) return static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
  }
  return SOURCE_ID_UNKNOWN;
}

bool pulseBucketMatches(uint8_t stored, uint8_t received) {
  const uint8_t difference = stored > received ? static_cast<uint8_t>(stored - received) : static_cast<uint8_t>(received - stored);
  return difference <= 3U;
}

const char *reservedName(uint8_t sourceId) {
  switch (sourceId) {
    case SOURCE_ID_MASTER: return "Master";
    case SOURCE_ID_WEB: return "Web";
    case SOURCE_ID_SOFTWARE: return "Software";
    case SOURCE_ID_API: return "API";
    case SOURCE_ID_HARDWARE: return "Hardware";
    case SOURCE_ID_UNKNOWN:
    default: return "Unknown";
  }
}

}  // namespace

bool begin() {
  if (initialized) return true;
  initialized = true;
  registry = StoredRegistry{};
  Preferences prefs;
  if (prefs.begin(PREF_NAMESPACE, true)) {
    if (prefs.getBytesLength(PREF_KEY) == sizeof(registry)) {
      StoredRegistry candidate;
      if (prefs.getBytes(PREF_KEY, &candidate, sizeof(candidate)) == sizeof(candidate) && validStoredRegistry(candidate)) {
        registry = candidate;
      } else {
        SerialLog::warning("RF433", "Source registry CRC/version invalid; radio bindings ignored, history remains untouched");
      }
    }
    prefs.end();
  }
  decodeEntries();
  learn = LearnState{};
  SerialLog::infof("RF433", "Source registry ready | assigned=%u/%u | bound=%u",
                   static_cast<unsigned int>(assignedRadioCount()),
                   static_cast<unsigned int>(RADIO_SOURCE_CAPACITY),
                   static_cast<unsigned int>(boundRadioCount()));
  return true;
}

void update() {
  if (!learn.active) return;
  if (static_cast<uint32_t>(millis() - learn.startedMs) < learn.timeoutMs) return;
  learn.active = false;
  learn.error = "timeout";
  SerialLog::info("RF433", "Learn mode timed out");
}

const Entry *entryForSource(uint8_t sourceId) {
  const int index = radioIndex(sourceId);
  return index >= 0 ? &decoded[index] : nullptr;
}

const Entry *entryAt(size_t radioIndexValue) {
  return radioIndexValue < RADIO_SOURCE_CAPACITY ? &decoded[radioIndexValue] : nullptr;
}

size_t assignedRadioCount() {
  size_t count = 0;
  for (const Entry &entry : decoded) if (entry.assigned) ++count;
  return count;
}

size_t boundRadioCount() {
  size_t count = 0;
  for (const Entry &entry : decoded) if (entry.bound) ++count;
  return count;
}

const char *sourceName(uint8_t sourceId) {
  if (sourceId < SOURCE_ID_RADIO_FIRST) return reservedName(sourceId);
  const Entry *entry = entryForSource(sourceId);
  if (!entry || !entry->assigned || !entry->name[0]) return "Radio";
  return entry->name;
}

const char *sourceKind(uint8_t sourceId) {
  switch (sourceId) {
    case SOURCE_ID_MASTER: return "master";
    case SOURCE_ID_WEB: return "web";
    case SOURCE_ID_SOFTWARE: return "software";
    case SOURCE_ID_API: return "api";
    case SOURCE_ID_HARDWARE: return "hardware";
    default: return sourceId >= SOURCE_ID_RADIO_FIRST && sourceId <= SOURCE_ID_RADIO_LAST ? "radio" : "unknown";
  }
}

bool startLearn(const char *name, uint8_t targetSourceId) {
  begin();
  if (targetSourceId != SOURCE_ID_UNKNOWN) {
    const int index = radioIndex(targetSourceId);
    if (index < 0 || (registry.entries[index].flags & FLAG_ASSIGNED) == 0) {
      learn.error = "unknown_source";
      return false;
    }
  } else if (firstUnassignedSource() == SOURCE_ID_UNKNOWN) {
    learn.error = "source_id_exhausted";
    return false;
  }

  learn = LearnState{};
  learn.active = true;
  learn.targetSourceId = targetSourceId;
  learn.startedMs = millis();
  learn.timeoutMs = 30000;
  copyName(learn.pendingName, name);
  learn.error = "none";
  SerialLog::infof("RF433", "Learn mode active | target=%u | timeout=%lus",
                   static_cast<unsigned int>(targetSourceId),
                   static_cast<unsigned long>(learn.timeoutMs / 1000U));
  return true;
}

void cancelLearn() {
  learn.active = false;
  learn.error = "cancelled";
}

const LearnState &learnState() { return learn; }

bool consumeFrame(uint32_t code,
                  uint8_t bitCount,
                  uint8_t pulseBucket,
                  uint8_t &sourceIdOut,
                  bool &learnedOut) {
  begin();
  sourceIdOut = SOURCE_ID_UNKNOWN;
  learnedOut = false;
  if (bitCount < 16 || bitCount > 32 || code == 0) return false;

  if (learn.active) {
    uint8_t target = learn.targetSourceId;
    if (target == SOURCE_ID_UNKNOWN) target = firstUnassignedSource();
    if (target == SOURCE_ID_UNKNOWN) {
      learn.error = "source_id_exhausted";
      learn.active = false;
      return false;
    }

    for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
      const StoredEntry &existing = registry.entries[i];
      const uint8_t existingId = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      if ((existing.flags & FLAG_BOUND) == 0 || existingId == target) continue;
      if (existing.code == code && existing.bitCount == bitCount && pulseBucketMatches(existing.pulseBucket, pulseBucket)) {
        learn.error = "already_bound";
        return false;
      }
    }

    const int index = radioIndex(target);
    StoredEntry before = registry.entries[index];
    StoredEntry &stored = registry.entries[index];
    stored.flags = FLAG_ASSIGNED | FLAG_BOUND;
    stored.bitCount = bitCount;
    stored.pulseBucket = pulseBucket;
    stored.code = code;
    if (learn.pendingName[0]) {
      copyName(stored.name, learn.pendingName);
    } else if (!stored.name[0]) {
      char generated[24];
      snprintf(generated, sizeof(generated), "Button %u", static_cast<unsigned int>(target - SOURCE_ID_RADIO_FIRST + 1U));
      copyName(stored.name, generated);
    }
    if (!save()) {
      registry.entries[index] = before;
      decodeEntries();
      learn.error = "persist_failed";
      return false;
    }

    learn.active = false;
    learn.error = "none";
    sourceIdOut = target;
    learnedOut = true;
    SerialLog::successf("RF433", "Button learned | source=%u | name=%s | bits=%u | code=0x%08lX",
                        static_cast<unsigned int>(target), sourceName(target), static_cast<unsigned int>(bitCount),
                        static_cast<unsigned long>(code));
    return true;
  }

  for (uint8_t i = 0; i < RADIO_SOURCE_CAPACITY; ++i) {
    const StoredEntry &stored = registry.entries[i];
    if ((stored.flags & FLAG_BOUND) == 0) continue;
    if (stored.code == code && stored.bitCount == bitCount && pulseBucketMatches(stored.pulseBucket, pulseBucket)) {
      sourceIdOut = static_cast<uint8_t>(SOURCE_ID_RADIO_FIRST + i);
      return true;
    }
  }
  return false;
}

bool renameSource(uint8_t sourceId, const char *name) {
  const int index = radioIndex(sourceId);
  if (index < 0 || (registry.entries[index].flags & FLAG_ASSIGNED) == 0 || !name || !name[0]) return false;
  StoredEntry before = registry.entries[index];
  copyName(registry.entries[index].name, name);
  if (save()) return true;
  registry.entries[index] = before;
  decodeEntries();
  return false;
}

bool unbindSource(uint8_t sourceId) {
  const int index = radioIndex(sourceId);
  if (index < 0 || (registry.entries[index].flags & FLAG_ASSIGNED) == 0) return false;
  StoredEntry before = registry.entries[index];
  registry.entries[index].flags &= static_cast<uint8_t>(~FLAG_BOUND);
  registry.entries[index].bitCount = 0;
  registry.entries[index].pulseBucket = 0;
  registry.entries[index].code = 0;
  if (save()) return true;
  registry.entries[index] = before;
  decodeEntries();
  return false;
}

}  // namespace SourceRegistry
