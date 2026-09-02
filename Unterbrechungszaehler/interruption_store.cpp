#include "interruption_store.h"

#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>

#include "project_config.h"
#include "project_time.h"
#include "serial_log.h"
#include "status_registry.h"

namespace InterruptionStore {
namespace {

constexpr uint32_t META_MAGIC = 0x52544E49UL; // "INTR" little-endian
constexpr uint16_t META_VERSION = 2;
constexpr size_t META_SIZE = 44;
constexpr size_t META_SLOTS = 2;
constexpr size_t RECORD_SIZE = ProjectConfig::RAW_RECORD_SIZE;

struct Meta {
  uint32_t writeIndex = 0;
  uint32_t count = 0;
  uint64_t totalSequence = 0;
  // Persist the latest event that had a valid local calendar. This makes the
  // next same-day interval O(1) after reboot even if the newest raw record was
  // recorded with relative/no absolute time.
  uint32_t lastCalendarEpochSeconds = 0;
  uint16_t lastCalendarDayIndex = 0;
  bool lastCalendarValid = false;
  uint32_t commit = 0;
};

Info currentInfo;
Meta meta;
File rawFile;
File metaFile;
bool mounted = false;
bool readyFlag = false;
const char *errorText = "none";
uint32_t lastFsUsageRefreshMs = 0;

struct RecoveryState {
  bool active = false;
  uint32_t slots = 0;
  uint32_t index = 0;
  uint32_t validCount = 0;
  uint32_t contiguousCount = 0;
  uint32_t transitionCount = 0;
  uint32_t transitionIndex = 0;
  bool allValid = true;
  bool havePrevious = false;
  uint8_t firstTag = 0;
  uint8_t previousTag = 0;
  uint8_t newestTag = 0;
  bool anchorSearch = false;
  uint32_t anchorOffset = 0;
} recovery;

void put16(uint8_t *p, uint16_t v) { p[0] = static_cast<uint8_t>(v); p[1] = static_cast<uint8_t>(v >> 8); }
void put32(uint8_t *p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v); p[1] = static_cast<uint8_t>(v >> 8);
  p[2] = static_cast<uint8_t>(v >> 16); p[3] = static_cast<uint8_t>(v >> 24);
}
void put64(uint8_t *p, uint64_t v) { for (uint8_t i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(v >> (8U * i)); }
uint16_t get16(const uint8_t *p) { return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8); }
uint32_t get32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
uint64_t get64(const uint8_t *p) {
  uint64_t value = 0;
  for (uint8_t i = 0; i < 8; ++i) value |= static_cast<uint64_t>(p[i]) << (8U * i);
  return value;
}

uint8_t crc8(const uint8_t *data, size_t length) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) crc = (crc & 0x80U) ? static_cast<uint8_t>((crc << 1) ^ 0x07U) : static_cast<uint8_t>(crc << 1);
  }
  return crc;
}

uint32_t crc32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0UL);
  }
  return ~crc;
}

void updateInfo() {
  currentInfo.mounted = mounted;
  currentInfo.ready = readyFlag;
  currentInfo.recovering = recovery.active;
  currentInfo.count = meta.count;
  currentInfo.capacity = ProjectConfig::RAW_EVENT_CAPACITY;
  currentInfo.totalSequence = meta.totalSequence;
  currentInfo.error = errorText;
}

void refreshFsUsage(bool force) {
  if (!mounted) {
    currentInfo.fsTotalBytes = 0;
    currentInfo.fsUsedBytes = 0;
    return;
  }
  const uint32_t nowMs = millis();
  if (!force && static_cast<uint32_t>(nowMs - lastFsUsageRefreshMs) < 5000U) return;
  lastFsUsageRefreshMs = nowMs;
  currentInfo.fsTotalBytes = LittleFS.totalBytes();
  currentInfo.fsUsedBytes = LittleFS.usedBytes();
}

void setStatus(StatusRegistry::State state, const char *error = "none") {
  errorText = error ? error : "none";
  StatusRegistry::setState("data", state);
  updateInfo();
}

