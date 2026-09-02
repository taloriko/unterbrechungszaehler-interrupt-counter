#include "interruption_aggregates.h"

#include <FS.h>
#include <LittleFS.h>
#include <cstring>

#include "interruption_store.h"
#include "project_config.h"
#include "project_time.h"
#include "serial_log.h"

namespace InterruptionAggregates {
namespace {

constexpr uint32_t META_MAGIC = 0x47474149UL;  // "IAGG"
constexpr uint16_t META_VERSION = 1;
constexpr size_t META_SIZE = 40;
constexpr size_t META_SLOTS = 2;
constexpr size_t RECORD_SIZE = ProjectConfig::DAILY_RECORD_SIZE;
constexpr uint8_t RECORD_FLAG_VALID = 0x01;

struct Meta {
  uint32_t writeIndex = 0;
  uint32_t count = 0;
  uint64_t lastProcessedSequence = 0;
  uint32_t unassignedCount = 0;
  uint32_t commit = 0;
};

Info currentInfo;
Meta meta;
File dataFile;
File metaFile;
bool readyFlag = false;
const char *errorText = "none";
uint16_t cachedDay = 0;
uint32_t cachedSlot = 0;
bool cacheValid = false;

bool rebuildActive = false;
uint64_t rebuildSequence = 0;
bool locateReadError = false;

void startRebuildFromRaw(const char *reason);

void put16(uint8_t *p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8);
}

void put32(uint8_t *p, uint32_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8);
  p[2] = static_cast<uint8_t>(value >> 16);
  p[3] = static_cast<uint8_t>(value >> 24);
}

void put64(uint8_t *p, uint64_t value) {
  for (uint8_t i = 0; i < 8; ++i) {
    p[i] = static_cast<uint8_t>(value >> (8U * i));
  }
}

uint16_t get16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8);
}

uint32_t get32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

uint64_t get64(const uint8_t *p) {
  uint64_t value = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(p[i]) << (8U * i);
  }
  return value;
}

uint16_t crc16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFFU;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U)
                ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

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

void updateInfo() {
  currentInfo.ready = readyFlag;
  currentInfo.rebuilding = rebuildActive;
  currentInfo.dayCount = static_cast<uint16_t>(meta.count);
  currentInfo.capacity = ProjectConfig::DAILY_AGGREGATE_CAPACITY;
  currentInfo.lastProcessedSequence = meta.lastProcessedSequence;
  currentInfo.unassignedCount = meta.unassignedCount;
  currentInfo.error = errorText;
}

void setState(const char *error = "none") {
  errorText = error ? error : "none";
  updateInfo();
}

void encodeRecord(const DailyRecord &record, uint8_t out[RECORD_SIZE]) {
  memset(out, 0, RECORD_SIZE);
  put16(&out[0], record.dayIndex);
  out[2] = ProjectConfig::DAILY_FORMAT_VERSION;
  out[3] = RECORD_FLAG_VALID;
  put16(&out[4], record.total);
  put16(&out[6], 0);
  put64(&out[8], record.lastSequence);
  for (uint8_t hour = 0; hour < 24; ++hour) {
    put16(&out[16 + static_cast<size_t>(hour) * 2U], record.hours[hour]);
  }
  put16(&out[6], crc16(out, RECORD_SIZE));
}

bool decodeRecord(uint8_t bytes[RECORD_SIZE], DailyRecord &record) {
  const uint16_t storedCrc = get16(&bytes[6]);
  put16(&bytes[6], 0);
  const uint16_t calculated = crc16(bytes, RECORD_SIZE);
  put16(&bytes[6], storedCrc);

  if (storedCrc != calculated ||
      bytes[2] != ProjectConfig::DAILY_FORMAT_VERSION ||
      (bytes[3] & RECORD_FLAG_VALID) == 0) {
    return false;
  }

  record.dayIndex = get16(&bytes[0]);
  record.total = get16(&bytes[4]);
  record.lastSequence = get64(&bytes[8]);
  for (uint8_t hour = 0; hour < 24; ++hour) {
    record.hours[hour] = get16(&bytes[16 + static_cast<size_t>(hour) * 2U]);
  }
  return true;
}

