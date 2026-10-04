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
bool goodbyeActive = false;
uint32_t goodbyeUntilMs = 0;
uint32_t nextGoodbyeRenderMs = 0;

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

void beginGoodbye() {
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

  if (local.dayIndex != state.dayIndex) {
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

  const uint32_t nowMs = millis();
  if (logicalState) {
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
  if (state.active && currentLocal(epochSeconds, local) && local.dayIndex != state.dayIndex) {
    finalizeAutomaticEnd(epochSeconds);
  }

  SerialLog::successf("CYCLE", "Work-cycle manager ready | channel=%s | long-press=%lums | active=%s",
                      ProjectConfig::WORK_CYCLE_INPUT_ID,
                      static_cast<unsigned long>(ProjectConfig::WORK_CYCLE_LONG_PRESS_MS),
                      state.active ? "yes" : "no");
}

void update() {
  uint32_t epochSeconds = 0;
  ProjectTime::LocalDateTime local;
  if (state.active && currentLocal(epochSeconds, local) && local.dayIndex != state.dayIndex) {
    finalizeAutomaticEnd(epochSeconds);
  }

  if (!goodbyeActive) return;
  const uint32_t nowMs = millis();
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
uint32_t suppressedPhysicalPressCount() { return shortPressGuard.suppressedCount; }
bool hasSuppressedPhysicalPress() { return shortPressGuard.hasSuppressed; }
uint32_t lastSuppressedPhysicalPressMs() { return shortPressGuard.lastSuppressedMs; }

}  // namespace WorkCycle
