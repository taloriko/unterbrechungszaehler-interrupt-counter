#include "display_views.h"

#include <algorithm>
#include <cstring>

#include "display_sh1106.h"
#include "project_config.h"
#include "project_preferences.h"
#include "project_time.h"
#include "time_service.h"
#include "wifi_module.h"

namespace DisplayViews {
namespace {

bool renderRequested = true;
bool flashRequested = false;
bool flashActive = false;
uint32_t flashUntilMs = 0;
uint32_t lastIdleEvaluationMs = 0;
uint32_t lastActivityMs = 0;
uint32_t lastCount = UINT32_MAX;
uint64_t lastIntervalSum = UINT64_MAX;
uint32_t lastIntervalSamples = UINT32_MAX;
bool lastWifi = false;
bool lastTimeOk = false;
char lastAge[16] = "";
ProjectPreferences::DisplayMode lastMode = ProjectPreferences::DisplayMode::Standard;
int16_t lastContrast = -1;
bool dimmed = false;
bool displayPowerOn = true;
bool manualTestWasActive = false;

struct Labels {
  const char *today;
  const char *last;
  const char *now;
  const char *focus;
  const char *average;
};

const Labels &labels() {
  static const Labels de{"HEUTE", "LETZTE", "JETZT", "FOKUS", "SCHNITT"};
  static const Labels en{"TODAY", "LAST", "NOW", "FOCUS", "AVG"};
  static const Labels fr{"JOUR", "DERNIER", "MAINT", "FOCUS", "MOY"};
  static const Labels it{"OGGI", "ULTIMA", "ORA", "FOCUS", "MEDIA"};
  static const Labels swg{"HEIT", "LETSCHTE", "JETZT", "FOKUS", "SCHNITT"};
  const char *language = ProjectPreferences::language();
  if (strcmp(language, "en") == 0) return en;
  if (strcmp(language, "fr") == 0) return fr;
  if (strcmp(language, "it") == 0) return it;
  if (strncmp(language, "swg", 3) == 0) return swg;
  return de;
}

bool due(uint32_t now, uint32_t deadline) { return static_cast<int32_t>(now - deadline) >= 0; }

void ageText(const InterruptionTypes::Summary &summary, char out[16]) {
  if (!summary.lastAvailable) { snprintf(out, 16, "--"); return; }
  uint64_t ageSeconds = 0;
  bool known = false;
  if (summary.lastAbsoluteValid) {
    const TimeTypes::Snapshot now = TimeService::now();
    if (now.valid && now.epochMs >= static_cast<int64_t>(summary.lastTimeValueSeconds) * 1000LL) {
      ageSeconds = static_cast<uint64_t>(now.epochMs / 1000LL) - summary.lastTimeValueSeconds;
      known = true;
    }
  } else {
    const uint64_t nowMono = TimeService::eventTimestamp().monotonicMs;
    if (summary.lastMonotonicMs > 0 && nowMono >= summary.lastMonotonicMs) {
      ageSeconds = (nowMono - summary.lastMonotonicMs) / 1000ULL;
      known = true;
    }
  }
  if (!known) { snprintf(out, 16, "--"); return; }
  if (ageSeconds < 10) snprintf(out, 16, "%s", labels().now);
  else if (ageSeconds < 60) snprintf(out, 16, "%llus", static_cast<unsigned long long>(ageSeconds));
  else if (ageSeconds < 3600) snprintf(out, 16, "%llum", static_cast<unsigned long long>(ageSeconds / 60ULL));
  else if (ageSeconds < 86400) {
    const uint64_t hours = ageSeconds / 3600ULL;
    const uint64_t minutes = (ageSeconds % 3600ULL) / 60ULL;
    if (minutes) snprintf(out, 16, "%lluh%02llum", static_cast<unsigned long long>(hours), static_cast<unsigned long long>(minutes));
    else snprintf(out, 16, "%lluh", static_cast<unsigned long long>(hours));
  } else {
    snprintf(out, 16, "%llud", static_cast<unsigned long long>(ageSeconds / 86400ULL));
  }
}

void intervalAverageText(const InterruptionTypes::Summary &summary, char out[16]) {
  if (summary.todayIntervalSamples == 0U) { snprintf(out, 16, "--"); return; }
  const uint64_t seconds = summary.todayIntervalSumSeconds / summary.todayIntervalSamples;
  if (seconds < 60ULL) snprintf(out, 16, "%llus", static_cast<unsigned long long>(seconds));
  else if (seconds < 3600ULL) snprintf(out, 16, "%llum", static_cast<unsigned long long>(seconds / 60ULL));
  else {
    const uint64_t hours = seconds / 3600ULL;
    const uint64_t minutes = (seconds % 3600ULL) / 60ULL;
    snprintf(out, 16, "%lluh%02llum", static_cast<unsigned long long>(hours), static_cast<unsigned long long>(minutes));
  }
}

void drawWifiIcon(int16_t x, int16_t y, bool connected) {
  if (!connected) { DisplaySh1106::drawText(x, y, "W-"); return; }
  DisplaySh1106::drawPixel(x + 5, y + 8);
  DisplaySh1106::drawHLine(x + 3, x + 7, y + 6);
  DisplaySh1106::drawHLine(x + 1, x + 9, y + 3);
}

void drawTimeIcon(int16_t x, int16_t y, bool ok) {
  DisplaySh1106::drawRect(x, y, 10, 10);
  if (ok) {
    DisplaySh1106::drawVLine(x + 5, y + 2, y + 5);
    DisplaySh1106::drawHLine(x + 5, x + 7, y + 5);
  } else {
    DisplaySh1106::drawHLine(x + 2, x + 7, y + 5);
  }
}

uint8_t fittedScale(const char *value, uint8_t maxScale = 9, uint8_t maxWidth = 126, uint8_t maxHeight = 62) {
  if (!value || !*value) return 1;
  const size_t length = strlen(value);
  uint8_t scale = maxScale;
  while (scale > 1) {
    const size_t width = length * 6U * scale - scale;
    const size_t height = 7U * scale;
    if (width <= maxWidth && height <= maxHeight) break;
    --scale;
  }
  return scale;
}

void drawCenteredScaledAt(const char *value, int16_t y, uint8_t maxScale, uint8_t maxWidth = 126) {
  const char *shown = value && *value ? value : "--";
  const uint8_t scale = fittedScale(shown, maxScale, maxWidth, 62);
  const size_t length = strlen(shown);
  const int16_t width = static_cast<int16_t>(length ? length * 6U * scale - scale : 0U);
  const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((128 - width) / 2));
  DisplaySh1106::drawTextScaled(x, y, shown, scale);
}

void drawCenteredScaled(const char *value) {
  const char *shown = value && *value ? value : "--";
  const uint8_t scale = fittedScale(shown);
  const size_t length = strlen(shown);
  const int16_t width = static_cast<int16_t>(length ? length * 6U * scale - scale : 0U);
  const int16_t height = static_cast<int16_t>(7U * scale);
  const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((128 - width) / 2));
  const int16_t y = std::max<int16_t>(0, static_cast<int16_t>((64 - height) / 2));
  DisplaySh1106::drawTextScaled(x, y, shown, scale);
}

