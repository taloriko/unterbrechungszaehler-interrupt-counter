#include "work_cycle.h"

#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>

#include "audio_dy_sv17f.h"
#include "display_sh1106.h"
#include "display_views.h"
#include "gpio_module.h"
#include "interruption_service.h"
#include "physical_button_guard.h"
#include "project_config.h"
#include "project_preferences.h"
#include "project_time.h"
#include "serial_log.h"
#include "time_service.h"

namespace WorkCycle {
namespace {

constexpr uint32_t STATE_MAGIC = 0x32435943UL;  // "CYC2"
constexpr uint8_t JOURNAL_START = 1;
constexpr uint8_t JOURNAL_END = 2;
constexpr uint8_t END_REASON_MANUAL = 1;
constexpr uint8_t END_REASON_DAY_CHANGE = 2;

struct PersistentState {
  uint32_t magic = STATE_MAGIC;
  uint16_t dayIndex = 0;
  uint8_t active = 0;
  uint8_t reserved = 0;
  uint32_t startEpochSeconds = 0;
};

PersistentState state;
PhysicalButtonGuard::State shortPressGuard;
bool buttonDown = false;
uint32_t buttonDownMs = 0;
bool holdPreviewActive = false;
uint32_t nextHoldPreviewMs = 0;
bool goodbyeActive = false;
uint32_t goodbyeUntilMs = 0;
uint32_t nextGoodbyeRenderMs = 0;
bool startDisplayActive = false;
uint32_t startDisplayUntilMs = 0;

bool due(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

bool currentLocal(uint32_t &epochSeconds, ProjectTime::LocalDateTime &local) {
  const TimeTypes::Snapshot snapshot = TimeService::eventTimestamp();
  if (!snapshot.valid || snapshot.epochMs < 0) return false;
  epochSeconds = static_cast<uint32_t>(snapshot.epochMs / 1000LL);
  return ProjectTime::fromEpochSeconds(epochSeconds, local);
}

void saveState() {
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::WORK_CYCLE_PREF_NAMESPACE, false)) {
    SerialLog::warning("CYCLE", "Could not persist work-cycle state");
    return;
  }
  prefs.putBytes("state", &state, sizeof(state));
  prefs.end();
}

void loadState() {
  state = PersistentState{};
  Preferences prefs;
  if (!prefs.begin(ProjectConfig::WORK_CYCLE_PREF_NAMESPACE, true)) return;
  PersistentState loaded;
  const size_t length = prefs.getBytesLength("state");
  if (length == sizeof(loaded) && prefs.getBytes("state", &loaded, sizeof(loaded)) == sizeof(loaded) &&
      loaded.magic == STATE_MAGIC) {
    state = loaded;
  }
  prefs.end();
}

void appendJournal(uint32_t epochSeconds, uint16_t dayIndex, uint8_t type, uint8_t reason) {
  File file = LittleFS.open(ProjectConfig::CYCLE_JOURNAL_PATH, "a");
  if (!file) {
    SerialLog::warning("CYCLE", "Cycle journal unavailable; live state remains persisted in NVS");
    return;
  }

  uint8_t record[8];
  record[0] = static_cast<uint8_t>(epochSeconds);
  record[1] = static_cast<uint8_t>(epochSeconds >> 8);
  record[2] = static_cast<uint8_t>(epochSeconds >> 16);
  record[3] = static_cast<uint8_t>(epochSeconds >> 24);
  record[4] = static_cast<uint8_t>(dayIndex);
  record[5] = static_cast<uint8_t>(dayIndex >> 8);
  record[6] = type;
  record[7] = reason;

  if (file.write(record, sizeof(record)) != sizeof(record)) {
    SerialLog::warning("CYCLE", "Cycle journal write failed");
  }
  file.flush();
  file.close();
}

void showSuppressed(uint32_t nowMs) {
  const uint32_t elapsed = static_cast<uint32_t>(nowMs - shortPressGuard.lastAcceptedMs);
  const uint32_t remaining = elapsed < ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS
                                 ? ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS - elapsed
                                 : 0U;

  if (ProjectPreferences::soundEnabled()) {
    const uint16_t count = AudioDySv17f::musicCount();
    if (count == 0U || count >= ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK) {
      AudioDySv17f::playPriorityFeedbackTrack(ProjectConfig::INTERRUPTION_SPAM_SOUND_TRACK);
    }
  }

  DisplayViews::notifySuppressedPhysicalPress(remaining);
  DisplayViews::update(InterruptionService::summary());
  SerialLog::infof("CYCLE", "short press suppressed | remaining=%lums",
                   static_cast<unsigned long>(remaining));
}

// A cycle's configured deadline belongs to the start day when it is still
// ahead of the starting minute; otherwise it belongs to the next local day.
// This calculation uses only integer calendar fields and the existing RTC.
bool cycleDeadlineReached(const ProjectTime::LocalDateTime &nowLocal) {
  if (!state.active) return false;
  ProjectTime::LocalDateTime startLocal;
  if (!ProjectTime::fromEpochSeconds(state.startEpochSeconds, startLocal)) return false;
  const uint16_t cutoff = ProjectPreferences::workCycleEndMinute();
  const uint16_t startedAt = static_cast<uint16_t>(startLocal.hour * 60U + startLocal.minute);
  const uint16_t deadlineDay = static_cast<uint16_t>(startLocal.dayIndex + (startedAt >= cutoff ? 1U : 0U));
  const uint16_t nowMinute = static_cast<uint16_t>(nowLocal.hour * 60U + nowLocal.minute);
  return nowLocal.dayIndex > deadlineDay ||
         (nowLocal.dayIndex == deadlineDay && nowMinute >= cutoff);
}

void showCycleStart() {
  if (!ProjectPreferences::displayEnabled() || !DisplaySh1106::enabled() || !DisplaySh1106::detected() ||
      DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;
  const char *language = ProjectPreferences::language();
  const char *upper = "LOS";
  const char *lower = "GEHT\u0027S!";
  if (language && strcmp(language, "en") == 0) {
    upper = "LET\u0027S"; lower = "GO!";
  } else if (language && strcmp(language, "fr") == 0) {
    upper = "C'EST"; lower = "PARTI!";
  } else if (language && strcmp(language, "it") == 0) {
    upper = "SI"; lower = "PARTE!";
  } else if (language && strncmp(language, "swg", 3) == 0) {
    upper = "AUF"; lower = "GEHTS!";
  }
  DisplaySh1106::setPower(true);
  DisplaySh1106::setInverted(false);
  DisplaySh1106::frameClear();
  const int16_t upperWidth = static_cast<int16_t>(strlen(upper) * 12U - 2U);
  const int16_t lowerWidth = static_cast<int16_t>(strlen(lower) * 12U - 2U);
  DisplaySh1106::drawTextScaled((128 - upperWidth) / 2, 11, upper, 2);
  DisplaySh1106::drawTextScaled((128 - lowerWidth) / 2, 35, lower, 2);
  DisplaySh1106::present();
}

void startCycle(uint32_t epochSeconds, const ProjectTime::LocalDateTime &local) {
  state.magic = STATE_MAGIC;
  state.active = 1;
  state.dayIndex = local.dayIndex;
  state.startEpochSeconds = epochSeconds;

  // START is only a cycle boundary. It must not enter interruption feedback and
  // must not consume the 10-second anti-spam window used by real interruptions.
  shortPressGuard = PhysicalButtonGuard::State{};
  saveState();
  appendJournal(epochSeconds, local.dayIndex, JOURNAL_START, 0);
  startDisplayActive = true;
  startDisplayUntilMs = millis() + ProjectConfig::WORK_CYCLE_START_DISPLAY_MS;
  showCycleStart();

  SerialLog::successf("CYCLE", "Work cycle started | day=%u | epoch=%lu",
                      static_cast<unsigned int>(local.dayIndex),
                      static_cast<unsigned long>(epochSeconds));
}

void renderGoodbye() {
  if (!ProjectPreferences::displayEnabled() || !DisplaySh1106::enabled() || !DisplaySh1106::detected() ||
      DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;

  DisplaySh1106::setPower(true);
  DisplaySh1106::setInverted(false);
  DisplaySh1106::frameClear();

  const char *language = ProjectPreferences::language();
  const char *title = "FEIERABEND";
  const char *today = "HEUTE";
  const char *unit = "UNTERBR.";
  if (language && strcmp(language, "en") == 0) {
    title = "DONE FOR TODAY";
    today = "TODAY";
    unit = "INTERRUPT.";
  } else if (language && strcmp(language, "fr") == 0) {
    title = "FIN DE JOURNEE";
    today = "JOUR";
    unit = "INTERRUPT.";
  } else if (language && strcmp(language, "it") == 0) {
    title = "FINE LAVORO";
    today = "OGGI";
    unit = "INTERRUZ.";
  }

  DisplaySh1106::drawCenteredText(3, title);
  DisplaySh1106::drawHLine(0, 127, 14);

  char count[16];
  snprintf(count, sizeof(count), "%lu", static_cast<unsigned long>(InterruptionService::summary().todayCount));
  DisplaySh1106::drawTextScaled(50, 20, count, 3);

  char footer[28];
  snprintf(footer, sizeof(footer), "%s %s", today, unit);
  DisplaySh1106::drawCenteredText(53, footer);
  DisplaySh1106::present();
}

// A fixed-rate progress preview: no heap allocation, persistence or new task.
// The original two-second gesture is still committed only when released.
void renderHoldPreview(uint32_t heldMs) {
  if (!ProjectPreferences::displayEnabled() || !DisplaySh1106::enabled() || !DisplaySh1106::detected() ||
      DisplaySh1106::bootScreenActive() || DisplaySh1106::manualTestActive()) return;

  const char *language = ProjectPreferences::language();
  const char *title = "FEIERABEND";
  const char *instruction = heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS ? "LOSLASSEN" : "HALTEN ...";
  if (language && strcmp(language, "en") == 0) {
    title = "FINISH WORK";
    instruction = heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS ? "RELEASE" : "KEEP HOLDING";
  } else if (language && strcmp(language, "fr") == 0) {
    title = "FIN DE JOURNEE";
    instruction = heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS ? "RELACHER" : "MAINTENIR";
  } else if (language && strcmp(language, "it") == 0) {
    title = "FINE LAVORO";
    instruction = heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS ? "RILASCIA" : "TIENI PREMUTO";
  } else if (language && strncmp(language, "swg", 3) == 0) {
    title = "FEIERABEND";
    instruction = heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS ? "LOS LASSA" : "HALTA ...";
  }

  DisplaySh1106::setPower(true);
  DisplaySh1106::setInverted(false);
  DisplaySh1106::frameClear();
  DisplaySh1106::drawCenteredText(5, title);
  DisplaySh1106::drawHLine(2, 125, 16);
  // The outline spans almost the full 128px panel width.
  DisplaySh1106::drawRect(2, 24, 124, 23);
  const uint32_t start = ProjectConfig::WORK_CYCLE_HOLD_FEEDBACK_AFTER_MS;
  const uint32_t finish = ProjectConfig::WORK_CYCLE_LONG_PRESS_MS;
  const uint32_t elapsed = heldMs >= finish ? finish - start : heldMs > start ? heldMs - start : 0U;
  const uint16_t width = static_cast<uint16_t>((elapsed * 120U) / (finish - start));
  for (uint16_t x = 0; x < width; ++x) {
    DisplaySh1106::drawVLine(static_cast<int16_t>(4 + x), 26, 44);
  }
  DisplaySh1106::drawCenteredText(54, instruction);
  DisplaySh1106::present();
}

void beginGoodbye() {
  holdPreviewActive = false;
  goodbyeActive = true;
  buttonDown = false;
  const uint32_t nowMs = millis();
  goodbyeUntilMs = nowMs + ProjectConfig::WORK_CYCLE_GOODBYE_DISPLAY_MS;
  nextGoodbyeRenderMs = nowMs;
  renderGoodbye();
}

void clearCycleState() {
  state = PersistentState{};
  saveState();
  shortPressGuard = PhysicalButtonGuard::State{};
}

void finalizeAutomaticEnd(uint32_t epochSeconds) {
  if (!state.active) return;

  // A forgotten explicit END closes only the cycle. Real interruptions have
  // already gone through the normal capture path and are never reclassified.
  appendJournal(epochSeconds, state.dayIndex, JOURNAL_END, END_REASON_DAY_CHANGE);
  SerialLog::infof("CYCLE", "Work cycle auto-ended at local day change | day=%u | epoch=%lu",
                   static_cast<unsigned int>(state.dayIndex),
                   static_cast<unsigned long>(epochSeconds));
  clearCycleState();
}

void finalizeManualEnd(uint32_t epochSeconds) {
  if (!state.active) return;

  appendJournal(epochSeconds, state.dayIndex, JOURNAL_END, END_REASON_MANUAL);
  SerialLog::successf("CYCLE", "Work cycle ended manually | day=%u | interruptions=%lu",
                      static_cast<unsigned int>(state.dayIndex),
                      static_cast<unsigned long>(InterruptionService::summary().todayCount));
  clearCycleState();
  startDisplayActive = false;
  beginGoodbye();
}

void captureShortPress(uint32_t nowMs) {
  if (!PhysicalButtonGuard::accept(shortPressGuard, nowMs, ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS)) {
    showSuppressed(nowMs);
    return;
  }

  // Reuse the exact proven 3.5 capture path. WorkCycle classifies the physical
  // gesture only; counting, normal Track 3+ feedback, RAM queueing, persistence
  // and analytics remain owned by InterruptionService.
  if (!InterruptionService::capture(InterruptionTypes::EventSource::PhysicalButton)) {
    SerialLog::warning("CYCLE", "Physical interruption capture failed");
    return;
  }

  SerialLog::info("CYCLE", "Physical interruption accepted immediately");
}

void handleShortPress(uint32_t nowMs, uint32_t epochSeconds, const ProjectTime::LocalDateTime &local) {
  if (!state.active) {
    startCycle(epochSeconds, local);
    return;
  }

  if (cycleDeadlineReached(local)) {
    finalizeAutomaticEnd(epochSeconds);
    startCycle(epochSeconds, local);
    return;
  }

  captureShortPress(nowMs);
}

void onGpioChanged(const char *channelId, bool logicalState) {
  if (!channelId || strcmp(channelId, ProjectConfig::WORK_CYCLE_INPUT_ID) != 0) return;

  // During the ten-second goodbye screen the local interaction path is closed.
  // Edges are intentionally ignored; the next complete press after the window
  // starts a fresh cycle normally.
  if (goodbyeActive) return;

  // Reset on every physical release, even when the threshold has been met.
  // A new press always starts a new independent preview from zero.
  if (!logicalState && holdPreviewActive) {
    holdPreviewActive = false;
    DisplayViews::requestHomeRefresh();
  }
  if (startDisplayActive) {
    startDisplayActive = false;
    DisplayViews::requestHomeRefresh();
  }
  const uint32_t nowMs = millis();
  if (logicalState) {
    holdPreviewActive = false;
    nextHoldPreviewMs = nowMs + ProjectConfig::WORK_CYCLE_HOLD_FEEDBACK_AFTER_MS;
    buttonDown = true;
    buttonDownMs = nowMs;
    return;
  }

  if (!buttonDown) return;
  buttonDown = false;
  const uint32_t heldMs = static_cast<uint32_t>(nowMs - buttonDownMs);

  uint32_t epochSeconds = 0;
  ProjectTime::LocalDateTime local;
  if (!currentLocal(epochSeconds, local)) {
    // Without a valid local calendar cycle boundaries cannot be classified.
    // Preserve the proven legacy interruption behavior and its 10-second guard
    // instead of guessing START/END semantics.
    if (heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS && state.active) {
      SerialLog::warning("CYCLE", "Local time unavailable; cannot close work cycle safely");
      return;
    }
    captureShortPress(nowMs);
    return;
  }

  if (state.active && heldMs >= ProjectConfig::WORK_CYCLE_LONG_PRESS_MS) {
    finalizeManualEnd(epochSeconds);
    return;
  }

  handleShortPress(nowMs, epochSeconds, local);
}

}  // namespace

void begin() {
  loadState();
  if (!GpioModule::registerInputChangedCallback(onGpioChanged)) {
    SerialLog::error("CYCLE", "Could not register work-cycle DI callback");
    return;
  }

  uint32_t epochSeconds = 0;
  ProjectTime::LocalDateTime local;
  if (state.active && currentLocal(epochSeconds, local) && cycleDeadlineReached(local)) {
    finalizeAutomaticEnd(epochSeconds);
  }

  SerialLog::successf("CYCLE", "Work-cycle manager ready | channel=%s | long-press=%lums | active=%s",
                      ProjectConfig::WORK_CYCLE_INPUT_ID,
                      static_cast<unsigned long>(ProjectConfig::WORK_CYCLE_LONG_PRESS_MS),
                      state.active ? "yes" : "no");
}

void update() {
  const uint32_t nowMs = millis();
  if (buttonDown && state.active && !goodbyeActive &&
      static_cast<uint32_t>(nowMs - buttonDownMs) >= ProjectConfig::WORK_CYCLE_HOLD_FEEDBACK_AFTER_MS &&
      due(nowMs, nextHoldPreviewMs)) {
    holdPreviewActive = true;
    renderHoldPreview(static_cast<uint32_t>(nowMs - buttonDownMs));
    nextHoldPreviewMs = nowMs + ProjectConfig::WORK_CYCLE_HOLD_FEEDBACK_FRAME_MS;
  }
  if (startDisplayActive && due(nowMs, startDisplayUntilMs)) {
    startDisplayActive = false;
    DisplayViews::requestHomeRefresh();
  }
  uint32_t epochSeconds = 0;
  ProjectTime::LocalDateTime local;
  if (state.active && currentLocal(epochSeconds, local) && cycleDeadlineReached(local)) {
    finalizeAutomaticEnd(epochSeconds);
  }

  if (!goodbyeActive) return;
  if (due(nowMs, goodbyeUntilMs)) {
    goodbyeActive = false;
    DisplayViews::requestHomeRefresh();
    return;
  }

  if (due(nowMs, nextGoodbyeRenderMs)) {
    renderGoodbye();
    nextGoodbyeRenderMs = nowMs + 250U;
  }
}

bool exclusiveGoodbyeActive() { return goodbyeActive; }
bool active() { return state.active != 0U; }
const char *stateName() {
  if (active()) return "active";
  return TimeService::now().valid ? "pause" : "time-unknown";
}
bool exclusiveDisplayActive() { return goodbyeActive || startDisplayActive || holdPreviewActive; }
uint32_t suppressedPhysicalPressCount() { return shortPressGuard.suppressedCount; }
bool hasSuppressedPhysicalPress() { return shortPressGuard.hasSuppressed; }
uint32_t lastSuppressedPhysicalPressMs() { return shortPressGuard.lastSuppressedMs; }

}  // namespace WorkCycle