bool readSlot(uint32_t slot, DailyRecord &record) {
  if (!dataFile || slot >= ProjectConfig::DAILY_AGGREGATE_CAPACITY) return false;
  const size_t offset = static_cast<size_t>(slot) * RECORD_SIZE;
  if (dataFile.size() < offset + RECORD_SIZE || !dataFile.seek(offset, SeekSet)) return false;

  uint8_t bytes[RECORD_SIZE];
  if (dataFile.read(bytes, RECORD_SIZE) != RECORD_SIZE) return false;
  return decodeRecord(bytes, record);
}

bool writeSlot(uint32_t slot, const DailyRecord &record) {
  uint8_t bytes[RECORD_SIZE];
  encodeRecord(record, bytes);
  const size_t offset = static_cast<size_t>(slot) * RECORD_SIZE;
  if (!dataFile.seek(offset, SeekSet) || dataFile.write(bytes, RECORD_SIZE) != RECORD_SIZE) {
    return false;
  }
  dataFile.flush();
  return true;
}

void encodeMeta(const Meta &value, uint8_t out[META_SIZE]) {
  memset(out, 0, META_SIZE);
  put32(&out[0], META_MAGIC);
  put16(&out[4], META_VERSION);
  put16(&out[6], static_cast<uint16_t>(RECORD_SIZE));
  put32(&out[8], ProjectConfig::DAILY_AGGREGATE_CAPACITY);
  put32(&out[12], value.writeIndex);
  put32(&out[16], value.count);
  put64(&out[20], value.lastProcessedSequence);
  put32(&out[28], value.unassignedCount);
  put32(&out[32], value.commit);
  put32(&out[36], crc32(out, 36));
}

bool decodeMeta(const uint8_t in[META_SIZE], Meta &value) {
  if (get32(&in[0]) != META_MAGIC ||
      get16(&in[4]) != META_VERSION ||
      get16(&in[6]) != RECORD_SIZE ||
      get32(&in[8]) != ProjectConfig::DAILY_AGGREGATE_CAPACITY ||
      get32(&in[36]) != crc32(in, 36)) {
    return false;
  }

  value.writeIndex = get32(&in[12]);
  value.count = get32(&in[16]);
  value.lastProcessedSequence = get64(&in[20]);
  value.unassignedCount = get32(&in[28]);
  value.commit = get32(&in[32]);
  return value.writeIndex < ProjectConfig::DAILY_AGGREGATE_CAPACITY &&
         value.count <= ProjectConfig::DAILY_AGGREGATE_CAPACITY;
}

bool readMetaSlot(uint8_t slot, Meta &value) {
  if (!metaFile || slot >= META_SLOTS) return false;
  const size_t offset = static_cast<size_t>(slot) * META_SIZE;
  if (metaFile.size() < offset + META_SIZE || !metaFile.seek(offset, SeekSet)) return false;

  uint8_t bytes[META_SIZE];
  if (metaFile.read(bytes, META_SIZE) != META_SIZE) return false;
  return decodeMeta(bytes, value);
}

bool commitMeta() {
  ++meta.commit;
  if (meta.commit == 0) ++meta.commit;

  uint8_t bytes[META_SIZE];
  encodeMeta(meta, bytes);
  const size_t offset = static_cast<size_t>(meta.commit & 1U) * META_SIZE;
  if (!metaFile.seek(offset, SeekSet) || metaFile.write(bytes, META_SIZE) != META_SIZE) {
    return false;
  }
  metaFile.flush();
  updateInfo();
  return true;
}

bool loadMeta() {
  Meta a;
  Meta b;
  const bool aValid = readMetaSlot(0, a);
  const bool bValid = readMetaSlot(1, b);
  if (!aValid && !bValid) return false;

  if (aValid && bValid) {
    meta = static_cast<int32_t>(a.commit - b.commit) > 0 ? a : b;
  } else {
    meta = aValid ? a : b;
  }
  return true;
}

