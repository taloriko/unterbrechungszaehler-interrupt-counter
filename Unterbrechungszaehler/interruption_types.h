#pragma once

#include <Arduino.h>

#include "time_types.h"

namespace InterruptionTypes {

enum class EventSource : uint8_t {
  Unknown = 0,
  PhysicalButton = 1,
  WebButton = 2,
  Software = 3,
  Api = 4,
  Hardware = 5
};

enum class StorageState : uint8_t {
  Unavailable,
  Ready,
  Warning,
  Error
};

constexpr uint32_t DELTA_MAX = 131069U;
constexpr uint32_t DELTA_UNKNOWN = 131070U;
constexpr uint32_t DELTA_FIRST_OF_DAY = 131071U;

struct CapturedEvent {
  uint32_t timeValueSeconds = 0;  // epoch seconds if absolute, monotonic seconds otherwise
  uint32_t deltaSeconds = DELTA_UNKNOWN;
  uint64_t monotonicMs = 0;
  uint16_t localDayIndex = 0;
  uint8_t localHour = 0;
  TimeTypes::Source timeSource = TimeTypes::Source::Relative;
  EventSource eventSource = EventSource::Unknown;
  bool absoluteValid = false;
  bool localCalendarValid = false;
};

struct RawEvent {
  uint32_t timeValueSeconds = 0;
  uint32_t deltaSeconds = DELTA_UNKNOWN;
  TimeTypes::Source timeSource = TimeTypes::Source::Relative;
  EventSource eventSource = EventSource::Unknown;
  bool absoluteValid = false;
  uint8_t sequenceTag = 0;
};

struct Summary {
  uint32_t todayCount = 0;
  uint64_t todayIntervalSumSeconds = 0;
  uint32_t todayIntervalSamples = 0;
  uint32_t unassignedCount = 0;
  uint64_t liveSequence = 0;
  uint64_t persistedSequence = 0;
  uint64_t revision = 0;  // visible summary changes, independent of event sequence
  uint8_t pendingCount = 0;
  // Events accepted for immediate UI/feedback but not queued for persistence
  // because the fixed RAM queue was full. This is boot-local and deliberately
  // visible so durability loss is never hidden from the user.
  uint32_t droppedCount = 0;
  StorageState storageState = StorageState::Unavailable;
  bool soundEnabled = true;

  bool lastAvailable = false;
  bool lastAbsoluteValid = false;
  uint32_t lastTimeValueSeconds = 0;
  uint64_t lastMonotonicMs = 0;
  uint16_t lastLocalDayIndex = 0;
  TimeTypes::Source lastTimeSource = TimeTypes::Source::Relative;
  EventSource lastEventSource = EventSource::Unknown;
  uint32_t lastDeltaSeconds = DELTA_UNKNOWN;
};

inline const char *eventSourceName(EventSource source) {
  switch (source) {
    case EventSource::PhysicalButton: return "physical_button";
    case EventSource::WebButton: return "web_button";
    case EventSource::Software: return "software";
    case EventSource::Api: return "api";
    case EventSource::Hardware: return "hardware";
    case EventSource::Unknown:
    default: return "unknown";
  }
}

inline const char *storageStateName(StorageState state) {
  switch (state) {
    case StorageState::Ready: return "ready";
    case StorageState::Warning: return "warning";
    case StorageState::Error: return "error";
    case StorageState::Unavailable:
    default: return "unavailable";
  }
}

}  // namespace InterruptionTypes
