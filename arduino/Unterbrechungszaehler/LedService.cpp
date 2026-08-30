#include "LedService.h"

#include "Config.h"

namespace {
const LedService::Step STORED_PATTERN[] = {
  {true, 90}, {false, 1}
};

const LedService::Step DELETED_PATTERN[] = {
  {true, 55}, {false, 70},
  {true, 55}, {false, 70},
  {true, 55}, {false, 1}
};

const LedService::Step WARNING_PATTERN[] = {
  {true, 55}, {false, 70},
  {true, 55}, {false, 220},
  {true, 280}, {false, 220},
  {true, 280}, {false, 1}
};
}

void LedService::begin() {
  pinMode(UicConfig::LED_PIN, OUTPUT);
  off();
}

void LedService::tick() {
  if (buttonPressed_) {
    write(true);
    return;
  }

  if (pattern_ == nullptr || patternLength_ == 0) {
    write(false);
    return;
  }

  const uint32_t now = millis();
  const Step& current = pattern_[patternIndex_];
  write(current.on);

  if (now - stepStartedAt_ < current.durationMs) return;

  patternIndex_++;
  stepStartedAt_ = now;
  if (patternIndex_ >= patternLength_) {
    pattern_ = nullptr;
    patternLength_ = 0;
    patternIndex_ = 0;
    write(false);
  }
}

void LedService::setButtonPressed(bool pressed) {
  buttonPressed_ = pressed;
  if (pressed) write(true);
}

void LedService::signalStored() {
  startPattern(STORED_PATTERN, sizeof(STORED_PATTERN) / sizeof(STORED_PATTERN[0]));
}

void LedService::signalDeleted() {
  startPattern(DELETED_PATTERN, sizeof(DELETED_PATTERN) / sizeof(DELETED_PATTERN[0]));
}

void LedService::signalWarning() {
  startPattern(WARNING_PATTERN, sizeof(WARNING_PATTERN) / sizeof(WARNING_PATTERN[0]));
}

void LedService::off() {
  buttonPressed_ = false;
  pattern_ = nullptr;
  patternLength_ = 0;
  patternIndex_ = 0;
  write(false);
}

void LedService::startPattern(const Step* pattern, uint8_t length) {
  pattern_ = pattern;
  patternLength_ = length;
  patternIndex_ = 0;
  stepStartedAt_ = millis();
}

void LedService::write(bool on) {
  if (outputState_ == on) return;
  outputState_ = on;
  const bool electricalHigh = UicConfig::LED_ACTIVE_LOW ? !on : on;
  digitalWrite(UicConfig::LED_PIN, electricalHigh ? HIGH : LOW);
}
