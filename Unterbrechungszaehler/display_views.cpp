#include "display_views.h"

#include <algorithm>
#include <cstring>

#include "display_sh1106.h"
#include "project_config.h"
#include "project_preferences.h"
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
bool lastWifi = false;
bool lastTimeOk = false;
char lastAge[16] = "";
ProjectPreferences::DisplayMode lastMode = ProjectPreferences::DisplayMode::Standard;
int16_t lastContrast = -1;
bool dimmed = false;

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
  if (ageSeconds < 10) snprintf(out, 16, "JETZT");
  else if (ageSeconds < 60) snprintf(out, 16, "%llus", static_cast<unsigned long long>(ageSeconds));
  else if (ageSeconds < 3600) snprintf(out, 16, "%llum", static_cast<unsigned long long>(ageSeconds / 60ULL));
  else if (ageSeconds < 86400) snprintf(out, 16, "%lluh", static_cast<unsigned long long>(ageSeconds / 3600ULL));
  else {
    const uint32_t days = static_cast<uint32_t>(ageSeconds / 86400ULL);
    snprintf(out, 16, "%lud", static_cast<unsigned long>(days));
  }
}

void drawWifiIcon(int16_t x, int16_t y, bool connected) {
  if (!connected) {
    DisplaySh1106::drawText(x, y, "W-");
    return;
  }
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

uint8_t fittedScale(const char *value, uint8_t maxScale = 9) {
  if (!value || !*value) return 1;
  const size_t length = strlen(value);
  uint8_t scale = maxScale;
  while (scale > 1) {
    const size_t width = length * 6U * scale - scale;
    const size_t height = 7U * scale;
    if (width <= 126U && height <= 62U) break;
    --scale;
  }
  return scale;
}

void drawCenteredScaled(const char *value) {
  const uint8_t scale = fittedScale(value);
  const size_t length = value ? strlen(value) : 0U;
  const int16_t width = static_cast<int16_t>(length ? length * 6U * scale - scale : 0U);
  const int16_t height = static_cast<int16_t>(7U * scale);
  const int16_t x = std::max<int16_t>(0, static_cast<int16_t>((128 - width) / 2));
  const int16_t y = std::max<int16_t>(0, static_cast<int16_t>((64 - height) / 2));
  DisplaySh1106::drawTextScaled(x, y, value, scale);
}

bool renderStandard(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  DisplaySh1106::frameClear();
  DisplaySh1106::drawText(2, 1, "HEUTE");
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
  DisplaySh1106::drawText(2, 48, "LETZTE");
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
  drawCenteredScaled(age && *age ? age : "--");
  return DisplaySh1106::present();
}

bool renderHome(const InterruptionTypes::Summary &summary, const char *age, bool wifi, bool timeOk) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return false;
  switch (ProjectPreferences::displayMode()) {
    case ProjectPreferences::DisplayMode::CountOnly: return renderCountOnly(summary);
    case ProjectPreferences::DisplayMode::LastOnly: return renderLastOnly(age);
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
  renderRequested = true;
  update(summary);
}

void notifyInterruption(bool flashEnabled) {
  const uint32_t nowMs = millis();
  lastActivityMs = nowMs;
  dimmed = false;
  renderRequested = true;
  flashRequested = flashEnabled;
  // A new interruption always wakes the display to normal brightness even when
  // the optional inversion flash is disabled.
  lastContrast = -1;
}

void requestHomeRefresh() { renderRequested = true; }

void settingsChanged() {
  lastActivityMs = millis();
  dimmed = false;
  lastContrast = -1;
  renderRequested = true;
}

void update(const InterruptionTypes::Summary &summary) {
  if (!DisplaySh1106::enabled() || !DisplaySh1106::detected()) return;
  const uint32_t nowMs = millis();
  updateContrast(nowMs);

  // The project loop runs every few milliseconds, but the idle OLED view does
  // not need that cadence. Immediate/transient rendering remains responsive;
  // ordinary age/Wi-Fi/time/dimmer evaluation is at most once per second.
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
  const bool changed = summary.todayCount != lastCount || wifi != lastWifi || timeOk != lastTimeOk ||
                       strcmp(age, lastAge) != 0 || mode != lastMode;
  if (!renderRequested && !changed) return;

  if (renderHome(summary, age, wifi, timeOk)) {
    lastCount = summary.todayCount;
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
