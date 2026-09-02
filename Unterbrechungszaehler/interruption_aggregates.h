#pragma once

#include <Arduino.h>

#include "interruption_types.h"

namespace InterruptionAggregates {

struct DailyRecord {
  uint16_t dayIndex = 0;
  uint16_t total = 0;
  uint16_t hours[24]{};
  uint64_t lastSequence = 0;
};

struct Info {
  bool ready = false;
  bool rebuilding = false;
  uint16_t dayCount = 0;
  uint16_t capacity = 0;
  uint64_t lastProcessedSequence = 0;
  uint32_t unassignedCount = 0;
  const char *error = "none";
};

using DailyVisitor = bool (*)(const DailyRecord &record, void *context);

bool begin();
void update();
bool ready();
const Info &info();

bool apply(const InterruptionTypes::CapturedEvent &event, uint64_t sequence);
bool find(uint16_t dayIndex, DailyRecord &recordOut);
bool forEach(DailyVisitor visitor, void *context);
uint32_t countForDay(uint16_t dayIndex);

}  // namespace InterruptionAggregates