bool openFiles() {
  dataFile = LittleFS.open(ProjectConfig::DAILY_DATA_PATH, "r+");
  if (!dataFile) dataFile = LittleFS.open(ProjectConfig::DAILY_DATA_PATH, "w+");
  metaFile = LittleFS.open(ProjectConfig::DAILY_META_PATH, "r+");
  if (!metaFile) metaFile = LittleFS.open(ProjectConfig::DAILY_META_PATH, "w+");
  return static_cast<bool>(dataFile) && static_cast<bool>(metaFile);
}

uint32_t oldestSlot() {
  if (meta.count == 0) return 0;
  return (meta.writeIndex + ProjectConfig::DAILY_AGGREGATE_CAPACITY - meta.count) %
         ProjectConfig::DAILY_AGGREGATE_CAPACITY;
}

bool locate(uint16_t dayIndex, DailyRecord &record, uint32_t &slot) {
  locateReadError = false;
  if (cacheValid && cachedDay == dayIndex) {
    if (readSlot(cachedSlot, record)) {
      slot = cachedSlot;
      return true;
    }
    locateReadError = true;
    return false;
  }

  const uint32_t start = oldestSlot();
  for (uint32_t i = 0; i < meta.count; ++i) {
    const uint32_t candidate = (start + i) % ProjectConfig::DAILY_AGGREGATE_CAPACITY;
    DailyRecord candidateRecord;
    if (!readSlot(candidate, candidateRecord)) {
      locateReadError = true;
      return false;
    }
    if (candidateRecord.dayIndex == dayIndex) {
      record = candidateRecord;
      slot = candidate;
      cachedDay = dayIndex;
      cachedSlot = slot;
      cacheValid = true;
      return true;
    }
  }
  return false;
}

bool applyDecoded(const InterruptionTypes::RawEvent &raw, uint64_t sequence) {
  InterruptionTypes::CapturedEvent event;
  event.absoluteValid = raw.absoluteValid;
  if (raw.absoluteValid) {
    ProjectTime::LocalDateTime local;
    if (ProjectTime::fromEpochSeconds(raw.timeValueSeconds, local)) {
      event.localCalendarValid = true;
      event.localDayIndex = local.dayIndex;
      event.localHour = local.hour;
    }
  }

  // apply() owns the transactional metadata update and treats a missing local
  // calendar as an unassigned event. Rebuild and live writes share one path.
  return apply(event, sequence);
}

void startRebuildFromRaw(const char *reason) {
  SerialLog::warningf("STATS", "Daily aggregates rebuilt from retained raw ring | reason=%s", reason);
  dataFile.close();
  metaFile.close();
  LittleFS.remove(ProjectConfig::DAILY_DATA_PATH);
  LittleFS.remove(ProjectConfig::DAILY_META_PATH);

  meta = Meta{};
  cacheValid = false;
  if (!openFiles()) {
    readyFlag = false;
    rebuildActive = false;
    setState("aggregate files unavailable after reset");
    return;
  }

  readyFlag = commitMeta();
  if (!readyFlag) {
    rebuildActive = false;
    setState("aggregate metadata initialization failed");
    return;
  }

  rebuildSequence = InterruptionStore::oldestSequence();
  rebuildActive = rebuildSequence != 0;
  setState(rebuildActive ? "rebuilding from raw ring" : "none");
}

