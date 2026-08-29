#include "StorageService.h"

StorageService::StorageService()
  : recent_{UicConfig::RECENT_FILE, UicConfig::RECENT_MAGIC, UicConfig::RECENT_VERSION,
            UicConfig::RECENT_CAPACITY, {}, false},
    archive_{UicConfig::LONGTERM_FILE, UicConfig::LONGTERM_MAGIC, UicConfig::LONGTERM_VERSION,
             UicConfig::LONGTERM_CAPACITY, {}, false} {
}

bool StorageService::begin() {
  fsReady_ = LittleFS.begin(false);
  if (!fsReady_) {
    Serial.println("[SPEICHER] LittleFS konnte nicht eingebunden werden. Es wird nicht automatisch formatiert.");
    return false;
  }

  recent_.ready = initTimestampRing(recent_);
  archive_.ready = initTimestampRing(archive_);
  autarkReady_ = initAutarkRing();

  if (recent_.ready) migrateLegacyEvents();
  if (recent_.ready && archive_.ready) seedArchiveFromRecent();

  Serial.printf("[SPEICHER] Recent=%s %lu/%lu | Langzeit=%s %lu/%lu | Autark=%s %lu/%lu\n",
                recent_.ready ? "OK" : "FEHLER",
                static_cast<unsigned long>(recent_.header.count),
                static_cast<unsigned long>(recent_.header.capacity),
                archive_.ready ? "OK" : "FEHLER",
                static_cast<unsigned long>(archive_.header.count),
                static_cast<unsigned long>(archive_.header.capacity),
                autarkReady_ ? "OK" : "FEHLER",
                static_cast<unsigned long>(autarkHeader_.count),
                static_cast<unsigned long>(autarkHeader_.capacity));

  return recent_.ready;
}

size_t StorageService::fsTotalBytes() const {
  return fsReady_ ? LittleFS.totalBytes() : 0;
}

size_t StorageService::fsUsedBytes() const {
  return fsReady_ ? LittleFS.usedBytes() : 0;
}

bool StorageService::appendEvent(uint32_t epoch) {
  if (!recent_.ready || epoch <= UicConfig::VALID_TIME_MIN) return false;

  if (!appendTimestamp(recent_, epoch)) return false;
  revision_++;

  if (archive_.ready) {
    if (!appendTimestamp(archive_, epoch)) archiveSynchronized_ = false;
  } else {
    archiveSynchronized_ = false;
  }

  return true;
}

bool StorageService::deleteLastEvent() {
  if (!recent_.ready || recent_.header.count == 0) return false;

  const uint32_t expected = lastTimestamp(recent_);
  if (expected == 0 || !popTimestamp(recent_)) return false;
  revision_++;

  if (archive_.ready && archive_.header.count > 0) {
    const uint32_t archiveLast = lastTimestamp(archive_);
    if (archiveLast == expected) {
      if (!popTimestamp(archive_)) archiveSynchronized_ = false;
    } else {
      archiveSynchronized_ = false;
    }
  } else if (archive_.ready) {
    archiveSynchronized_ = false;
  }

  return true;
}

bool StorageService::readRecent(uint32_t chronologicalIndex, uint32_t& epoch) {
  return readTimestamp(recent_, chronologicalIndex, epoch);
}

bool StorageService::readArchive(uint32_t chronologicalIndex, uint32_t& epoch) {
  return readTimestamp(archive_, chronologicalIndex, epoch);
}

uint32_t StorageService::lastEvent() {
  return lastTimestamp(recent_);
}

bool StorageService::startAutarkSession(uint32_t anchorEpoch, uint32_t& sessionId) {
  if (!autarkReady_) return false;

  sessionId = autarkHeader_.nextSessionId++;
  if (sessionId == 0) {
    sessionId = 1;
    autarkHeader_.nextSessionId = 2;
  }

  File file = LittleFS.open(UicConfig::AUTARK_FILE, "r+");
  if (!file) return false;
  const bool headerOk = writeAutarkHeader(file);
  file.close();
  if (!headerOk) return false;

  return appendAutarkRecord(AutarkRecordType::Start, sessionId, 0, anchorEpoch);
}

bool StorageService::appendAutarkEvent(uint32_t sessionId, uint32_t elapsedSec) {
  return appendAutarkRecord(AutarkRecordType::Event, sessionId, elapsedSec, 0);
}

bool StorageService::endAutarkSession(uint32_t sessionId, uint32_t elapsedSec, uint32_t anchorEpoch) {
  return appendAutarkRecord(AutarkRecordType::End, sessionId, elapsedSec, anchorEpoch);
}