void encodeRecord(const InterruptionTypes::CapturedEvent &event, uint64_t sequence, uint8_t out[RECORD_SIZE]) {
  put32(out, event.timeValueSeconds);
  uint32_t packed = event.deltaSeconds & 0x1FFFFUL;
  packed |= (static_cast<uint32_t>(event.timeSource) & 0x07UL) << 17;
  packed |= (static_cast<uint32_t>(event.eventSource) & 0x07UL) << 20;
  if (event.absoluteValid) packed |= 1UL << 23;
  out[4] = static_cast<uint8_t>(packed);
  out[5] = static_cast<uint8_t>(packed >> 8);
  out[6] = static_cast<uint8_t>(packed >> 16);
  out[7] = static_cast<uint8_t>(sequence & 0xFFU);
  out[8] = crc8(out, 8);
}

bool decodeRecord(const uint8_t in[RECORD_SIZE], InterruptionTypes::RawEvent &out) {
  if (crc8(in, 8) != in[8]) return false;
  const uint32_t packed = static_cast<uint32_t>(in[4]) | (static_cast<uint32_t>(in[5]) << 8) | (static_cast<uint32_t>(in[6]) << 16);
  out.timeValueSeconds = get32(in);
  out.deltaSeconds = packed & 0x1FFFFUL;
  out.timeSource = static_cast<TimeTypes::Source>((packed >> 17) & 0x07UL);
  out.eventSource = static_cast<InterruptionTypes::EventSource>((packed >> 20) & 0x07UL);
  out.absoluteValid = (packed & (1UL << 23)) != 0;
  out.sequenceTag = in[7];
  if (static_cast<uint8_t>(out.timeSource) > static_cast<uint8_t>(TimeTypes::Source::Relative)) return false;
  if (static_cast<uint8_t>(out.eventSource) > static_cast<uint8_t>(InterruptionTypes::EventSource::Hardware)) return false;
  return true;
}

bool readRecordAt(uint32_t index, InterruptionTypes::RawEvent &out) {
  if (!rawFile || index >= ProjectConfig::RAW_EVENT_CAPACITY) return false;
  const size_t offset = static_cast<size_t>(index) * RECORD_SIZE;
  if (rawFile.size() < offset + RECORD_SIZE || !rawFile.seek(offset, SeekSet)) return false;
  uint8_t bytes[RECORD_SIZE];
  if (rawFile.read(bytes, RECORD_SIZE) != RECORD_SIZE) return false;
  return decodeRecord(bytes, out);
}

void encodeMeta(const Meta &value, uint8_t out[META_SIZE]) {
  memset(out, 0, META_SIZE);
  put32(&out[0], META_MAGIC);
  put16(&out[4], META_VERSION);
  put16(&out[6], static_cast<uint16_t>(RECORD_SIZE));
  put32(&out[8], ProjectConfig::RAW_EVENT_CAPACITY);
  put32(&out[12], value.writeIndex);
  put32(&out[16], value.count);
  put64(&out[20], value.totalSequence);
  put32(&out[28], value.lastCalendarEpochSeconds);
  put16(&out[32], value.lastCalendarDayIndex);
  put16(&out[34], value.lastCalendarValid ? 1U : 0U);
  put32(&out[36], value.commit);
  put32(&out[40], crc32(out, 40));
}

bool decodeMeta(const uint8_t in[META_SIZE], Meta &value) {
  if (get32(&in[0]) != META_MAGIC || get16(&in[4]) != META_VERSION || get16(&in[6]) != RECORD_SIZE ||
      get32(&in[8]) != ProjectConfig::RAW_EVENT_CAPACITY || get32(&in[40]) != crc32(in, 40)) return false;
  value.writeIndex = get32(&in[12]);
  value.count = get32(&in[16]);
  value.totalSequence = get64(&in[20]);
  value.lastCalendarEpochSeconds = get32(&in[28]);
  value.lastCalendarDayIndex = get16(&in[32]);
  value.lastCalendarValid = (get16(&in[34]) & 1U) != 0U;
  value.commit = get32(&in[36]);
  return value.writeIndex < ProjectConfig::RAW_EVENT_CAPACITY && value.count <= ProjectConfig::RAW_EVENT_CAPACITY && value.totalSequence >= value.count;
}

