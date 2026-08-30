#include "CounterService.h"

#include "LedService.h"
#include "StorageService.h"
#include "TimeService.h"

void CounterService::begin(StorageService* storage, TimeService* time, LedService* led) {
  storage_ = storage;
  time_ = time;
  led_ = led;
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
