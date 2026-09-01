#include "AutarkService.h"

#include <esp_timer.h>

#include "LedService.h"
#include "SoundService.h"
#include "StorageService.h"
#include "TimeService.h"

void AutarkService::begin(StorageService* storage, TimeService* time, LedService* led, SoundService* sound) {
  storage_ = storage;
  time_ = time;
  led_ = led;
  sound_ = sound;
}

void AutarkService::tick() {
  if (active_ || pendingEndSession_ == 0 || !storage_ || !time_ || !time_->valid()) return;
  if (storage_->updateAutarkEndAnchor(pendingEndSession_, time_->epoch())) pendingEndSession_ = 0;
}

bool AutarkService::enter() {
  if (active_ || !storage_ || !storage_->autarkReady()) return false;

  const uint32_t anchor = time_ && time_->valid() ? time_->epoch() : 0;
  uint32_t session = 0;
  if (!storage_->startAutarkSession(anchor, session)) return false;

  active_ = true;
  sessionId_ = session;
  sessionEvents_ = 0;
  lastElapsedSeconds_ = 0;
  sessionStartedUs_ = static_cast<uint64_t>(esp_timer_get_time());
  return true;
}

bool AutarkService::leave() {
  if (!active_ || !storage_) return false;

  const uint32_t elapsed = elapsedSeconds();
  const uint32_t anchor = time_ && time_->valid() ? time_->epoch() : 0;
  const bool ok = storage_->endAutarkSession(sessionId_, elapsed, anchor);
  if (ok && anchor == 0) pendingEndSession_ = sessionId_;

  lastElapsedSeconds_ = elapsed;
  active_ = false;
  sessionStartedUs_ = 0;
  return ok;
}

bool AutarkService::addEvent() {
  if (!active_ || !storage_) return false;
  if (!storage_->appendAutarkEvent(sessionId_, elapsedSeconds())) {
    if (led_) led_->signalWarning();
    return false;
  }
  sessionEvents_++;
  if (led_) led_->signalStored();
  if (sound_) sound_->requestPlay();
  return true;
}

bool AutarkService::deleteLastEvent() {
  if (!active_ || !storage_ || !storage_->deleteLastAutarkEvent(sessionId_)) {
    if (led_) led_->signalWarning();
    return false;
  }
  if (sessionEvents_ > 0) sessionEvents_--;
  if (led_) led_->signalDeleted();
  return true;
}

uint32_t AutarkService::elapsedSeconds() const {
  if (!active_ || sessionStartedUs_ == 0) return 0;
  return static_cast<uint32_t>((static_cast<uint64_t>(esp_timer_get_time()) - sessionStartedUs_) / 1000000ULL);
}
