#include "focus_insights.h"

#include <algorithm>

#include "hardware_registry.h"
#include "interruption_service.h"
#include "interruption_store.h"
#include "interruption_types.h"
#include "project_config.h"
#include "project_time.h"
#include "time_service.h"

namespace FocusInsights {
namespace {

constexpr uint32_t CACHE_MAX_AGE_MS = ProjectConfig::FOCUS_INSIGHTS_CACHE_MAX_AGE_MS;
constexpr uint8_t PATTERN_DAYS = ProjectConfig::FOCUS_PATTERN_DAYS;
constexpr uint8_t MIN_COVERED_DAYS = ProjectConfig::FOCUS_PATTERN_MIN_COVERED_DAYS;

struct DayData {
  uint16_t dayIndex = 0;
  uint16_t count = 0;
  uint32_t firstSecond = 86400U;
  uint32_t lastSecond = 0U;
  uint16_t hours[24]{};
};

Snapshot current;
bool dirty = true;
uint32_t lastScanMs = 0;
uint16_t cachedTodayIndex = 0;
uint32_t cachedLastTodayEpoch = 0;
uint32_t cachedLongestTodayCompleted = 0;
uint32_t cachedLongestWeekCompleted = 0;

bool due(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

uint32_t secondOfDay(const ProjectTime::LocalDateTime &local) {
  return static_cast<uint32_t>(local.hour) * 3600U +
         static_cast<uint32_t>(local.minute) * 60U + local.second;
}

void serviceInputDuringScan(uint16_t &counter) {
  ++counter;
  if ((counter & 0x3FU) != 0U) return;
  HardwareRegistry::update();
  InterruptionService::serviceUrgent();
  delay(0);
}

void refreshDynamicCurrent() {
  if (!current.timeValid || cachedLastTodayEpoch == 0U) {
    current.currentPhaseAvailable = false;
    current.currentPhaseSeconds = 0U;
    current.longestTodayAvailable = cachedLongestTodayCompleted > 0U;
    current.longestTodaySeconds = cachedLongestTodayCompleted;
    current.longestWeekAvailable = cachedLongestWeekCompleted > 0U;
    current.longestWeekSeconds = cachedLongestWeekCompleted;
    return;
  }

  const TimeTypes::Snapshot now = TimeService::now();
  ProjectTime::LocalDateTime local;
  if (!now.valid || now.epochMs < 0 || !ProjectTime::fromEpochMs(now.epochMs, local) ||
      local.dayIndex != cachedTodayIndex) {
    current.currentPhaseAvailable = false;
    current.currentPhaseSeconds = 0U;
    return;
  }

  const uint32_t nowSeconds = static_cast<uint32_t>(now.epochMs / 1000LL);
  if (nowSeconds < cachedLastTodayEpoch) {
    current.currentPhaseAvailable = false;
    current.currentPhaseSeconds = 0U;
    return;
  }

  current.currentPhaseAvailable = true;
  current.currentPhaseSeconds = nowSeconds - cachedLastTodayEpoch;
  current.longestTodaySeconds = std::max(cachedLongestTodayCompleted, current.currentPhaseSeconds);
  current.longestTodayAvailable = true;
  current.longestWeekSeconds = std::max(cachedLongestWeekCompleted, current.currentPhaseSeconds);
  current.longestWeekAvailable = true;
}

void calculatePatterns(DayData (&days)[PATTERN_DAYS]) {
  current.evaluatedDays = 0U;
  for (const auto &day : days) {
    if (day.count >= 2U) ++current.evaluatedDays;
  }

  FocusInsightsLogic::Candidate bestQuiet;
  for (uint8_t startHour = 0U; startHour <= 22U; ++startHour) {
    FocusInsightsLogic::Candidate candidate;
    candidate.startHour = startHour;
    for (const auto &day : days) {
      if (day.count < 2U ||
          !FocusInsightsLogic::windowCovered(day.firstSecond, day.lastSecond, startHour, 2U)) continue;
      ++candidate.coveredDays;
      candidate.eventSum += static_cast<uint32_t>(day.hours[startHour]) + day.hours[startHour + 1U];
    }
    if (candidate.coveredDays >= MIN_COVERED_DAYS && FocusInsightsLogic::betterQuiet(candidate, bestQuiet)) {
      bestQuiet = candidate;
    }
  }

  current.quietSufficient = bestQuiet.coveredDays >= MIN_COVERED_DAYS;
  current.quietCoveredDays = static_cast<uint8_t>(std::min<uint16_t>(bestQuiet.coveredDays, 255U));
  current.quietStartHour = bestQuiet.startHour;
  current.quietEndHour = static_cast<uint8_t>(bestQuiet.startHour + 2U);

  FocusInsightsLogic::Candidate bestPeak;
  for (uint8_t startHour = 0U; startHour <= 23U; ++startHour) {
    FocusInsightsLogic::Candidate candidate;
    candidate.startHour = startHour;
    for (const auto &day : days) {
      if (day.count < 2U ||
          !FocusInsightsLogic::windowCovered(day.firstSecond, day.lastSecond, startHour, 1U)) continue;
      ++candidate.coveredDays;
      candidate.eventSum += day.hours[startHour];
    }
    if (candidate.coveredDays >= MIN_COVERED_DAYS && FocusInsightsLogic::betterPeak(candidate, bestPeak)) {
      bestPeak = candidate;
    }
  }

  current.peakSufficient = bestPeak.coveredDays >= MIN_COVERED_DAYS;
  current.peakCoveredDays = static_cast<uint8_t>(std::min<uint16_t>(bestPeak.coveredDays, 255U));
  current.peakStartHour = bestPeak.startHour;
  current.peakEndHour = static_cast<uint8_t>(bestPeak.startHour + 1U);
}

void scan() {
  current = Snapshot{};
  cachedLastTodayEpoch = 0U;
  cachedLongestTodayCompleted = 0U;
  cachedLongestWeekCompleted = 0U;

  const TimeTypes::Snapshot time = TimeService::now();
  ProjectTime::LocalDateTime nowLocal;
  if (!time.valid || time.epochMs < 0 || !ProjectTime::fromEpochMs(time.epochMs, nowLocal)) {
    cachedTodayIndex = 0U;
    dirty = false;
    lastScanMs = millis();
    return;
  }

  current.timeValid = true;
  cachedTodayIndex = nowLocal.dayIndex;
  const uint32_t nowSeconds = static_cast<uint32_t>(time.epochMs / 1000LL);
  current.generatedEpochSeconds = nowSeconds;
  const uint16_t weekStart = static_cast<uint16_t>(nowLocal.dayIndex - nowLocal.weekday);
  const uint16_t patternStart = nowLocal.dayIndex > PATTERN_DAYS
                                    ? static_cast<uint16_t>(nowLocal.dayIndex - PATTERN_DAYS)
                                    : 0U;
  const uint16_t earliestNeeded = std::min(weekStart, patternStart);
  const uint32_t previousWindowStart = nowSeconds >= 7200U ? nowSeconds - 7200U : 0U;
  const uint32_t currentWindowStart = nowSeconds >= 3600U ? nowSeconds - 3600U : 0U;

  DayData days[PATTERN_DAYS]{};
  const uint64_t first = InterruptionStore::oldestSequence();
  const uint64_t last = InterruptionStore::newestSequence();
  uint16_t scanCounter = 0U;
  bool sawRecentAbsolute = false;
  bool reachedBeforePatternStart = false;
  uint16_t oldestAbsoluteDaySeen = nowLocal.dayIndex;

  if (first != 0U && last >= first) {
    for (uint64_t sequence = last;; --sequence) {
      serviceInputDuringScan(scanCounter);
      InterruptionTypes::RawEvent raw;
      if (InterruptionStore::readSequence(sequence, raw) && raw.absoluteValid && raw.timeValueSeconds <= nowSeconds) {
        ProjectTime::LocalDateTime local;
        if (ProjectTime::fromEpochSeconds(raw.timeValueSeconds, local)) {
          sawRecentAbsolute = true;
          oldestAbsoluteDaySeen = local.dayIndex;

          if (local.dayIndex == nowLocal.dayIndex && cachedLastTodayEpoch == 0U) {
            cachedLastTodayEpoch = raw.timeValueSeconds;
          }

          if (raw.deltaSeconds > 0U && raw.deltaSeconds < InterruptionTypes::DELTA_UNKNOWN) {
            if (local.dayIndex == nowLocal.dayIndex) {
              cachedLongestTodayCompleted = std::max(cachedLongestTodayCompleted, raw.deltaSeconds);
            }
            if (local.dayIndex >= weekStart && local.dayIndex <= nowLocal.dayIndex) {
              cachedLongestWeekCompleted = std::max(cachedLongestWeekCompleted, raw.deltaSeconds);
            }
          }

          if (raw.timeValueSeconds >= currentWindowStart) {
            ++current.trendLast60;
          } else if (raw.timeValueSeconds >= previousWindowStart && raw.timeValueSeconds < currentWindowStart) {
            ++current.trendPrevious60;
          }

          if (local.dayIndex < nowLocal.dayIndex && local.dayIndex >= patternStart) {
            const uint16_t ageDays = static_cast<uint16_t>(nowLocal.dayIndex - local.dayIndex);
            if (ageDays >= 1U && ageDays <= PATTERN_DAYS) {
              DayData &day = days[ageDays - 1U];
              if (day.dayIndex == 0U) day.dayIndex = local.dayIndex;
              ++day.count;
              const uint32_t sod = secondOfDay(local);
              day.firstSecond = std::min(day.firstSecond, sod);
              day.lastSecond = std::max(day.lastSecond, sod);
              if (local.hour < 24U && day.hours[local.hour] != UINT16_MAX) ++day.hours[local.hour];
            }
          }

          if (local.dayIndex < earliestNeeded) {
            if (local.dayIndex < patternStart) reachedBeforePatternStart = true;
            break;
          }
        }
      }
      if (sequence == first) break;
    }
  }

  current.trendDirection = FocusInsightsLogic::classifyTrend(current.trendPrevious60, current.trendLast60);
  calculatePatterns(days);

  const uint32_t stored = InterruptionStore::count();
  const uint32_t capacity = InterruptionStore::capacity();
  const bool wrapped = capacity > 0U && stored >= capacity;
  current.patternsCoverageComplete = !wrapped || reachedBeforePatternStart ||
                                     (sawRecentAbsolute && oldestAbsoluteDaySeen <= patternStart);

  current.longestTodayAvailable = cachedLongestTodayCompleted > 0U;
  current.longestTodaySeconds = cachedLongestTodayCompleted;
  current.longestWeekAvailable = cachedLongestWeekCompleted > 0U;
  current.longestWeekSeconds = cachedLongestWeekCompleted;
  refreshDynamicCurrent();

  dirty = false;
  lastScanMs = millis();
}

}  // namespace

void begin() {
  current = Snapshot{};
  dirty = true;
  lastScanMs = 0U;
  cachedTodayIndex = 0U;
  cachedLastTodayEpoch = 0U;
  cachedLongestTodayCompleted = 0U;
  cachedLongestWeekCompleted = 0U;
}

void update() {
  const TimeTypes::Snapshot time = TimeService::now();
  ProjectTime::LocalDateTime local;
  const bool localValid = time.valid && time.epochMs >= 0 && ProjectTime::fromEpochMs(time.epochMs, local);
  const bool dayChanged = localValid && cachedTodayIndex != 0U && local.dayIndex != cachedTodayIndex;
  const uint32_t nowMs = millis();
  if (dirty || dayChanged || lastScanMs == 0U || due(nowMs, lastScanMs + CACHE_MAX_AGE_MS)) {
    scan();
  } else {
    refreshDynamicCurrent();
  }
}

void markDirty() {
  dirty = true;
}

const Snapshot &snapshot() {
  update();
  return current;
}

const char *trendDirectionName(FocusInsightsLogic::TrendDirection direction) {
  switch (direction) {
    case FocusInsightsLogic::TrendDirection::Falling: return "falling";
    case FocusInsightsLogic::TrendDirection::Rising: return "rising";
    case FocusInsightsLogic::TrendDirection::Stable:
    default: return "stable";
  }
}

}  // namespace FocusInsights