bool readMetaSlot(uint8_t slot, Meta &value) {
  if (!metaFile || slot >= META_SLOTS) return false;
  const size_t offset = static_cast<size_t>(slot) * META_SIZE;
  if (metaFile.size() < offset + META_SIZE || !metaFile.seek(offset, SeekSet)) return false;
  uint8_t bytes[META_SIZE];
  if (metaFile.read(bytes, META_SIZE) != META_SIZE) return false;
  return decodeMeta(bytes, value);
}

bool metaReferencesValidRecord(const Meta &candidate) {
  if (candidate.count == 0) return true;
  const uint32_t lastIndex = (candidate.writeIndex + ProjectConfig::RAW_EVENT_CAPACITY - 1U) % ProjectConfig::RAW_EVENT_CAPACITY;
  InterruptionTypes::RawEvent record;
  return readRecordAt(lastIndex, record) && record.sequenceTag == static_cast<uint8_t>(candidate.totalSequence & 0xFFU);
}

bool commitMeta() {
  ++meta.commit;
  if (meta.commit == 0) ++meta.commit;
  const uint8_t slot = static_cast<uint8_t>(meta.commit & 1U);
  uint8_t bytes[META_SIZE];
  encodeMeta(meta, bytes);
  const size_t offset = static_cast<size_t>(slot) * META_SIZE;
  if (!metaFile.seek(offset, SeekSet) || metaFile.write(bytes, META_SIZE) != META_SIZE) return false;
  metaFile.flush();
  updateInfo();
  return true;
}

bool openFiles() {
  rawFile = LittleFS.open(ProjectConfig::RAW_DATA_PATH, "r+");
  if (!rawFile) rawFile = LittleFS.open(ProjectConfig::RAW_DATA_PATH, "w+");
  metaFile = LittleFS.open(ProjectConfig::RAW_META_PATH, "r+");
  if (!metaFile) metaFile = LittleFS.open(ProjectConfig::RAW_META_PATH, "w+");
  return static_cast<bool>(rawFile) && static_cast<bool>(metaFile);
}

bool mountFilesystem() {
  Preferences prefs;
  if (LittleFS.begin(false, "/littlefs", 8, "littlefs")) {
    // Mark a successfully mounted data partition as initialized as well. This
    // covers a pre-created filesystem and avoids a future transient mount
    // failure being mistaken for a never-used partition. Write only once.
    if (prefs.begin(ProjectConfig::FS_PREF_NAMESPACE, false)) {
      if (!prefs.getBool("initialized", false)) prefs.putBool("initialized", true);
      prefs.end();
    }
    return true;
  }

  bool initializedBefore = false;
  if (prefs.begin(ProjectConfig::FS_PREF_NAMESPACE, false)) {
    initializedBefore = prefs.getBool("initialized", false);
    prefs.end();
  }
  if (initializedBefore) {
    SerialLog::error("STORE", "LittleFS mount failed; automatic format suppressed because project data may exist");
    return false;
  }

  SerialLog::warning("STORE", "LittleFS appears uninitialized; formatting project data partition once");
  // begin(true) formats only after a failed mount and then retries the mount.
  if (!LittleFS.begin(true, "/littlefs", 8, "littlefs")) return false;
  if (prefs.begin(ProjectConfig::FS_PREF_NAMESPACE, false)) {
    prefs.putBool("initialized", true);
    prefs.end();
  }
  return true;
}

bool loadBestMeta() {
  Meta a, b;
  const bool aValid = readMetaSlot(0, a) && metaReferencesValidRecord(a);
  const bool bValid = readMetaSlot(1, b) && metaReferencesValidRecord(b);
  if (!aValid && !bValid) return false;
  if (aValid && bValid) meta = static_cast<int32_t>(a.commit - b.commit) > 0 ? a : b;
  else meta = aValid ? a : b;
  return true;
}