bool renderStandard(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawText(2, 1, labels().today);
  drawTimeIcon(98, 0, timeOk);
  drawWifiIcon(114, 0, wifi);
  DisplaySh1106::drawHLine(0, 127, 12);

  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  const size_t digits = strlen(count);
  const int16_t width = static_cast<int16_t>(digits * 18U - (digits ? 3U : 0U));
  const int16_t x = static_cast<int16_t>((128 - width) / 2);
  DisplaySh1106::drawTextScaled(x < 0 ? 0 : x, 17, count, 3);

  DisplaySh1106::drawHLine(0, 127, 43);
  DisplaySh1106::drawText(2, 48, labels().last);
  DisplaySh1106::drawText(80, 48, age);
  return DisplaySh1106::present();
}

bool renderCountOnly(const InterruptionTypes::Summary &summary) {
  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  DisplaySh1106::frameClear();
  drawCenteredScaled(count);
  return DisplaySh1106::present();
}

bool renderLastOnly(const char *age) {
  DisplaySh1106::frameClear();
  drawCenteredScaled(age);
  return DisplaySh1106::present();
}

bool renderDayProgress(const InterruptionTypes::Summary &summary, bool wifi, bool timeOk) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawText(2, 1, labels().today);
  drawTimeIcon(98, 0, timeOk);
  drawWifiIcon(114, 0, wifi);
  DisplaySh1106::drawHLine(0, 127, 12);

  char count[12];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(summary.todayCount));
  const uint8_t countScale = fittedScale(count, 3, 58, 28);
  DisplaySh1106::drawTextScaled(4, 20, count, countScale);

  char average[16];
  intervalAverageText(summary, average);
  DisplaySh1106::drawText(68, 18, labels().average);
  DisplaySh1106::drawText(68, 31, average);

  // A tiny clock-progress bar gives the day overview useful visual structure
  // without inventing a score or target that the project does not know.
  DisplaySh1106::drawRect(5, 53, 118, 7);
  if (timeOk) {
    const TimeTypes::Snapshot now = TimeService::now();
    ProjectTime::LocalDateTime local;
    if (now.valid && ProjectTime::fromEpochMs(now.epochMs, local)) {
      const uint16_t minuteOfDay = static_cast<uint16_t>(local.hour) * 60U + local.minute;
      const int16_t filled = static_cast<int16_t>((static_cast<uint32_t>(minuteOfDay) * 114U) / 1439U);
      for (int16_t x = 7; x < 7 + filled; ++x) {
        DisplaySh1106::drawVLine(x, 55, 57);
      }
    }
  }
  return DisplaySh1106::present();
}

