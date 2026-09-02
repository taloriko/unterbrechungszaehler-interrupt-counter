#include "interruption_service.h"

#include <cstring>

#include "audio_dy_sv17f.h"
#include "display_views.h"
#include "gpio_module.h"
#include "interruption_aggregates.h"
#include "interruption_store.h"
#include "project_config.h"
#include "project_preferences.h"
#include "project_time.h"
#include "serial_log.h"
#include "status_registry.h"
#include "time_service.h"

namespace InterruptionService {
namespace {

InterruptionTypes::Summary currentSummary;
InterruptionTypes::CapturedEvent queue[ProjectConfig::PENDING_EVENT_CAPACITY];
uint8_t queueHead = 0;
uint8_t queueTail = 0;
uint8_t queueCount = 0;
uint32_t nextStorageRetryMs = 0;
uint8_t audioPending = 0;
uint16_t lastRotatingTrack = 1;
bool rotateFallbackLogged = false;
bool displayFeedbackPending = false;
bool aggregatesStarted = false;
bool aggregatesReadyLast = false;
uint32_t nextAggregateRetryMs = 0;

bool currentDayValid = false;
uint16_t currentDayIndex = 0;
bool anchorValid = false;
uint16_t anchorDayIndex = 0;
uint32_t anchorEpochSeconds = 0;
uint32_t lastDayCheckMs = 0;

bool due(uint32_t now, uint32_t deadline) { return static_cast<int32_t>(now - deadline) >= 0; }

void bumpRevision() {
  ++currentSummary.revision;
  if (currentSummary.revision == 0) ++currentSummary.revision;
}


uint32_t pendingUnassignedCount() {
  uint32_t count = 0;
  for (uint8_t i = 0, idx = queueHead; i < queueCount; ++i,
       idx = static_cast<uint8_t>((idx + 1U) % ProjectConfig::PENDING_EVENT_CAPACITY)) {
    if (!queue[idx].localCalendarValid) ++count;
  }
  return count;
}

void refreshStorageState() {
  const auto previousState = currentSummary.storageState;
  const uint64_t previousPersisted = currentSummary.persistedSequence;
  const uint8_t previousPending = currentSummary.pendingCount;
  const auto &store = InterruptionStore::info();
  if (currentSummary.droppedCount > 0 || !store.mounted || (!store.ready && !store.recovering)) {
    currentSummary.storageState = InterruptionTypes::StorageState::Error;
  } else if (store.recovering || !InterruptionAggregates::ready() || queueCount > 0) {
    currentSummary.storageState = InterruptionTypes::StorageState::Warning;
  } else {
    currentSummary.storageState = InterruptionTypes::StorageState::Ready;
  }
  currentSummary.persistedSequence = InterruptionStore::newestSequence();
  currentSummary.pendingCount = queueCount;
  const StatusRegistry::State statusState =
      currentSummary.storageState == InterruptionTypes::StorageState::Ready ? StatusRegistry::State::Ok :
      currentSummary.storageState == InterruptionTypes::StorageState::Warning ? StatusRegistry::State::Warning :
      StatusRegistry::State::Error;
  StatusRegistry::setState("data", statusState);
  if (previousState != currentSummary.storageState || previousPersisted != currentSummary.persistedSequence ||
      previousPending != currentSummary.pendingCount) bumpRevision();
}

void loadLastEvent() {
  InterruptionTypes::RawEvent raw;
  uint64_t sequence = 0;
  if (!InterruptionStore::readLast(raw, sequence)) return;
  currentSummary.lastAvailable = true;
  currentSummary.lastAbsoluteValid = raw.absoluteValid;
  currentSummary.lastTimeValueSeconds = raw.timeValueSeconds;
  currentSummary.lastTimeSource = raw.timeSource;
  currentSummary.lastEventSource = raw.eventSource;
  currentSummary.lastDeltaSeconds = raw.deltaSeconds;
  currentSummary.lastMonotonicMs = 0; // monotonic time cannot span reboot
  if (raw.absoluteValid) {
    ProjectTime::LocalDateTime local;
    if (ProjectTime::fromEpochSeconds(raw.timeValueSeconds, local)) {
      currentSummary.lastLocalDayIndex = local.dayIndex;
      anchorValid = true;
      anchorDayIndex = local.dayIndex;
      anchorEpochSeconds = raw.timeValueSeconds;
    }
  }
  currentSummary.persistedSequence = sequence;
  currentSummary.liveSequence = sequence;
}

void refreshCurrentDay(bool force) {
  const uint32_t nowMs = millis();
  if (!force && static_cast<uint32_t>(nowMs - lastDayCheckMs) < 500U) return;
  lastDayCheckMs = nowMs;
  const TimeTypes::Snapshot snapshot = TimeService::now();
  ProjectTime::LocalDateTime local;
  const bool valid = snapshot.valid && ProjectTime::fromEpochMs(snapshot.epochMs, local);
  if (!valid) {
    if (currentDayValid) {
      currentDayValid = false;
      currentSummary.todayCount = 0;
      bumpRevision();
      DisplayViews::requestHomeRefresh();
    }
    return;
  }
  if (!currentDayValid || local.dayIndex != currentDayIndex || force) {
    currentDayValid = true;
    currentDayIndex = local.dayIndex;
    currentSummary.todayCount = InterruptionAggregates::ready() ? InterruptionAggregates::countForDay(currentDayIndex) : 0;
    // Pending events captured for today are already counted in RAM; after a
    // source/date change recompute only persisted data and then add pending today.
    for (uint8_t i = 0, idx = queueHead; i < queueCount; ++i, idx = static_cast<uint8_t>((idx + 1U) % ProjectConfig::PENDING_EVENT_CAPACITY)) {
      if (queue[idx].localCalendarValid && queue[idx].localDayIndex == currentDayIndex) ++currentSummary.todayCount;
    }
    if (!(anchorValid && anchorDayIndex == currentDayIndex)) anchorValid = false;
    bumpRevision();
    DisplayViews::requestHomeRefresh();
  }
}

void onGpioChanged(const char *channelId, bool logicalState) {
  if (!logicalState || !channelId || strcmp(channelId, ProjectConfig::INTERRUPTION_INPUT_ID) != 0) return;
  capture(InterruptionTypes::EventSource::PhysicalButton);
}

bool enqueue(const InterruptionTypes::CapturedEvent &event) {
  if (queueCount >= ProjectConfig::PENDING_EVENT_CAPACITY) return false;
  queue[queueTail] = event;
  queueTail = static_cast<uint8_t>((queueTail + 1U) % ProjectConfig::PENDING_EVENT_CAPACITY);
  ++queueCount;
  currentSummary.pendingCount = queueCount;
  return true;
}

void popQueue() {
  if (queueCount == 0) return;
  queueHead = static_cast<uint8_t>((queueHead + 1U) % ProjectConfig::PENDING_EVENT_CAPACITY);
  --queueCount;
  currentSummary.pendingCount = queueCount;
  bumpRevision();
}

uint16_t interruptionSoundTrack() {
  if (ProjectPreferences::soundMode() != ProjectPreferences::SoundMode::Rotate) {
    return ProjectPreferences::soundTrack();
  }

  const uint16_t count = AudioDySv17f::musicCount();
  if (count >= 2) {
    // Track 1 belongs exclusively to the boot sound. Rotate deterministically
    // through 2..N so consecutive interruptions never repeat a track when at
    // least two interruption tracks exist. No RNG state or heap is required.
    lastRotatingTrack = (lastRotatingTrack < 2 || lastRotatingTrack >= count)
                          ? 2
                          : static_cast<uint16_t>(lastRotatingTrack + 1U);
    rotateFallbackLogged = false;
    return lastRotatingTrack;
  }

  // The optional track-count query can be unavailable on some modules. Keep
  // feedback functional with the configured fixed fallback rather than
  // blocking/retrying in the interruption path.
  if (!rotateFallbackLogged) {
    SerialLog::warning("INTERRUPT", "Rotating sound requested but track count is unavailable/<2; using configured fixed track");
    rotateFallbackLogged = true;
  }
  return ProjectPreferences::soundTrack();
}

void processFeedback() {
  if (displayFeedbackPending) {
    displayFeedbackPending = false;
    DisplayViews::notifyInterruption(ProjectPreferences::displayFlashEnabled());
  }
  // Render the OLED before entering the audio command path. The DY-SV17F
  // driver may emit diagnostic serial output after sending its UART frame; that
  // must not postpone the user's visual acknowledgement.
  DisplayViews::update(currentSummary);
  if (audioPending > 0 && ProjectPreferences::soundEnabled()) {
    const uint16_t track = interruptionSoundTrack();
    if (AudioDySv17f::playTrack(track)) --audioPending;
  } else if (!ProjectPreferences::soundEnabled()) {
    audioPending = 0;
  }
}

void processPersistence() {
  if (queueCount == 0) return;
  const uint32_t nowMs = millis();
  if (!due(nowMs, nextStorageRetryMs)) return;
  if (!InterruptionStore::ready()) {
    nextStorageRetryMs = nowMs + ProjectConfig::STORAGE_RETRY_INTERVAL_MS;
    currentSummary.storageState = InterruptionTypes::StorageState::Warning;
    return;
  }

  const InterruptionTypes::CapturedEvent event = queue[queueHead];
  uint64_t sequence = 0;
  if (!InterruptionStore::append(event, sequence)) {
    nextStorageRetryMs = nowMs + ProjectConfig::STORAGE_RETRY_INTERVAL_MS;
    currentSummary.storageState = InterruptionTypes::StorageState::Error;
    SerialLog::error("INTERRUPT", "Event persistence failed; event kept in RAM queue for retry");
    return;
  }

  currentSummary.persistedSequence = sequence;
  bool aggregateOk = true;
  if (aggregatesStarted) aggregateOk = InterruptionAggregates::apply(event, sequence);
  if (!aggregateOk) {
    currentSummary.storageState = InterruptionTypes::StorageState::Warning;
    StatusRegistry::setState("data", StatusRegistry::State::Warning);
    SerialLog::warning("STATS", "Raw event persisted but daily aggregate update failed; raw data remains source of truth");
  }
  popQueue();
  refreshStorageState();
  char deltaText[24];
  if (event.deltaSeconds == InterruptionTypes::DELTA_FIRST_OF_DAY) {
    strncpy(deltaText, "first_of_day", sizeof(deltaText));
    deltaText[sizeof(deltaText) - 1U] = '\0';
  } else if (event.deltaSeconds == InterruptionTypes::DELTA_UNKNOWN) {
    strncpy(deltaText, "unknown", sizeof(deltaText));
    deltaText[sizeof(deltaText) - 1U] = '\0';
  } else {
    snprintf(deltaText, sizeof(deltaText), "%lus", static_cast<unsigned long>(event.deltaSeconds));
  }
  SerialLog::infof("INTERRUPT", "Persisted | sequence=%llu | source=%s | time=%s | delta=%s | pending=%u",
                   static_cast<unsigned long long>(sequence), InterruptionTypes::eventSourceName(event.eventSource),
                   TimeTypes::sourceName(event.timeSource), deltaText, static_cast<unsigned int>(queueCount));
}

}  // namespace

void begin() {
  ProjectTime::begin();
  ProjectPreferences::begin();
  currentSummary = InterruptionTypes::Summary{};
  currentSummary.soundEnabled = ProjectPreferences::soundEnabled();

  InterruptionStore::begin();
  if (InterruptionStore::ready()) aggregatesStarted = InterruptionAggregates::begin();
  aggregatesReadyLast = aggregatesStarted && InterruptionAggregates::ready();
  if (aggregatesReadyLast) {
    currentSummary.unassignedCount = InterruptionAggregates::info().unassignedCount + pendingUnassignedCount();
  }
  loadLastEvent();
  uint16_t persistedAnchorDay = 0;
  uint32_t persistedAnchorEpoch = 0;
  if (InterruptionStore::lastCalendarAnchor(persistedAnchorDay, persistedAnchorEpoch)) {
    anchorValid = true;
    anchorDayIndex = persistedAnchorDay;
    anchorEpochSeconds = persistedAnchorEpoch;
  }
  refreshStorageState();
  refreshCurrentDay(true);

  if (!GpioModule::registerInputChangedCallback(onGpioChanged)) {
    SerialLog::error("INTERRUPT", "Could not register DI callback");
  } else {
    SerialLog::successf("INTERRUPT", "Project input ready | channel=%s | physical button -> one interruption",
                        ProjectConfig::INTERRUPTION_INPUT_ID);
  }
  if (currentSummary.revision == 0) bumpRevision();
  DisplayViews::begin(currentSummary);
}

void update() {
  InterruptionStore::update();
  const uint32_t nowMs = millis();
  if (!aggregatesStarted && InterruptionStore::ready() && due(nowMs, nextAggregateRetryMs)) {
    aggregatesStarted = InterruptionAggregates::begin();
    if (aggregatesStarted) {
      aggregatesReadyLast = InterruptionAggregates::ready();
      if (aggregatesReadyLast) {
        currentSummary.unassignedCount = InterruptionAggregates::info().unassignedCount + pendingUnassignedCount();
        refreshCurrentDay(true);
      }
    } else {
      nextAggregateRetryMs = nowMs + ProjectConfig::STORAGE_RETRY_INTERVAL_MS;
    }
  }
  if (aggregatesStarted) {
    InterruptionAggregates::update();
    const auto &aggregateInfo = InterruptionAggregates::info();
    const bool aggregatesReadyNow = InterruptionAggregates::ready();
    if (aggregatesReadyNow && !aggregatesReadyLast) {
      // A rebuild/catch-up has just become authoritative again. Re-sync the
      // small RAM summary from the repaired aggregate store without touching
      // or scanning the raw ring. Pending events remain represented in RAM.
      currentSummary.unassignedCount = aggregateInfo.unassignedCount + pendingUnassignedCount();
      bumpRevision();
      refreshCurrentDay(true);
      DisplayViews::requestHomeRefresh();
    }
    aggregatesReadyLast = aggregatesReadyNow;
    if (!aggregateInfo.ready && !aggregateInfo.rebuilding) {
      aggregatesStarted = false;
      aggregatesReadyLast = false;
      nextAggregateRetryMs = nowMs + ProjectConfig::STORAGE_RETRY_INTERVAL_MS;
      SerialLog::warning("STATS", "Aggregate engine stopped; retry scheduled while raw events remain authoritative");
    }
  }
  refreshCurrentDay(false);

  // Feedback always runs before flash persistence. A physical press is already
  // captured in RAM when this method begins.
  serviceUrgent();
  processPersistence();
  refreshStorageState();
}

void serviceUrgent() {
  processFeedback();
}

bool capture(InterruptionTypes::EventSource source) {
  const TimeTypes::Snapshot snapshot = TimeService::eventTimestamp();
  InterruptionTypes::CapturedEvent event;
  event.monotonicMs = snapshot.monotonicMs;
  event.timeSource = snapshot.source;
  event.eventSource = source;
  event.absoluteValid = snapshot.valid;
  event.timeValueSeconds = snapshot.valid ? static_cast<uint32_t>(snapshot.epochMs / 1000LL)
                                          : static_cast<uint32_t>(snapshot.monotonicMs / 1000ULL);

  if (snapshot.valid) {
    ProjectTime::LocalDateTime local;
    if (ProjectTime::fromEpochSeconds(event.timeValueSeconds, local)) {
      event.localCalendarValid = true;
      event.localDayIndex = local.dayIndex;
      event.localHour = local.hour;
      if (anchorValid && anchorDayIndex == local.dayIndex) {
        if (event.timeValueSeconds >= anchorEpochSeconds) {
          const uint32_t delta = event.timeValueSeconds - anchorEpochSeconds;
          event.deltaSeconds = delta < InterruptionTypes::DELTA_MAX ? delta : InterruptionTypes::DELTA_MAX;
        } else {
          // A time correction moved the wall clock backwards. There is a prior
          // event on this day, but a positive interval cannot be derived safely.
          event.deltaSeconds = InterruptionTypes::DELTA_UNKNOWN;
        }
      } else {
        event.deltaSeconds = InterruptionTypes::DELTA_FIRST_OF_DAY;
      }
    }
  }

  const bool queuedForPersistence = enqueue(event);
  if (!queuedForPersistence) {
    if (currentSummary.droppedCount < UINT32_MAX) ++currentSummary.droppedCount;
    currentSummary.storageState = InterruptionTypes::StorageState::Error;
    StatusRegistry::setState("data", StatusRegistry::State::Error);
    SerialLog::errorf(
        "INTERRUPT",
        "Pending queue full; interruption captured in RAM/UI but NOT durably queued | lost-this-boot=%lu",
        static_cast<unsigned long>(currentSummary.droppedCount));
  }

  // The interruption happened even if persistence capacity is temporarily
  // exhausted. Keep the user-visible count, feedback and interval anchor true
  // to reality; droppedCount makes the durability gap explicit.
  if (event.localCalendarValid) {
    anchorValid = true;
    anchorDayIndex = event.localDayIndex;
    anchorEpochSeconds = event.timeValueSeconds;
  }

  ++currentSummary.liveSequence;
  bumpRevision();
  currentSummary.lastAvailable = true;
  currentSummary.lastAbsoluteValid = event.absoluteValid;
  currentSummary.lastTimeValueSeconds = event.timeValueSeconds;
  currentSummary.lastMonotonicMs = event.monotonicMs;
  currentSummary.lastTimeSource = event.timeSource;
  currentSummary.lastEventSource = source;
  currentSummary.lastDeltaSeconds = event.deltaSeconds;
  if (event.localCalendarValid) {
    currentSummary.lastLocalDayIndex = event.localDayIndex;
    if (!currentDayValid || event.localDayIndex != currentDayIndex) {
      currentDayValid = true;
      currentDayIndex = event.localDayIndex;
      currentSummary.todayCount = 0;
    }
    ++currentSummary.todayCount;
  } else {
    ++currentSummary.unassignedCount;
  }

  displayFeedbackPending = true;
  if (ProjectPreferences::soundEnabled() && audioPending < ProjectConfig::PENDING_EVENT_CAPACITY) ++audioPending;
  currentSummary.soundEnabled = ProjectPreferences::soundEnabled();

  // A physical button must feel immediate even before the main loop advances to
  // the project service. We are already outside ISR context here, so the normal
  // nonblocking display/audio command path can safely run now. Web-triggered
  // events deliberately return their HTTP response first and are serviced a
  // couple of milliseconds later by update().
  if (source == InterruptionTypes::EventSource::PhysicalButton) serviceUrgent();
  return true;
}

bool captureWeb() { return capture(InterruptionTypes::EventSource::WebButton); }
const InterruptionTypes::Summary &summary() { return currentSummary; }

bool setSoundEnabled(bool enabled) {
  if (!ProjectPreferences::setSoundEnabled(enabled)) return false;
  currentSummary.soundEnabled = enabled;
  bumpRevision();
  if (!enabled) audioPending = 0;
  return true;
}

bool soundEnabled() { return ProjectPreferences::soundEnabled(); }

}  // namespace InterruptionService