void updateCalendarAnchorFromRaw(const InterruptionTypes::RawEvent &record) {
  if (!record.absoluteValid) return;
  ProjectTime::LocalDateTime local;
  if (!ProjectTime::fromEpochSeconds(record.timeValueSeconds, local)) return;
  meta.lastCalendarValid = true;
  meta.lastCalendarDayIndex = local.dayIndex;
  meta.lastCalendarEpochSeconds = record.timeValueSeconds;
}

void completeRecovery() {
  recovery.active = false;
  recovery.anchorSearch = false;
  readyFlag = commitMeta();
  setStatus(readyFlag ? StatusRegistry::State::Warning : StatusRegistry::State::Error,
            readyFlag ? "metadata recovered; sequences may be rebased" : "metadata recovery commit failed");
  if (readyFlag) {
    SerialLog::warningf("STORE", "Raw ring metadata recovered | events=%lu | sequence=%llu",
                        static_cast<unsigned long>(meta.count), static_cast<unsigned long long>(meta.totalSequence));
  }
}

void startCalendarAnchorRecovery() {
  meta.lastCalendarValid = false;
  meta.lastCalendarEpochSeconds = 0;
  meta.lastCalendarDayIndex = 0;
  recovery.anchorOffset = 0;
  recovery.anchorSearch = meta.count > 0;
  if (!recovery.anchorSearch) completeRecovery();
}

void calendarAnchorRecoveryStep() {
  if (!recovery.active || !recovery.anchorSearch) return;
  constexpr uint32_t BATCH = 128;
  const uint32_t remaining = meta.count - recovery.anchorOffset;
  const uint32_t work = remaining < BATCH ? remaining : BATCH;
  for (uint32_t i = 0; i < work; ++i, ++recovery.anchorOffset) {
    const uint32_t index = (meta.writeIndex + ProjectConfig::RAW_EVENT_CAPACITY - 1U - recovery.anchorOffset) %
                           ProjectConfig::RAW_EVENT_CAPACITY;
    InterruptionTypes::RawEvent record;
    if (!readRecordAt(index, record) || !record.absoluteValid) continue;
    updateCalendarAnchorFromRaw(record);
    if (meta.lastCalendarValid) {
      completeRecovery();
      return;
    }
  }
  if (recovery.anchorOffset >= meta.count) completeRecovery();
}

void recoverOrphan() {
  InterruptionTypes::RawEvent record;
  if (!readRecordAt(meta.writeIndex, record)) return;
  const uint64_t nextSequence = meta.totalSequence + 1ULL;
  if (record.sequenceTag != static_cast<uint8_t>(nextSequence & 0xFFU)) return;

  const Meta before = meta;
  meta.totalSequence = nextSequence;
  meta.writeIndex = (meta.writeIndex + 1U) % ProjectConfig::RAW_EVENT_CAPACITY;
  if (meta.count < ProjectConfig::RAW_EVENT_CAPACITY) ++meta.count;
  updateCalendarAnchorFromRaw(record);
  if (commitMeta()) {
    SerialLog::warning("STORE", "Recovered one raw event written before metadata commit");
  } else {
    // Keep the last durable metadata authoritative. The orphan remains in its
    // slot and can safely be overwritten/recovered on a later attempt.
    meta = before;
    updateInfo();
    SerialLog::warning("STORE", "Orphan raw event found, but metadata recovery commit failed; durable metadata kept");
  }
}

void startRecovery() {
  recovery = RecoveryState{};
  recovery.active = true;
  recovery.slots = static_cast<uint32_t>(rawFile.size() / RECORD_SIZE);
  if (recovery.slots > ProjectConfig::RAW_EVENT_CAPACITY) recovery.slots = ProjectConfig::RAW_EVENT_CAPACITY;
  readyFlag = false;
  setStatus(StatusRegistry::State::Warning, "metadata recovery");
  SerialLog::warningf("STORE", "Raw metadata unavailable; cooperative recovery scan started | slots=%lu",
                      static_cast<unsigned long>(recovery.slots));
}

