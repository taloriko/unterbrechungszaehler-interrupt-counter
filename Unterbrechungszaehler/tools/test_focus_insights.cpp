#include <cassert>
#include <cstdint>
#include <iostream>

#include "../focus_insights_logic.h"

using FocusInsightsLogic::Candidate;
using FocusInsightsLogic::TrendDirection;

int main() {
  assert(FocusInsightsLogic::classifyTrend(6, 3) == TrendDirection::Falling);
  assert(FocusInsightsLogic::classifyTrend(6, 4) == TrendDirection::Falling);
  assert(FocusInsightsLogic::classifyTrend(6, 5) == TrendDirection::Stable);
  assert(FocusInsightsLogic::classifyTrend(6, 6) == TrendDirection::Stable);
  assert(FocusInsightsLogic::classifyTrend(6, 7) == TrendDirection::Stable);
  assert(FocusInsightsLogic::classifyTrend(6, 8) == TrendDirection::Rising);

  assert(FocusInsightsLogic::windowCovered(7U * 3600U, 12U * 3600U, 7, 2));
  assert(!FocusInsightsLogic::windowCovered(7U * 3600U + 1U, 12U * 3600U, 7, 2));
  assert(!FocusInsightsLogic::windowCovered(7U * 3600U, 9U * 3600U - 1U, 7, 2));
  assert(FocusInsightsLogic::windowCovered(9U * 3600U, 10U * 3600U, 9, 1));

  Candidate quietA{10, 5, 7};   // 2.0/day
  Candidate quietB{12, 6, 8};   // 2.0/day, more coverage wins
  assert(FocusInsightsLogic::betterQuiet(quietB, quietA));
  assert(!FocusInsightsLogic::betterQuiet(quietA, quietB));
  Candidate quietEarlier{12, 6, 6};
  assert(FocusInsightsLogic::betterQuiet(quietEarlier, quietB));
  Candidate quieter{5, 5, 10};  // 1.0/day wins regardless of later hour
  assert(FocusInsightsLogic::betterQuiet(quieter, quietEarlier));

  Candidate peakA{20, 5, 10};    // 4.0/day
  Candidate peakB{18, 6, 11};    // 3.0/day
  assert(FocusInsightsLogic::betterPeak(peakA, peakB));
  Candidate peakMoreCoverage{24, 6, 12}; // 4.0/day, more coverage wins
  assert(FocusInsightsLogic::betterPeak(peakMoreCoverage, peakA));
  Candidate peakEarlier{24, 6, 9};
  assert(FocusInsightsLogic::betterPeak(peakEarlier, peakMoreCoverage));

  std::cout << "focus insights logic tests passed\n";
  return 0;
}
