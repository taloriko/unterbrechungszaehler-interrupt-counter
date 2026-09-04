#pragma once

#include <Arduino.h>

namespace SourceRegistry {

constexpr uint8_t SOURCE_ID_UNKNOWN = 0;
constexpr uint8_t SOURCE_ID_MASTER = 1;
constexpr uint8_t SOURCE_ID_WEB = 2;
constexpr uint8_t SOURCE_ID_SOFTWARE = 3;
constexpr uint8_t SOURCE_ID_API = 4;
constexpr uint8_t SOURCE_ID_HARDWARE = 5;
constexpr uint8_t SOURCE_ID_RADIO_FIRST = 6;
constexpr uint8_t SOURCE_ID_RADIO_LAST = 15;
constexpr uint8_t SOURCE_ID_MAX = 15;
constexpr uint8_t RADIO_SOURCE_CAPACITY = SOURCE_ID_RADIO_LAST - SOURCE_ID_RADIO_FIRST + 1;
constexpr size_t SOURCE_NAME_MAX_BYTES = 23;

struct Entry {
  uint8_t sourceId = SOURCE_ID_UNKNOWN;
  bool assigned = false;
  bool bound = false;
  uint8_t bitCount = 0;
  uint8_t pulseBucket = 0;
  uint32_t code = 0;
  char name[SOURCE_NAME_MAX_BYTES + 1]{};
};

struct LearnState {
  bool active = false;
  uint8_t targetSourceId = SOURCE_ID_UNKNOWN;
  uint32_t startedMs = 0;
  uint32_t timeoutMs = 30000;
  char pendingName[SOURCE_NAME_MAX_BYTES + 1]{};
  const char *error = "none";
};

bool begin();
void update();

const Entry *entryForSource(uint8_t sourceId);
const Entry *entryAt(size_t radioIndex);
size_t assignedRadioCount();
size_t boundRadioCount();

const char *sourceName(uint8_t sourceId);
const char *sourceKind(uint8_t sourceId);

bool startLearn(const char *name, uint8_t targetSourceId = SOURCE_ID_UNKNOWN);
void cancelLearn();
const LearnState &learnState();

// Returns a stable logical source id for a received fixed-code frame.
// During learn mode the first confirmed frame is bound but deliberately not
// emitted as an interruption, so setup does not pollute the statistics.
bool consumeFrame(uint32_t code,
                  uint8_t bitCount,
                  uint8_t pulseBucket,
                  uint8_t &sourceIdOut,
                  bool &learnedOut);

bool renameSource(uint8_t sourceId, const char *name);
bool unbindSource(uint8_t sourceId);

}  // namespace SourceRegistry
