#include "InputService.h"

#include "AutarkService.h"
#include "Config.h"
#include "CounterService.h"
#include "DisplayService.h"
#include "LedService.h"

void InputService::begin(CounterService* counter, AutarkService* autark, LedService* led, DisplayService* display) {
  counter_ = counter;
  autark_ = autark;
  led_ = led;
  display_ = display;

  pinMode(UicConfig::BUTTON_PIN, INPUT_PULLUP);
  pinMode(UicConfig::AUTARK_PIN, INPUT_PULLUP);
  buttonRaw_ = buttonStable_ = digitalRead(UicConfig::BUTTON_PIN);
  autarkRaw_ = autarkStable_ = digitalRead(UicConfig::AUTARK_PIN);
  buttonChangedAt_ = autarkChangedAt_ = millis();
}

void InputService::tick() {
  processAutarkSwitch();
  processButton();
}

void InputService::processButton() {
  const bool raw = digitalRead(UicConfig::BUTTON_PIN);
  if (raw != buttonRaw_) {
    buttonRaw_ = raw;
    buttonChangedAt_ = millis();
  }

  if (millis() - buttonChangedAt_ < UicConfig::DEBOUNCE_MS || buttonStable_ == buttonRaw_) return;
  buttonStable_ = buttonRaw_;

  if (buttonStable_ == LOW) {
    buttonPressedAt_ = millis();
    if (led_) led_->setButtonPressed(true);
    return;
  }

  if (led_) led_->setButtonPressed(false);
  const uint32_t duration = millis() - buttonPressedAt_;

  if (!counter_ || !autark_) return;
  if (duration >= UicConfig::LONG_PRESS_MS) {
    if (autark_->active()) autark_->deleteLastEvent();
    else counter_->deleteNormalEvent();
    return;
  }

  bool stored = false;
  if (autark_->active()) stored = autark_->addEvent();
  else stored = counter_->addNormalEvent(true);

  if (stored && display_) display_->notifyActivity(autark_->active());
}

void InputService::processAutarkSwitch() {
  const bool raw = digitalRead(UicConfig::AUTARK_PIN);
  if (raw != autarkRaw_) {
    autarkRaw_ = raw;
    autarkChangedAt_ = millis();
  }

  if (millis() - autarkChangedAt_ < UicConfig::AUTARK_SWITCH_DEBOUNCE_MS || autarkStable_ == autarkRaw_) return;
  autarkStable_ = autarkRaw_;

  if (!autark_) return;
  if (autarkStable_ == LOW) autark_->enter();
  else autark_->leave();
}