bool renderFocus(const InterruptionTypes::Summary &summary, const char *age) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawCenteredText(2, labels().focus);
  DisplaySh1106::drawHLine(0, 127, 12);
  drawCenteredScaledAt(age, 20, 4, 124);
  DisplaySh1106::drawHLine(0, 127, 48);
  char footer[22];
  snprintf(footer, sizeof(footer), "%s %lu", labels().today, static_cast<unsigned long>(summary.todayCount));
  DisplaySh1106::drawCenteredText(53, footer);
  return DisplaySh1106::present();
}

bool renderHome(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return false;
  switch (ProjectPreferences::displayMode()) {
    case ProjectPreferences::DisplayMode::CountOnly: return renderCountOnly(summary);
    case ProjectPreferences::DisplayMode::LastOnly: return renderLastOnly(age);
    case ProjectPreferences::DisplayMode::DayProgress: return renderDayProgress(summary, wifi, timeOk);
    case ProjectPreferences::DisplayMode::Focus: return renderFocus(summary, age);
    case ProjectPreferences::DisplayMode::Standard:
    default: return renderStandard(summary, age, wifi, timeOk);
  }
}

uint8_t contrastFromPercent(uint8_t percent) {
  return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U + 50U) / 100U);
}

void updateContrast(uint32_t nowMs) {
  const uint16_t dimMinutes = ProjectPreferences::displayDimAfterMinutes();
  const uint32_t dimAfterMs = static_cast<uint32_t>(dimMinutes) * 60000UL;
  const bool shouldDim = dimMinutes > 0U && static_cast<uint32_t>(nowMs - lastActivityMs) >= dimAfterMs;
  const uint8_t percent = shouldDim ? ProjectPreferences::displayDimBrightnessPercent()
                                    : ProjectPreferences::displayBrightnessPercent();
  const int16_t desired = contrastFromPercent(percent);
  if (desired == lastContrast && shouldDim == dimmed) return;
  if (DisplaySh1106::setContrast(static_cast<uint8_t>(desired))) {
    lastContrast = desired;
    dimmed = shouldDim;
  }
}

}  // namespace

