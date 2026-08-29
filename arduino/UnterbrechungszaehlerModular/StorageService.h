#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include "Config.h"

struct __attribute__((packed)) TimestampRingHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t capacity;
  uint32_t writeIndex;
  uint32_t count;
};

enum class AutarkRecordType : uint8_t {
  Start = 1,
  Event = 2,
  End = 3
};

struct __attribute__((packed)) AutarkHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t capacity;
  uint32_t writeIndex;
  uint32_t count;
  uint32_t nextSessionId;
};

struct __attribute__((packed)) AutarkRecord {
  uint32_t sessionId;
  uint32_t elapsedSec;
  uint32_t anchorEpoch;
  uint8_t type;
  uint8_t flags;
  uint16_t reserved;
};

class StorageService {
public:
  StorageService();

  bool begin();

  bool fsReady() const { return fsReady_; }
  bool recentReady() const { return recent_.ready; }
  bool archiveReady() const { return archive_.ready; }
  bool archiveSynchronized() const { return archiveSynchronized_; }
  bool autarkReady() const { return autarkReady_; }

  uint32_t recentCount() const { return recent_.header.count; }
  uint32_t recentCapacity() const { return recent_.header.capacity; }
  uint32_t archiveCount() const { return archive_.header.count; }
  uint32_t archiveCapacity() const { return archive_.header.capacity; }
  uint32_t autarkCount() const { return autarkHeader_.count; }
  uint32_t autarkCapacity() const { return autarkHeader_.capacity; }
  uint32_t revision() const { return revision_; }

  size_t fsTotalBytes() const;
  size_t fsUsedBytes() const;

  bool appendEvent(uint32_t epoch);
  bool deleteLastEvent();
  bool readRecent(uint32_t chronologicalIndex, uint32_t& epoch);
  bool readArchive(uint32_t chronologicalIndex, uint32_t& epoch);

  // Schneller Blockzugriff fuer Heatmaps/Statistiken. Die Datei wird nur
  // einmal pro Block geoeffnet. Ein Block darf den Ring-Uebergang enthalten.
  bool readArchiveChunk(uint32_t chronologicalStart,
                        uint32_t maxItems,
                        uint32_t* epochs,
                        uint32_t& readCount) {
    readCount = 0;
    if (!archive_.ready || !epochs || maxItems == 0 || chronologicalStart >= archive_.header.count) return false;

    const uint32_t remaining = archive_.header.count - chronologicalStart;
    const uint32_t wanted = remaining < maxItems ? remaining : maxItems;
    const uint32_t oldest = (archive_.header.writeIndex + archive_.header.capacity - archive_.header.count) % archive_.header.capacity;
    uint32_t physical = (oldest + chronologicalStart) % archive_.header.capacity;

    File file = LittleFS.open(archive_.path, FILE_READ);
    if (!file) return false;

    uint32_t left = wanted;
    while (left > 0) {
      const uint32_t contiguous = (physical + left <= archive_.header.capacity)
                                    ? left
                                    : (archive_.header.capacity - physical);
      if (!file.seek(timestampOffset(physical), SeekSet)) {
        file.close();
        return false;
      }
      const size_t bytesWanted = static_cast<size_t>(contiguous) * sizeof(uint32_t);
      const size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(epochs + readCount), bytesWanted);
      const uint32_t itemsRead = static_cast<uint32_t>(bytesRead / sizeof(uint32_t));
      readCount += itemsRead;
      left -= itemsRead;
      if (itemsRead != contiguous) {
        file.close();
        return false;
      }
      physical = 0;
    }

    file.close();
    return readCount == wanted;
  }

  uint32_t lastEvent();

  bool startAutarkSession(uint32_t anchorEpoch, uint32_t& sessionId);
  bool appendAutarkEvent(uint32_t sessionId, uint32_t elapsedSec);
  bool endAutarkSession(uint32_t sessionId, uint32_t elapsedSec, uint32_t anchorEpoch);
  bool deleteLastAutarkEvent(uint32_t sessionId);
  bool readAutark(uint32_t chronologicalIndex, AutarkRecord& record);
  bool updateAutarkStartAnchor(uint32_t sessionId, uint32_t anchorEpoch);
  bool updateAutarkEndAnchor(uint32_t sessionId, uint32_t anchorEpoch);

private:
  struct RingDescriptor {
    const char* path;
    uint32_t magic;
    uint16_t version;
    uint32_t configuredCapacity;
    TimestampRingHeader header;
    bool ready;
  };

  bool initTimestampRing(RingDescriptor& ring);
  bool createTimestampRing(RingDescriptor& ring);
  bool loadTimestampRing(RingDescriptor& ring);
  bool writeTimestampHeader(File& file, const RingDescriptor& ring);
  bool appendTimestamp(RingDescriptor& ring, uint32_t epoch);
  bool readTimestamp(RingDescriptor& ring, uint32_t chronologicalIndex, uint32_t& epoch);
  bool popTimestamp(RingDescriptor& ring);
  uint32_t lastTimestamp(RingDescriptor& ring);
  size_t timestampOffset(uint32_t index) const;
  void quarantineFile(const char* path);

  bool initAutarkRing();
  bool createAutarkRing();
  bool loadAutarkHeader();
  bool writeAutarkHeader(File& file);
  bool appendAutarkRecord(AutarkRecordType type, uint32_t sessionId, uint32_t elapsedSec, uint32_t anchorEpoch);
  bool updateAutarkAnchor(uint32_t sessionId, AutarkRecordType type, uint32_t anchorEpoch);
  size_t autarkOffset(uint32_t index) const;

  void migrateLegacyEvents();
  void seedArchiveFromRecent();

  RingDescriptor recent_;
  RingDescriptor archive_;
  AutarkHeader autarkHeader_ = {};

  bool fsReady_ = false;
  bool autarkReady_ = false;
  bool archiveSynchronized_ = true;
  uint32_t revision_ = 0;
};