void reconcileStep() {
  if (!rebuildActive) return;
  constexpr uint8_t BATCH = 4;

  for (uint8_t i = 0; i < BATCH && rebuildActive; ++i) {
    const uint64_t newest = InterruptionStore::newestSequence();
    if (rebuildSequence == 0 || rebuildSequence > newest) {
      rebuildActive = false;
      readyFlag = true;
      setState("none");
      SerialLog::success("STATS", "Aggregate reconciliation complete");
      break;
    }

    InterruptionTypes::RawEvent raw;
    if (!InterruptionStore::readSequence(rebuildSequence, raw)) {
      rebuildActive = false;
      readyFlag = false;
      setState("raw replay failed");
      SerialLog::error("STATS", "Aggregate reconciliation stopped: raw record unavailable");
      break;
    }
    if (!applyDecoded(raw, rebuildSequence)) {
      rebuildActive = false;
      readyFlag = false;
      setState("aggregate replay write failed");
      break;
    }
    ++rebuildSequence;
  }
}

}  // namespace

bool begin() {
  readyFlag = false;
  rebuildActive = false;
  cacheValid = false;
  meta = Meta{};

  if (!InterruptionStore::ready()) {
    setState("raw store not ready");
    return false;
  }
  if (!openFiles()) {
    setState("aggregate files unavailable");
    return false;
  }

  if (!loadMeta()) {
    if (dataFile.size() == 0) {
      readyFlag = commitMeta();
      setState(readyFlag ? "none" : "aggregate metadata initialization failed");
    } else {
      startRebuildFromRaw("metadata invalid");
    }
  } else {
    readyFlag = true;

    // If raw metadata had to be reconstructed, its high sequence bits may have
    // been rebased. A valid aggregate checkpoint is an independent durable
    // lower bound. Align raw numbering before comparing/catching up.
    if (meta.lastProcessedSequence > InterruptionStore::newestSequence() &&
        !InterruptionStore::alignSequenceAtLeast(meta.lastProcessedSequence)) {
      readyFlag = false;
      setState("raw sequence alignment failed");
      return false;
    }

    const uint64_t oldest = InterruptionStore::oldestSequence();
    const uint64_t newest = InterruptionStore::newestSequence();
    if (newest > meta.lastProcessedSequence) {
      rebuildSequence = meta.lastProcessedSequence + 1ULL;
      if (rebuildSequence < oldest) rebuildSequence = oldest;
      rebuildActive = rebuildSequence != 0 && rebuildSequence <= newest;
      if (rebuildActive) setState("catching up raw events");
    }
  }

  updateInfo();
  SerialLog::infof(
      "STATS",
      "Daily aggregates | days=%lu/%u | last-sequence=%llu | unassigned=%lu",
      static_cast<unsigned long>(meta.count),
      ProjectConfig::DAILY_AGGREGATE_CAPACITY,
      static_cast<unsigned long long>(meta.lastProcessedSequence),
      static_cast<unsigned long>(meta.unassignedCount));
  return readyFlag || rebuildActive;
}

void update() {
  if (rebuildActive) reconcileStep();
  updateInfo();
}

bool ready() {
  return readyFlag && !rebuildActive;
}

const Info &info() {
  updateInfo();
  return currentInfo;
}

