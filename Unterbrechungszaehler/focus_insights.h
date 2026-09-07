#pragma once

#include <Arduino.h>

#include "focus_insights_logic.h"

namespace FocusInsights {

struct Snapshot {
  bool timeValid = false;
  bool currentPhaseAvailable = false;
  uint32_t currentPhaseSeconds = 0;
  bool longestTodayAvailable = false;
  uint32_t longestTodaySeconds = 0;
  bool longestWeekAvailable = false;
  uint32_t longestWeekSeconds = 0;
  uint16_t trendPrevious60 = 0;
  uint16_t trendLast60 = 0;
  FocusInsightsLogic::TrendDirection trendDirection = FocusInsightsLogic::TrendDirection::Stable;

  bool patternsCoverageComplete = false;
  uint8_t evaluatedDays = 0;
  bool quietSufficient = false;
  uint8_t quietStartHour = 0;
  uint8_t quietEndHour = 0;
  uint8_t quietCoveredDays = 0;
  bool peakSufficient = false;
  uint8_t peakStartHour = 0;
  uint8_t peakEndHour = 0;
  uint8_t peakCoveredDays = 0;

  uint32_t generatedEpochSeconds = 0;
};

void begin();
void update();
void markDirty();
const Snapshot &snapshot();
const char *trendDirectionName(FocusInsightsLogic::TrendDirection direction);

}  // namespace FocusInsights
