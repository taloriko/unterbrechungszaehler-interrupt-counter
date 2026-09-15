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

constexpr uint32_t STATE_MAGIC = 0x31435943UL;  // "CYC1"
constexpr uint8_t JOURNAL_START = 1;
constexpr uint8_t JOURNAL_END = 2;
constexpr uint8_t END_REASON_MANUAL = 1;
constexpr uint8_t END_REASON_DAY_CHANGE = 2;

struct PersistentState {
  uint32_t magic = STATE_MAGIC;
  uint16_t dayIndex = 0;
  uint8_t active = 0;
  uint8_t pending = 0;
  uint32_t startEpochSeconds = 0;
  uint32_t pendingEpochSeconds = 0;
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

void startCycle(uint32_t nowMs, uint32_t epochSeconds, const ProjectTime::LocalDateTime &local) {
  state.magic = STATE_MAGIC;
  state.active = 1;
  state.pending = 0;
  state.dayIndex = local.dayIndex;
  state.startEpochSeconds = epochSeconds;
  state.pendingEpochSeconds = 0;
  shortPressGuard = PhysicalButtonGuard::State{};
  PhysicalButtonGuard::accept(shortPressGuard, nowMs, ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS);
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
  // Same ownership principle as the anti-spam OLED effect, but stricter: for
  // the full goodbye window no other physical input may change project state
  // and the normal interruption display service is gated by the main loop.
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

void finalizeAutomaticEnd() {
  if (!state.active) return;
  const uint32_t endEpoch = state.pending && state.pendingEpochSeconds != 0U
                                ? state.pendingEpochSeconds
                                : state.startEpochSeconds;
  appendJournal(endEpoch, state.dayIndex, JOURNAL_END, END_REASON_DAY_CHANGE);
  SerialLog::infof("CYCLE", "Work cycle auto-ended at day change | day=%u | epoch=%lu",
                   static_cast<unsigned int>(state.dayIndex),
                   static_cast<unsigned long>(endEpoch));
  clearCycleState();
}

void finalizeManualEnd(uint32_t epochSeconds) {
  if (!state.active) return;

  // With an explicit long press, a preceding short press was not the final
  // event after all. Promote it to a real interruption before closing.
  if (state.pending && state.pendingEpochSeconds != 0U) {
    if (!InterruptionService::captureAtEpoch(state.pendingEpochSeconds,
                                              InterruptionTypes::EventSource::PhysicalButton)) {
      SerialLog::warning("CYCLE", "Pending interruption could not be finalized before manual cycle end");
    }
  }

  appendJournal(epochSeconds, state.dayIndex, JOURNAL_END, END_REASON_MANUAL);
  SerialLog::successf("CYCLE", "Work cycle ended manually | day=%u | interruptions=%lu",
                      static_cast<unsigned int>(state.dayIndex),
                      static_cast<unsigned long>(InterruptionService::summary().todayCount));
  clearCycleState();
  beginGoodbye();
}

void handleShortPress(uint32_t nowMs, uint32_t epochSeconds, const ProjectTime::LocalDateTime &local) {
  if (!state.active) {
    startCycle(nowMs, epochSeconds, local);
    return;
  }

  if (local.dayIndex != state.dayIndex) {
    finalizeAutomaticEnd();
    startCycle(nowMs, epochSeconds, local);
    return;
  }

  if (!PhysicalButtonGuard::accept(shortPressGuard, nowMs, ProjectConfig::PHYSICAL_BUTTON_COOLDOWN_MS)) {
    showSuppressed(nowMs);
    return;
  }

  if (state.pending && state.pendingEpochSeconds != 0U) {
    if (!InterruptionService::captureAtEpoch(state.pendingEpochSeconds,
                                              InterruptionTypes::EventSource::PhysicalButton)) {
      SerialLog::warning("CYCLE", "Pending interruption finalization failed; candidate retained");
      return;
    }
  }

  state.pending = 1;
  state.pendingEpochSeconds = epochSeconds;
  saveState();
  SerialLog::infof("CYCLE", "Last press candidate updated | day=%u | epoch=%lu",
                   static_cast<unsigned int>(state.dayIndex),
                   static_cast<unsigned long>(epochSeconds));
}

void onGpioChanged(const char *channelId, bool logicalState) {
  if (!channelId || strcmp(channelId, ProjectConfig::WORK_CYCLE_INPUT_ID) != 0) return;

  // The goodbye screen is an exclusive 10-second terminal state. Ignore all
  // physical edges until it has finished so a held/repeated press cannot start
  // another cycle or replace the overlay with normal feedback.
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
    // Without a valid local calendar the device cannot safely distinguish day
    // boundaries. Fall back to the proven legacy interruption path instead of
    // inventing cycle semantics.
    SerialLog::warning("CYCLE", "Local time unavailable; button recorded as legacy interruption");
    InterruptionService::capture(InterruptionTypes::EventSource::PhysicalButton);
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
    finalizeAutomaticEnd();
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
    finalizeAutomaticEnd();
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
