#pragma once

#include <cstdint>

namespace FocusInsightsLogic {

enum class TrendDirection : uint8_t {
  Falling = 0,
  Stable = 1,
  Rising = 2
};

inline TrendDirection classifyTrend(uint32_t previous60, uint32_t last60) {
  const int64_t difference = static_cast<int64_t>(last60) - static_cast<int64_t>(previous60);
  if (difference <= -2) return TrendDirection::Falling;
  if (difference >= 2) return TrendDirection::Rising;
  return TrendDirection::Stable;
}

struct Candidate {
  uint32_t eventSum = 0;
  uint16_t coveredDays = 0;
  uint8_t startHour = 0;
};

inline bool windowCovered(uint32_t firstSecondOfDay,
                          uint32_t lastSecondOfDay,
                          uint8_t startHour,
                          uint8_t durationHours) {
  const uint32_t start = static_cast<uint32_t>(startHour) * 3600U;
  const uint32_t end = static_cast<uint32_t>(startHour + durationHours) * 3600U;
  return firstSecondOfDay <= start && lastSecondOfDay >= end;
}

inline bool sameAverage(const Candidate &a, const Candidate &b) {
  if (a.coveredDays == 0U || b.coveredDays == 0U) return false;
  return static_cast<uint64_t>(a.eventSum) * b.coveredDays ==
         static_cast<uint64_t>(b.eventSum) * a.coveredDays;
}

inline bool betterQuiet(const Candidate &candidate, const Candidate &best) {
  if (candidate.coveredDays == 0U) return false;
  if (best.coveredDays == 0U) return true;
  const uint64_t left = static_cast<uint64_t>(candidate.eventSum) * best.coveredDays;
  const uint64_t right = static_cast<uint64_t>(best.eventSum) * candidate.coveredDays;
  if (left != right) return left < right;
  if (candidate.coveredDays != best.coveredDays) return candidate.coveredDays > best.coveredDays;
  return candidate.startHour < best.startHour;
}

inline bool betterPeak(const Candidate &candidate, const Candidate &best) {
  if (candidate.coveredDays == 0U) return false;
  if (best.coveredDays == 0U) return true;
  const uint64_t left = static_cast<uint64_t>(candidate.eventSum) * best.coveredDays;
  const uint64_t right = static_cast<uint64_t>(best.eventSum) * candidate.coveredDays;
  if (left != right) return left > right;
  if (candidate.coveredDays != best.coveredDays) return candidate.coveredDays > best.coveredDays;
  return candidate.startHour < best.startHour;
}

}  // namespace FocusInsightsLogic
