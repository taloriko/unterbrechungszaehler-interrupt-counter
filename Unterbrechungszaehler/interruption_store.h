#pragma once

#include <Arduino.h>

#include "interruption_types.h"

namespace InterruptionStore {

struct Info {
  bool mounted = false;
  bool ready = false;
  bool recovering = false;
  uint32_t count = 0;
  uint32_t capacity = 0;
  uint64_t totalSequence = 0;
  size_t fsTotalBytes = 0;
  size_t fsUsedBytes = 0;
  const char *error = "none";
};

bool begin();
void update();

bool ready();
bool recovering();
const Info &info();

bool append(const InterruptionTypes::CapturedEvent &event, uint64_t &sequenceOut);
bool readSequence(uint64_t sequence, InterruptionTypes::RawEvent &eventOut);
bool readLast(InterruptionTypes::RawEvent &eventOut, uint64_t &sequenceOut);

uint64_t oldestSequence();
uint64_t newestSequence();
uint32_t count();
uint32_t capacity();
// Latest persisted event with a valid local calendar, independent of whether
// newer relative-only raw events exist. Used to restore same-day delta anchor.
bool lastCalendarAnchor(uint16_t &dayIndexOut, uint32_t &epochSecondsOut);
// After a catastrophic raw-metadata recovery only the low sequence tag stored
// in each record is known. A still-valid derived store can provide a durable
// lower-bound hint so the raw logical sequence is lifted by whole 256-count
// epochs without changing ring order or any record bytes.
bool alignSequenceAtLeast(uint64_t durableHint);

}  // namespace InterruptionStore