bool StorageService::deleteLastAutarkEvent(uint32_t sessionId) {
  if (!autarkReady_ || autarkHeader_.count == 0) return false;

  AutarkRecord record = {};
  if (!readAutark(autarkHeader_.count - 1, record)) return false;
  if (record.sessionId != sessionId || record.type != static_cast<uint8_t>(AutarkRecordType::Event)) return false;

  File file = LittleFS.open(UicConfig::AUTARK_FILE, "r+");
  if (!file) return false;
  autarkHeader_.writeIndex = (autarkHeader_.writeIndex + autarkHeader_.capacity - 1) % autarkHeader_.capacity;
  autarkHeader_.count--;
  const bool ok = writeAutarkHeader(file);
  file.close();
  return ok;
}

bool StorageService::readAutark(uint32_t chronologicalIndex, AutarkRecord& record) {
  if (!autarkReady_ || chronologicalIndex >= autarkHeader_.count) return false;

  const uint32_t oldest = (autarkHeader_.writeIndex + autarkHeader_.capacity - autarkHeader_.count) % autarkHeader_.capacity;
  const uint32_t physical = (oldest + chronologicalIndex) % autarkHeader_.capacity;

  File file = LittleFS.open(UicConfig::AUTARK_FILE, FILE_READ);
  if (!file) return false;
  const bool ok = file.seek(autarkOffset(physical), SeekSet) &&
                  file.read(reinterpret_cast<uint8_t*>(&record), sizeof(record)) == sizeof(record);
  file.close();
  return ok;
}

bool StorageService::updateAutarkStartAnchor(uint32_t sessionId, uint32_t anchorEpoch) {
  return updateAutarkAnchor(sessionId, AutarkRecordType::Start, anchorEpoch);
}

bool StorageService::updateAutarkEndAnchor(uint32_t sessionId, uint32_t anchorEpoch) {
  return updateAutarkAnchor(sessionId, AutarkRecordType::End, anchorEpoch);
}

bool StorageService::initTimestampRing(RingDescriptor& ring) {
  if (loadTimestampRing(ring)) return true;

  if (LittleFS.exists(ring.path)) quarantineFile(ring.path);
  if (!createTimestampRing(ring)) return false;
  return loadTimestampRing(ring);
}

bool StorageService::createTimestampRing(RingDescriptor& ring) {
  File file = LittleFS.open(ring.path, FILE_WRITE);
  if (!file) return false;

  ring.header.magic = ring.magic;
  ring.header.version = ring.version;
  ring.header.reserved = 0;
  ring.header.capacity = ring.configuredCapacity;
  ring.header.writeIndex = 0;
  ring.header.count = 0;

  if (!writeTimestampHeader(file, ring)) {
    file.close();
    return false;
  }

  const size_t finalSize = sizeof(TimestampRingHeader) + static_cast<size_t>(ring.configuredCapacity) * sizeof(uint32_t);
  if (!file.seek(finalSize - 1, SeekSet)) {
    file.close();
    return false;
  }

  const uint8_t zero = 0;
  const bool ok = file.write(&zero, 1) == 1;
  file.flush();
  file.close();
  return ok;
}

bool StorageService::loadTimestampRing(RingDescriptor& ring) {
  if (!LittleFS.exists(ring.path)) return false;
  File file = LittleFS.open(ring.path, FILE_READ);
  if (!file) return false;

  const bool readOk = file.read(reinterpret_cast<uint8_t*>(&ring.header), sizeof(ring.header)) == sizeof(ring.header);
  file.close();
  if (!readOk) return false;

  return ring.header.magic == ring.magic &&
         ring.header.version == ring.version &&
         ring.header.capacity == ring.configuredCapacity &&
         ring.header.writeIndex < ring.header.capacity &&
         ring.header.count <= ring.header.capacity;
}

bool StorageService::writeTimestampHeader(File& file, const RingDescriptor& ring) {
  if (!file.seek(0, SeekSet)) return false;
  const size_t written = file.write(reinterpret_cast<const uint8_t*>(&ring.header), sizeof(ring.header));
  file.flush();
  return written == sizeof(ring.header);
}

bool StorageService::appendTimestamp(RingDescriptor& ring, uint32_t epoch) {
  if (!ring.ready) return false;
  File file = LittleFS.open(ring.path, "r+");
  if (!file) return false;

  if (!file.seek(timestampOffset(ring.header.writeIndex), SeekSet) ||
      file.write(reinterpret_cast<const uint8_t*>(&epoch), sizeof(epoch)) != sizeof(epoch)) {
    file.close();
    return false;
  }

  ring.header.writeIndex = (ring.header.writeIndex + 1) % ring.header.capacity;
  if (ring.header.count < ring.header.capacity) ring.header.count++;
  const bool ok = writeTimestampHeader(file, ring);
  file.close();
  return ok;
}