void begin(const InterruptionTypes::Summary &summary) {
  lastActivityMs = millis();
  lastMode = ProjectPreferences::displayMode();
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  displayPowerOn = true;
  renderRequested = true;
  update(summary);
}

void notifyInterruption(bool flashEnabled) {
  const uint32_t nowMs = millis();
  lastActivityMs = nowMs;
  dimmed = false;
  renderRequested = true;
  lastContrast = -1;
  if (!ProjectPreferences::displayEnabled() || DisplaySh1106::bootScreenActive()) {
    flashRequested = false;
    return;
  }
  flashRequested = flashEnabled;
}

void requestHomeRefresh() { renderRequested = true; }

void settingsChanged() {
  lastActivityMs = millis();
  dimmed = false;
  lastContrast = -1;
  renderRequested = true;

  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  DisplaySh1106::setRotation180(ProjectPreferences::displayRotation180());
  if (DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }
  if (DisplaySh1106::setPower(true)) displayPowerOn = true;
}

void update(const InterruptionTypes::Summary &summary) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  const uint32_t nowMs = millis();

  if (DisplaySh1106::bootScreenActive()) {
    displayPowerOn = true;
    return;
  }
  if (DisplaySh1106::manualTestActive()) {
    manualTestWasActive = true;
    displayPowerOn = true;
    return;
  }
  if (manualTestWasActive) {
    manualTestWasActive = false;
    renderRequested = true;
    lastContrast = -1;
  }

  if (!ProjectPreferences::displayEnabled()) {
    if (flashActive) DisplaySh1106::setInverted(false);
    flashActive = false;
    flashRequested = false;
    if (displayPowerOn && DisplaySh1106::setPower(false)) displayPowerOn = false;
    return;
  }

  if (!displayPowerOn) {
    if (!DisplaySh1106::setPower(true)) return;
    displayPowerOn = true;
    lastContrast = -1;
    renderRequested = true;
  }

  updateContrast(nowMs);

  if (!renderRequested && !flashRequested && !flashActive &&
      static_cast<uint32_t>(nowMs - lastIdleEvaluationMs) < 1000U) return;
  if (flashActive && !due(nowMs, flashUntilMs)) return;
  lastIdleEvaluationMs = nowMs;

  if (flashActive && due(nowMs, flashUntilMs)) {
    DisplaySh1106::setInverted(false);
    flashActive = false;
    renderRequested = true;
  }

  char age[16]; ageText(summary, age);
  const bool wifi = WifiModule::stationConnected();
  const bool timeOk = TimeService::now().valid;
  const auto mode = ProjectPreferences::displayMode();
  const bool changed = summary.todayCount != lastCount ||
                       summary.todayIntervalSumSeconds != lastIntervalSum ||
                       summary.todayIntervalSamples != lastIntervalSamples ||
                       wifi != lastWifi || timeOk != lastTimeOk || strcmp(age, lastAge) != 0 || mode != lastMode;
  if (!renderRequested && !changed) return;

  if (renderHome(summary, age, wifi, timeOk)) {
    lastCount = summary.todayCount;
    lastIntervalSum = summary.todayIntervalSumSeconds;
    lastIntervalSamples = summary.todayIntervalSamples;
    lastWifi = wifi;
    lastTimeOk = timeOk;
    lastMode = mode;
    strncpy(lastAge, age, sizeof(lastAge) - 1);
    lastAge[sizeof(lastAge) - 1] = '\0';
    renderRequested = false;
    if (flashRequested) {
      flashRequested = false;
      if (DisplaySh1106::setInverted(true)) {
        flashActive = true;
        flashUntilMs = nowMs + ProjectConfig::DISPLAY_FLASH_MS;
      }
    }
  }
}

}  // namespace DisplayViews