uint64_t sequenceEndingWith(uint32_t minimum, uint8_t tag) {
  uint64_t value = minimum;
  const uint8_t currentTag = static_cast<uint8_t>(value & 0xFFU);
  value += static_cast<uint8_t>(tag - currentTag);
  if (value < minimum) value += 256ULL;
  return value;
}

void finishRecovery() {
  if (recovery.slots == 0) {
    meta = Meta{};
    recovery.active = false;
    readyFlag = commitMeta();
    setStatus(readyFlag ? StatusRegistry::State::Ok : StatusRegistry::State::Error,
              readyFlag ? "none" : "metadata write failed");
    return;
  }

  if (!recovery.allValid) {
    recovery.active = false;
    readyFlag = false;
    setStatus(StatusRegistry::State::Error, "raw recovery found invalid records");
    SerialLog::error("STORE", "Recovery stopped: raw file contains invalid records; data files were preserved");
    return;
  }

  meta = Meta{};
  if (recovery.slots < ProjectConfig::RAW_EVENT_CAPACITY) {
    meta.count = recovery.contiguousCount;
    meta.writeIndex = meta.count % ProjectConfig::RAW_EVENT_CAPACITY;
    if (meta.count > 0) meta.totalSequence = sequenceEndingWith(meta.count, recovery.newestTag);
  } else {
    // A full ring must have exactly one physical discontinuity when crossing
    // from newest back to oldest. Capacity 100000 is deliberately not a
    // multiple of 256, so the 8-bit sequence tag still identifies that edge.
    const uint8_t wrapExpected = static_cast<uint8_t>(recovery.previousTag + 1U);
    if (recovery.firstTag != wrapExpected) {
      ++recovery.transitionCount;
      recovery.transitionIndex = 0;
    }
    if (recovery.transitionCount != 1) {
      recovery.active = false;
      setStatus(StatusRegistry::State::Error, "raw recovery sequence ambiguous");
      SerialLog::errorf("STORE", "Recovery stopped: expected one ring transition, found %lu",
                        static_cast<unsigned long>(recovery.transitionCount));
      return;
    }
    meta.count = ProjectConfig::RAW_EVENT_CAPACITY;
    meta.writeIndex = recovery.transitionIndex;
    const uint32_t newestIndex = (meta.writeIndex + meta.count - 1U) % ProjectConfig::RAW_EVENT_CAPACITY;
    InterruptionTypes::RawEvent newest;
    if (!readRecordAt(newestIndex, newest)) {
      recovery.active = false;
      setStatus(StatusRegistry::State::Error, "raw recovery newest record unreadable");
      return;
    }
    meta.totalSequence = sequenceEndingWith(meta.count, newest.sequenceTag);
  }

  // Recover the latest calendar-valid anchor in a second cooperative phase.
  // This can require walking back through the ring if the newest records were
  // relative-only, so it is deliberately not done in one blocking loop.
  startCalendarAnchorRecovery();
}

void recoveryStep() {
  if (!recovery.active) return;
  if (recovery.anchorSearch) {
    calendarAnchorRecoveryStep();
    return;
  }
  constexpr uint32_t BATCH = 128;
  const uint32_t batchEnd = recovery.index + BATCH;
  const uint32_t end = recovery.slots < batchEnd ? recovery.slots : batchEnd;
  for (; recovery.index < end; ++recovery.index) {
    InterruptionTypes::RawEvent event;
    if (!readRecordAt(recovery.index, event)) {
      recovery.allValid = false;
      continue;
    }
    ++recovery.validCount;
    if (recovery.index == recovery.contiguousCount) ++recovery.contiguousCount;
    if (!recovery.havePrevious) {
      recovery.havePrevious = true;
      recovery.firstTag = event.sequenceTag;
    } else if (event.sequenceTag != static_cast<uint8_t>(recovery.previousTag + 1U)) {
      ++recovery.transitionCount;
      recovery.transitionIndex = recovery.index;
    }
    recovery.previousTag = event.sequenceTag;
    recovery.newestTag = event.sequenceTag;
  }
  if (recovery.index >= recovery.slots) finishRecovery();
}

}  // namespace

