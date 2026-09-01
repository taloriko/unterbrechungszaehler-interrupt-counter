#include "CounterService.h"

#include "LedService.h"
#include "SoundService.h"
#include "StorageService.h"
#include "TimeService.h"

void CounterService::begin(StorageService* storage, TimeService* time, LedService* led, SoundService* sound) {
  storage_ = storage;
  time_ = time;
  led_ = led;
  sound_ = sound;
}

bool CounterService::addNormalEvent(bool physicalButton) {
  if (!storage_ || !time_ || !time_->valid()) {
    if (led_) led_->signalWarning();
    return false;
  }

  if (!storage_->appendEvent(time_->epoch())) {
    if (led_) led_->signalWarning();
    return false;
  }

  if (physicalButton) pulseSequence_++;
  noteAction(1);
  if (led_) led_->signalStored();

  // Rueckmeldung startet direkt nach erfolgreichem Speichern. Dadurch ist der
  // Ton unabhaengig davon, ob das Ereignis ueber Web oder Taster kam, vor der
  // nachfolgenden Display-Aktualisierung dran.
  if (sound_) sound_->requestPlay();
  return true;
}

bool CounterService::deleteNormalEvent() {
  if (!storage_ || !storage_->deleteLastEvent()) {
    if (led_) led_->signalWarning();
    return false;
  }

  noteAction(2);
  if (led_) led_->signalDeleted();
  return true;
}

void CounterService::noteAction(uint8_t kind) {
  actionKind_ = kind;
  actionSequence_++;
}