bool StorageService::readTimestamp(RingDescriptor& ring, uint32_t chronologicalIndex, uint32_t& epoch) {
  if (!ring.ready || chronologicalIndex >= ring.header.count) return false;

  const uint32_t oldest = (ring.header.writeIndex + ring.header.capacity - ring.header.count) % ring.header.capacity;
  const uint32_t physical = (oldest + chronologicalIndex) % ring.header.capacity;

  File file = LittleFS.open(ring.path, FILE_READ);
  if (!file) return false;
  const bool ok = file.seek(timestampOffset(physical), SeekSet) &&
                  file.read(reinterpret_cast<uint8_t*>(&epoch), sizeof(epoch)) == sizeof(epoch);
  file.close();
  return ok;
}

bool StorageService::popTimestamp(RingDescriptor& ring) {
  if (!ring.ready || ring.header.count == 0) return false;
  File file = LittleFS.open(ring.path, "r+");
  if (!file) return false;

  ring.header.writeIndex = (ring.header.writeIndex + ring.header.capacity - 1) % ring.header.capacity;
  ring.header.count--;
  const bool ok = writeTimestampHeader(file, ring);
  file.close();
  return ok;
}

uint32_t StorageService::lastTimestamp(RingDescriptor& ring) {
  if (!ring.ready || ring.header.count == 0) return 0;
  uint32_t epoch = 0;
  readTimestamp(ring, ring.header.count - 1, epoch);
  return epoch;
}

size_t StorageService::timestampOffset(uint32_t index) const {
  return sizeof(TimestampRingHeader) + static_cast<size_t>(index) * sizeof(uint32_t);
}

void StorageService::quarantineFile(const char* path) {
  String backup(path);
  backup += ".invalid";
  if (LittleFS.exists(backup.c_str())) LittleFS.remove(backup.c_str());
  if (LittleFS.rename(path, backup.c_str())) {
    Serial.printf("[SPEICHER] Ungueltige Datei gesichert: %s -> %s\n", path, backup.c_str());
  }
}

bool StorageService::initAutarkRing() {
  if (loadAutarkHeader()) return true;

  if (LittleFS.exists(UicConfig::AUTARK_FILE)) quarantineFile(UicConfig::AUTARK_FILE);
  if (!createAutarkRing()) return false;
  return loadAutarkHeader();
}

bool StorageService::createAutarkRing() {
  File file = LittleFS.open(UicConfig::AUTARK_FILE, FILE_WRITE);
  if (!file) return false;

  autarkHeader_.magic = UicConfig::AUTARK_MAGIC;
  autarkHeader_.version = UicConfig::AUTARK_VERSION;
  autarkHeader_.reserved = 0;
  autarkHeader_.capacity = UicConfig::AUTARK_CAPACITY;
  autarkHeader_.writeIndex = 0;
  autarkHeader_.count = 0;
  autarkHeader_.nextSessionId = 1;

  if (!writeAutarkHeader(file)) {
    file.close();
    return false;
  }

  const size_t finalSize = sizeof(AutarkHeader) + static_cast<size_t>(autarkHeader_.capacity) * sizeof(AutarkRecord);
  if (!file.seek(finalSize - 1, SeekSet)) {
    file.close();
    return false;
  }

  const uint8_t zero = 0;
  const bool ok = file.write(&zero, 1) == 1;
  file.flush();
  file.close();
  return ok;
}

bool StorageService::loadAutarkHeader() {
  if (!LittleFS.exists(UicConfig::AUTARK_FILE)) return false;
  File file = LittleFS.open(UicConfig::AUTARK_FILE, FILE_READ);
  if (!file) return false;

  const bool readOk = file.read(reinterpret_cast<uint8_t*>(&autarkHeader_), sizeof(autarkHeader_)) == sizeof(autarkHeader_);
  file.close();
  if (!readOk) return false;

  return autarkHeader_.magic == UicConfig::AUTARK_MAGIC &&
         autarkHeader_.version == UicConfig::AUTARK_VERSION &&
         autarkHeader_.capacity == UicConfig::AUTARK_CAPACITY &&
         autarkHeader_.writeIndex < autarkHeader_.capacity &&
         autarkHeader_.count <= autarkHeader_.capacity &&
         autarkHeader_.nextSessionId > 0;
}