bool apply(const InterruptionTypes::CapturedEvent &event, uint64_t sequence) {
  if (!readyFlag) return false;
  if (sequence <= meta.lastProcessedSequence) return true;

  // If a prior aggregate write failed while raw persistence continued, never
  // skip the missing older raw event. Switch to cooperative replay first.
  if (sequence > meta.lastProcessedSequence + 1ULL) {
    const uint64_t oldest = InterruptionStore::oldestSequence();
    rebuildSequence = meta.lastProcessedSequence + 1ULL;
    if (rebuildSequence < oldest) rebuildSequence = oldest;
    rebuildActive = rebuildSequence != 0 && rebuildSequence <= InterruptionStore::newestSequence();
    if (rebuildActive) setState("catching up raw events");
    return false;
  }

  const Meta beforeMeta = meta;
  const bool beforeCacheValid = cacheValid;
  const uint16_t beforeCachedDay = cachedDay;
  const uint32_t beforeCachedSlot = cachedSlot;
  auto rollbackMeta = [&]() {
    meta = beforeMeta;
    cacheValid = beforeCacheValid;
    cachedDay = beforeCachedDay;
    cachedSlot = beforeCachedSlot;
    updateInfo();
  };

  if (!event.localCalendarValid) {
    if (meta.unassignedCount < UINT32_MAX) ++meta.unassignedCount;
    meta.lastProcessedSequence = sequence;
    if (!commitMeta()) {
      rollbackMeta();
      setState("daily aggregate metadata failed");
      return false;
    }
    setState("none");
    return true;
  }

  DailyRecord record;
  DailyRecord displacedRecord;
  bool displacedRecordValid = false;
  uint32_t slot = 0;
  const bool existing = locate(event.localDayIndex, record, slot);
  if (locateReadError) {
    rollbackMeta();
    startRebuildFromRaw("daily record unreadable or CRC invalid");
    return false;
  }

  if (existing && record.lastSequence >= sequence) {
    meta.lastProcessedSequence = sequence;
    if (!commitMeta()) {
      rollbackMeta();
      setState("daily aggregate metadata failed");
      return false;
    }
    setState("none");
    return true;
  }

  if (!existing) {
    record = DailyRecord{};
    record.dayIndex = event.localDayIndex;
    slot = meta.writeIndex;

    // When the daily ring is full this target still belongs to the last durable
    // metadata state. Back it up so a failed meta commit can restore it.
    if (beforeMeta.count >= ProjectConfig::DAILY_AGGREGATE_CAPACITY) {
      displacedRecordValid = readSlot(slot, displacedRecord);
      if (!displacedRecordValid) {
        rollbackMeta();
        startRebuildFromRaw("daily overwrite target unreadable");
        return false;
      }
    }
  }

  if (record.total < UINT16_MAX) ++record.total;
  if (event.localHour < 24 && record.hours[event.localHour] < UINT16_MAX) {
    ++record.hours[event.localHour];
  }
  record.lastSequence = sequence;
  if (!writeSlot(slot, record)) {
    rollbackMeta();
    setState("daily aggregate write failed");
    return false;
  }

  if (!existing) {
    meta.writeIndex = (beforeMeta.writeIndex + 1U) % ProjectConfig::DAILY_AGGREGATE_CAPACITY;
    meta.count = beforeMeta.count < ProjectConfig::DAILY_AGGREGATE_CAPACITY
                     ? beforeMeta.count + 1U
                     : beforeMeta.count;
  }
  meta.lastProcessedSequence = sequence;

  if (!commitMeta()) {
    if (!existing && displacedRecordValid && !writeSlot(slot, displacedRecord)) {
      rollbackMeta();
      startRebuildFromRaw("daily transaction rollback failed");
      return false;
    }

    // Existing-day data may already contain this sequence. Replay is idempotent
    // via record.lastSequence and then only needs to re-commit metadata.
    rollbackMeta();
    setState("daily aggregate metadata failed");
    return false;
  }

  cachedDay = event.localDayIndex;
  cachedSlot = slot;
  cacheValid = true;
  setState("none");
  return true;
}

bool find(uint16_t dayIndex, DailyRecord &recordOut) {
  uint32_t slot = 0;
  const bool found = locate(dayIndex, recordOut, slot);
  if (!found && locateReadError) {
    startRebuildFromRaw("daily record unreadable during lookup");
  }
  return found;
}

uint32_t countForDay(uint16_t dayIndex) {
  DailyRecord record;
  return find(dayIndex, record) ? record.total : 0;
}

bool forEach(DailyVisitor visitor, void *context) {
  if (!visitor || (!readyFlag && !rebuildActive)) return false;

  const uint32_t start = oldestSlot();
  for (uint32_t i = 0; i < meta.count; ++i) {
    DailyRecord record;
    const uint32_t slot = (start + i) % ProjectConfig::DAILY_AGGREGATE_CAPACITY;
    if (!readSlot(slot, record)) {
      startRebuildFromRaw("daily record unreadable during statistics read");
      return false;
    }
    if (!visitor(record, context)) return true;
  }
  return true;
}

}  // namespace InterruptionAggregates