bool begin() {
  StatusRegistry::registerProvider("data", "status.data", "memory", true);
  setStatus(StatusRegistry::State::Checking, "initializing");
  mounted = mountFilesystem();
  if (!mounted) {
    readyFlag = false;
    setStatus(StatusRegistry::State::Error, "LittleFS mount failed");
    return false;
  }
  if (!openFiles()) {
    readyFlag = false;
    setStatus(StatusRegistry::State::Error, "project data files unavailable");
    return false;
  }

  if (!loadBestMeta()) {
    if (rawFile.size() == 0) {
      meta = Meta{};
      readyFlag = commitMeta();
      setStatus(readyFlag ? StatusRegistry::State::Ok : StatusRegistry::State::Error,
                readyFlag ? "none" : "metadata initialization failed");
    } else {
      startRecovery();
    }
  } else {
    recoverOrphan();
    readyFlag = true;
    setStatus(StatusRegistry::State::Ok);
  }

  updateInfo();
  refreshFsUsage(true);
  SerialLog::infof("STORE", "LittleFS | total=%u | used=%u | raw=%lu/%lu",
                   static_cast<unsigned int>(currentInfo.fsTotalBytes), static_cast<unsigned int>(currentInfo.fsUsedBytes),
                   static_cast<unsigned long>(meta.count), static_cast<unsigned long>(ProjectConfig::RAW_EVENT_CAPACITY));
  return readyFlag || recovery.active;
}

void update() {
  if (recovery.active) recoveryStep();
  updateInfo();
}

bool ready() { return readyFlag && !recovery.active; }
bool recovering() { return recovery.active; }
const Info &info() { updateInfo(); refreshFsUsage(false); return currentInfo; }

bool append(const InterruptionTypes::CapturedEvent &event, uint64_t &sequenceOut) {
  if (!ready()) return false;
  const Meta before = meta;
  const uint64_t sequence = before.totalSequence + 1ULL;
  uint8_t bytes[RECORD_SIZE];
  uint8_t displaced[RECORD_SIZE];
  bool displacedValid = false;
  const size_t offset = static_cast<size_t>(before.writeIndex) * RECORD_SIZE;

  // Once the raw ring is full the next slot is still part of the last durable
  // metadata state (the oldest event). Keep its 9 bytes so an in-process meta
  // commit failure can roll the physical record back as well. A sudden power
  // loss before that rollback is handled by recoverOrphan() on the next boot.
  if (before.count >= ProjectConfig::RAW_EVENT_CAPACITY) {
    if (!rawFile.seek(offset, SeekSet) || rawFile.read(displaced, RECORD_SIZE) != RECORD_SIZE) {
      setStatus(StatusRegistry::State::Error, "raw overwrite backup failed");
      return false;
    }
    displacedValid = true;
  }

  encodeRecord(event, sequence, bytes);
  if (!rawFile.seek(offset, SeekSet) || rawFile.write(bytes, RECORD_SIZE) != RECORD_SIZE) {
    setStatus(StatusRegistry::State::Error, "raw event write failed");
    return false;
  }
  rawFile.flush();

  meta.totalSequence = sequence;
  meta.writeIndex = (before.writeIndex + 1U) % ProjectConfig::RAW_EVENT_CAPACITY;
  meta.count = before.count < ProjectConfig::RAW_EVENT_CAPACITY ? before.count + 1U : before.count;
  if (event.localCalendarValid) {
    meta.lastCalendarValid = true;
    meta.lastCalendarDayIndex = event.localDayIndex;
    meta.lastCalendarEpochSeconds = event.timeValueSeconds;
  }
  if (!commitMeta()) {
    // Do not advance the in-RAM ring state when the durable metadata commit
    // fails. If this write displaced the oldest event in a full ring, restore
    // that 9-byte record too; otherwise the rolled-back metadata would point at
    // a slot that already contains the new sequence.
    bool restored = true;
    if (displacedValid) {
      restored = rawFile.seek(offset, SeekSet) &&
                 rawFile.write(displaced, RECORD_SIZE) == RECORD_SIZE;
      if (restored) rawFile.flush();
    }
    meta = before;
    updateInfo();
    setStatus(StatusRegistry::State::Error,
              restored ? "raw metadata commit failed" : "raw transaction rollback failed");
    return false;
  }
  sequenceOut = sequence;
  refreshFsUsage(false);
  setStatus(StatusRegistry::State::Ok);
  return true;
}