bool StorageService::writeAutarkHeader(File& file) {
  if (!file.seek(0, SeekSet)) return false;
  const size_t written = file.write(reinterpret_cast<const uint8_t*>(&autarkHeader_), sizeof(autarkHeader_));
  file.flush();
  return written == sizeof(autarkHeader_);
}

bool StorageService::appendAutarkRecord(AutarkRecordType type, uint32_t sessionId, uint32_t elapsedSec, uint32_t anchorEpoch) {
  if (!autarkReady_) return false;
  File file = LittleFS.open(UicConfig::AUTARK_FILE, "r+");
  if (!file) return false;

  AutarkRecord record = {};
  record.sessionId = sessionId;
  record.elapsedSec = elapsedSec;
  record.anchorEpoch = anchorEpoch;
  record.type = static_cast<uint8_t>(type);

  if (!file.seek(autarkOffset(autarkHeader_.writeIndex), SeekSet) ||
      file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record)) != sizeof(record)) {
    file.close();
    return false;
  }

  autarkHeader_.writeIndex = (autarkHeader_.writeIndex + 1) % autarkHeader_.capacity;
  if (autarkHeader_.count < autarkHeader_.capacity) autarkHeader_.count++;
  const bool ok = writeAutarkHeader(file);
  file.close();
  return ok;
}

bool StorageService::updateAutarkAnchor(uint32_t sessionId, AutarkRecordType type, uint32_t anchorEpoch) {
  if (!autarkReady_ || anchorEpoch <= UicConfig::VALID_TIME_MIN || autarkHeader_.count == 0) return false;

  File file = LittleFS.open(UicConfig::AUTARK_FILE, "r+");
  if (!file) return false;

  const uint32_t oldest = (autarkHeader_.writeIndex + autarkHeader_.capacity - autarkHeader_.count) % autarkHeader_.capacity;

  for (uint32_t reverse = 0; reverse < autarkHeader_.count; reverse++) {
    const uint32_t chronological = autarkHeader_.count - 1 - reverse;
    const uint32_t physical = (oldest + chronological) % autarkHeader_.capacity;

    if (!file.seek(autarkOffset(physical), SeekSet)) continue;
    AutarkRecord record = {};
    if (file.read(reinterpret_cast<uint8_t*>(&record), sizeof(record)) != sizeof(record)) continue;
    if (record.sessionId != sessionId) {
      if (reverse > 0) break;
      continue;
    }
    if (record.type != static_cast<uint8_t>(type)) continue;

    record.anchorEpoch = anchorEpoch;
    if (!file.seek(autarkOffset(physical), SeekSet)) {
      file.close();
      return false;
    }
    const bool ok = file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record)) == sizeof(record);
    file.flush();
    file.close();
    return ok;
  }

  file.close();
  return false;
}

size_t StorageService::autarkOffset(uint32_t index) const {
  return sizeof(AutarkHeader) + static_cast<size_t>(index) * sizeof(AutarkRecord);
}

void StorageService::migrateLegacyEvents() {
  if (!LittleFS.exists(UicConfig::LEGACY_FILE)) return;

  File file = LittleFS.open(UicConfig::LEGACY_FILE, FILE_READ);
  if (!file) return;

  uint32_t migrated = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    const uint32_t epoch = strtoul(line.c_str(), nullptr, 10);
    if (epoch <= UicConfig::VALID_TIME_MIN) continue;
    if (appendTimestamp(recent_, epoch)) migrated++;
  }
  file.close();

  String migratedPath(UicConfig::LEGACY_FILE);
  migratedPath += ".migrated";
  if (LittleFS.exists(migratedPath.c_str())) LittleFS.remove(migratedPath.c_str());
  LittleFS.rename(UicConfig::LEGACY_FILE, migratedPath.c_str());
  if (migrated) revision_++;
  Serial.printf("[SPEICHER] %lu Legacy-Eintraege uebernommen.\n", static_cast<unsigned long>(migrated));
}

void StorageService::seedArchiveFromRecent() {
  if (!archive_.ready || archive_.header.count != 0 || !recent_.ready || recent_.header.count == 0) return;

  uint32_t copied = 0;
  for (uint32_t i = 0; i < recent_.header.count; i++) {
    uint32_t epoch = 0;
    if (!readTimestamp(recent_, i, epoch)) continue;
    if (appendTimestamp(archive_, epoch)) copied++;
    else {
      archiveSynchronized_ = false;
      break;
    }
  }
  Serial.printf("[SPEICHER] Langzeitarchiv mit %lu bestehenden Eintraegen initialisiert.\n",
                static_cast<unsigned long>(copied));
}