uint64_t oldestSequence() {
  return meta.count == 0 ? 0 : meta.totalSequence - static_cast<uint64_t>(meta.count) + 1ULL;
}
uint64_t newestSequence() { return meta.count == 0 ? 0 : meta.totalSequence; }
uint32_t count() { return meta.count; }
uint32_t capacity() { return ProjectConfig::RAW_EVENT_CAPACITY; }

bool lastCalendarAnchor(uint16_t &dayIndexOut, uint32_t &epochSecondsOut) {
  if (!meta.lastCalendarValid) return false;
  ProjectTime::LocalDateTime local;
  if (!ProjectTime::fromEpochSeconds(meta.lastCalendarEpochSeconds, local) ||
      local.dayIndex != meta.lastCalendarDayIndex) return false;
  dayIndexOut = meta.lastCalendarDayIndex;
  epochSecondsOut = meta.lastCalendarEpochSeconds;
  return true;
}

bool alignSequenceAtLeast(uint64_t durableHint) {
  if (!ready() || durableHint <= meta.totalSequence) return true;

  uint64_t aligned = durableHint;
  if (meta.count > 0) {
    // Recovery can reconstruct the low 8 sequence bits from the newest record.
    // Lift the recovered sequence to the first value at/after the durable hint
    // with the same tag. This changes only logical numbering; physical order and
    // raw event contents remain untouched.
    const uint8_t newestTag = static_cast<uint8_t>(meta.totalSequence & 0xFFU);
    const uint8_t hintTag = static_cast<uint8_t>(durableHint & 0xFFU);
    const uint8_t forward = static_cast<uint8_t>(newestTag - hintTag);
    aligned = durableHint + static_cast<uint64_t>(forward);
    if (aligned < meta.totalSequence) return false;
  }

  // An empty raw ring can coexist with a still-valid long-term daily store only
  // after exceptional recovery/data loss. Preserve the aggregate checkpoint as
  // the next raw sequence base so future events are not mistaken for ancient
  // already-aggregated sequence numbers. Count remains zero.
  const Meta before = meta;
  meta.totalSequence = aligned;
  if (!commitMeta()) {
    meta = before;
    updateInfo();
    setStatus(StatusRegistry::State::Error, "raw sequence alignment commit failed");
    return false;
  }

  SerialLog::warningf(
      "STORE", "Recovered raw sequence aligned to aggregate checkpoint | hint=%llu | newest=%llu",
      static_cast<unsigned long long>(durableHint), static_cast<unsigned long long>(meta.totalSequence));
  return true;
}

bool readSequence(uint64_t sequence, InterruptionTypes::RawEvent &eventOut) {
  if (!ready() || meta.count == 0) return false;
  const uint64_t oldest = oldestSequence();
  if (sequence < oldest || sequence > meta.totalSequence) return false;
  const uint32_t oldestIndex = (meta.writeIndex + ProjectConfig::RAW_EVENT_CAPACITY - meta.count) % ProjectConfig::RAW_EVENT_CAPACITY;
  const uint32_t offset = static_cast<uint32_t>(sequence - oldest);
  const uint32_t index = (oldestIndex + offset) % ProjectConfig::RAW_EVENT_CAPACITY;
  if (!readRecordAt(index, eventOut)) return false;
  return eventOut.sequenceTag == static_cast<uint8_t>(sequence & 0xFFU);
}

bool readLast(InterruptionTypes::RawEvent &eventOut, uint64_t &sequenceOut) {
  if (meta.count == 0) return false;
  sequenceOut = meta.totalSequence;
  return readSequence(sequenceOut, eventOut);
}

}  // namespace InterruptionStore
